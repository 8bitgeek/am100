/* am600.c         (c) Copyright Mike Noel, 2001-2008                */
/* ----------------------------------------------------------------- */
/*                                                                   */
/* This software is an emulator for the Alpha-Micro AM-100 computer. */
/* It is copyright by Michael Noel and licensed for non-commercial   */
/* hobbyist use under terms of the "Q public license", an open       */
/* source certified license.  A copy of that license may be found    */
/* here:       http://www.otterway.com/am100/license.html            */
/*                                                                   */
/* There exist known serious discrepancies between this software's   */
/* internal functioning and that of a real AM-100, as well as        */
/* between it and the WD-1600 manual describing the functionality of */
/* a real AM-100, and even between it and the comments in the code   */
/* describing what it is intended to do! Use it at your own risk!    */
/*                                                                   */
/* Reliability aside, it isn't the intent of the copyright holder to */
/* use this software to compete with current or future Alpha-Micro   */
/* products, and no such competing application of the software will  */
/* be supported.                                                     */
/*                                                                   */
/* Alpha-Micro and other software that may be run on this emulator   */
/* are not covered by the above copyright or license and must be     */
/* legally obtained from an authorized source.                       */
/*                                                                   */
/* ----------------------------------------------------------------- */

#include "am100.h"

unsigned char bigbuf[65000];

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This module presents an am600 hardware interface to the cpu,      */
/* and presents a connection to a magnetic tape 'file'.              */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void am600_Init(unsigned int port,  // base port
                unsigned int mtlvl, // dma level
                char *filename,     // file name
                int filerw)         // file r/w flag
{
  CARDS *cptr;

  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("am600.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_am600;
  cptr->CType.AM600.port = port;
  cptr->CType.AM600.mtlvl = mtlvl;
  cptr->CType.AM600.LSTr3 = 0;
  cptr->CType.AM600.CNTr4 = 0;
  cptr->CType.AM600.DMAr4 = 0;
  cptr->CType.AM600.CNTr5 = 0;
  cptr->CType.AM600.DMAr5 = 0;
  for (cptr->CType.AM600.which = 0; cptr->CType.AM600.which < MAXTAPES;
       cptr->CType.AM600.which++)
    cptr->CType.AM600.file[cptr->CType.AM600.which] = NULL;
  cptr->CType.AM600.which = 0;

  /* register port(s) (note 2 & 6 not needed) */
  regAMport(port + 0, &am600_Port0, (unsigned char *)cptr);
  regAMport(port + 1, &am600_Port1, (unsigned char *)cptr);
  //      regAMport(port+2, &am600_Port2, (unsigned char *) cptr);
  regAMport(port + 3, &am600_Port3, (unsigned char *)cptr);
  regAMport(port + 4, &am600_Port4, (unsigned char *)cptr);
  regAMport(port + 5, &am600_Port5, (unsigned char *)cptr);
  //      regAMport(port+6, &am600_Port6, (unsigned char *) cptr);

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = cards;
  cards = cptr;

  /* open the initial tape file */
  am600_mount(port, cptr->CType.AM600.which, filename, filerw);
}

/*-------------------------------------------------------------------*/
/* Stop the board emulator                                           */
/*-------------------------------------------------------------------*/
void am600_stop() {
  CARDS *cptr;
  int which;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am600) {
      for (which = 0; which < MAXTAPES; which++)
        if (cptr->CType.AM600.file[which] != NULL)
          run_awstape(cptr, which);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Reset the board emulator                                          */
/*-------------------------------------------------------------------*/
void am600_reset(unsigned char *sa) {
  CARDS *cptr;
  int which;
  cptr = (CARDS *)sa;

  if (cptr->C_Type == C_Type_am600) {
    for (which = 0; which < MAXTAPES; which++)
      if (cptr->CType.AM600.file[which] != NULL)
        run_awstape(cptr, which);
  }
}

/*-------------------------------------------------------------------*/
/* Obtain list of currently mounted files...                         */
/*-------------------------------------------------------------------*/
void am600_getfiles(unsigned int port, char *fn[]) {
  CARDS *cptr;
  int unit;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am600)
      if (cptr->CType.AM600.port == port)
        for (unit = 0; unit < MAXTAPES; unit++) {
          if (cptr->CType.AM600.file[unit] != NULL)
            sprintf(fn[8 + unit], "%-28s", // left justified...
                    cptr->CType.AM600.filename[unit]);
          else
            sprintf(fn[8 + unit], "                           ");
        }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Mount a tape...                                                   */
/*-------------------------------------------------------------------*/
int am600_mount(unsigned int port, int unit, char *filename, int filerw) {
  CARDS *cptr;
  int itWorked = FALSE;

  am600_unmount(port, unit);

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am600)
      if (cptr->CType.AM600.port == port) {
        strncpy(cptr->CType.AM600.filename[unit], filename, 253);
        cptr->CType.AM600.filerw[unit] = filerw;
        if (open_awstape(cptr, unit) == 0)
          itWorked = TRUE;
      }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);

  return (itWorked);
}

/*-------------------------------------------------------------------*/
/* Un Mount a tape...                                                */
/*-------------------------------------------------------------------*/
void am600_unmount(unsigned int port, int unit) {
  CARDS *cptr;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am600)
      if (cptr->CType.AM600.port == port)
        if (cptr->CType.AM600.file[unit] != NULL) {
          run_awstape(cptr, unit);
          strcpy(cptr->CType.AM600.filename[unit], "");
        }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Port Handlers                                                     */
/*-------------------------------------------------------------------*/
void am600_Port0(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //
  // INPUT - drive status
  //
  // x'80' - ready
  // x'40' - online
  // x'20' - rewinding
  // x'10' - file protected
  // x'08' - load point
  // x'04' - end of tape
  // x'02' - ?
  // x'01' - ?
  //
  cptr = (CARDS *)sa;

  // regs.tracing=true;

  switch (rwflag) {

  case 0: /* read */
    if (cptr->CType.AM600.file[cptr->CType.AM600.which] == NULL)
      *chr = 0;
    else {
      *chr = 0x80 | 0x40;
      if (cptr->CType.AM600.filerw[cptr->CType.AM600.which] == 1)
        *chr |= 0x10;
      if (cptr->CType.AM600.filepos[cptr->CType.AM600.which] == 0)
        *chr |= 0x08;
    }
    break;

  case 1: /* write */
    cptr->CType.AM600.which = *chr & 7;
    break;

  default:
    assert("am600.c - invalid rwflag arg to am600_Port0...");
  }
}

void am600_Port1(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  unsigned char cmd;
  U16 cnt1, cnt3, ambuf, i, which;

  //
  // INPUT - interface status
  //
  // x'80' - formatter busy
  // x'40' - data ready
  // x'20' - lost data
  // x'10' - ?
  // x'08' - ?
  // x'04' - filemark detected
  // x'02' - error
  // x'01' - ?
  //
  cptr = (CARDS *)sa;
  which = cptr->CType.AM600.which;

  switch (rwflag) {

  case 0: /* read */

    if (cptr->CType.AM600.file[which] == NULL)
      *chr = 0;
    else {
      *chr = 0x40;
      if (cptr->CType.AM600.filecnt[which] == 0)
        *chr |= 0x04;
      if (feof(cptr->CType.AM600.file[which]))
        *chr &= 0xbf; // turn OFF data ready
      else if (ferror(cptr->CType.AM600.file[which]) != 0)
        *chr |= 0x22;
    }
    break;

  case 1: /* write */

    if (((*chr & 0x80) != 0) || // normal command w/GO or
        (*chr == 040))          // rewind issued wo/GO...
    {
      cnt1 = cptr->CType.AM600.CNTr4 + 256 * cptr->CType.AM600.CNTr5;
      ambuf = cptr->CType.AM600.DMAr4 + 256 * cptr->CType.AM600.DMAr5;
      cmd = *chr & 0x7f;

      if (cptr->CType.AM600.file[which] != NULL)
        switch (cmd) {
        case 0:   /*read */
          cnt1--; // read cnt set funny...
          cnt3 = read_awstape(cptr, which, (BYTE *)&bigbuf);
          for (i = 0; i < cnt1; i++)
            putAMbyte(&bigbuf[i], (ambuf + i));
          cptr->CType.AM600.filecnt[which] = cnt3;
          break;
        case 2: /*write */
          if (cptr->CType.AM600.filerw[which] == 1)
            cnt3 = 0;
          else {
            for (i = 0; i < cnt1; i++)
              getAMbyte(&bigbuf[i], (ambuf + i));
            cnt3 = write_awstape(cptr, which, (BYTE *)&bigbuf, cnt1);
          }
          cptr->CType.AM600.filecnt[which] = cnt3;
          break;
        case 4: /*fwd space block */
          cnt3 = fsb_awstape(cptr, which);
          cptr->CType.AM600.filecnt[which] = cnt3;
          break;
        case 5:
        case 21: /*bwd space block(s) */
          cnt3 = bsb_awstape(cptr, which);
          cptr->CType.AM600.filecnt[which] = cnt3;
          break;
        case 6: /*write tape mark */
          if (cptr->CType.AM600.filerw[which] != 1)
            write_awsmark(cptr, which);
          cptr->CType.AM600.filecnt[which] = 0;
          break;
        case 026: /*erase gap */
          if (cptr->CType.AM600.filerw[which] != 1)
            erg_awstape(cptr, which);
          break;
        case 040: /*rewind */
          rew_awstape(cptr, which);
          break;
        default:
          //        fprintf (stderr, "%d unknown command\n\r", cmd);
          break;
        }
      if (cptr->CType.AM600.file[which] != NULL)
        cptr->CType.AM600.filepos[which] = ftell(cptr->CType.AM600.file[which]);
    }
    break;

  default:
    assert("am600.c - invalid rwflag arg to am600_Port1...");
  }
}

// void am600_Port2(unsigned char *chr, int rwflag, unsigned char *sa)
//{
//}

void am600_Port3(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    *chr = 0xff;
    break;

  case 1: /* write */
    cptr->CType.AM600.LSTr3 = *chr;
    break;

  default:
    assert("am600.c - invalid rwflag arg to am600_Port3...");
  }
}

void am600_Port4(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    *chr = (cptr->CType.AM600.filecnt[cptr->CType.AM600.which] & 0xff);
    break;

  case 1: /* write */
    if (cptr->CType.AM600.LSTr3 == 0xA8)
      cptr->CType.AM600.DMAr4 = *chr;
    else if (cptr->CType.AM600.LSTr3 == 0xC8)
      cptr->CType.AM600.CNTr4 = *chr;
    break;

  default:
    assert("am600.c - invalid rwflag arg to am600_Port4...");
  }
}

void am600_Port5(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    *chr =
        ((cptr->CType.AM600.filecnt[cptr->CType.AM600.which] & 0xff00) / 256);
    break;

  case 1: /* write */
    if (cptr->CType.AM600.LSTr3 == 0xA8)
      cptr->CType.AM600.DMAr5 = *chr;
    else if (cptr->CType.AM600.LSTr3 == 0xC8)
      cptr->CType.AM600.CNTr5 = *chr;
    break;

  default:
    assert("am600.c - invalid rwflag arg to am600_Port5...");
  }
}

// void am600_Port6(unsigned char *chr, int rwflag, unsigned char *sa)
//{
//}

/*-------------------------------------------------------------------*/
/* The following AWS routines started life as functions in TAPEDEV.C */
/* in Hercules, an IBM 360-370-390-zSYS emulator.                    */
/* (c) Copyright Roger Bowler, 1999-2003                             */
/*                                                                   */
/* Original Author : Roger Bowler                                    */
/* Prime Maintainer : Ivan Warren                                    */
/* minor changes by John Summerfield                                 */
/*                                                                   */
/* I of course have introducted new and exciting errors entirely of  */
/* my own making for which the above can take no credit...           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Open an AWSTAPE format file                                       */
/*                                                                   */
/* If successful zero returned.              */
/*-------------------------------------------------------------------*/
int open_awstape(CARDS *cptr, int which) {
  /* first try read update mode */
  cptr->CType.AM600.file[which] =
      fopen(cptr->CType.AM600.filename[which], "r+");
  /* but if not R/O, try to create new, write TM,    */
  /* then retry opening read update mode...          */
  if (cptr->CType.AM600.file[which] == NULL) {
    /* if that fails and mode is R/O we are done...    */
    if (cptr->CType.AM600.filerw[which] == 1)
      return -1;
    cptr->CType.AM600.file[which] =
        fopen(cptr->CType.AM600.filename[which], "w");
    if (cptr->CType.AM600.file[which] == NULL)
      return -1;
    fclose(cptr->CType.AM600.file[which]);
    cptr->CType.AM600.file[which] =
        fopen(cptr->CType.AM600.filename[which], "r+");
    if (cptr->CType.AM600.file[which] == NULL)
      return -1;
    write_awsmark(cptr, which);
  }
  cptr->CType.AM600.filefirstwrite[which] = true;

  /* finish open with rewind to setup prev/next */
  rew_awstape(cptr, which);
  return 0;
}

/*-------------------------------------------------------------------*/
/* Read an AWSTAPE block header                                      */
/*                                                                   */
/* If successful, return value is zero, and buffer contains header.  */
/*-------------------------------------------------------------------*/
int readhdr_awstape(CARDS *cptr, int which, long blkpos, AWSTAPE_BLKHDR *buf) {
  int rc;

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* Reposition file to the requested block header */
  rc = fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
  if (rc != 0) {
    return -1;
  }

  /* Read the 6-byte block header */
  rc = fread(buf, 1, sizeof(AWSTAPE_BLKHDR), cptr->CType.AM600.file[which]);
  if (rc < sizeof(AWSTAPE_BLKHDR)) {
    if ((rc == 0) & feof(cptr->CType.AM600.file[which])) {
      return -3;
    }
    return -2;
  }
  return 0;
}

/*-------------------------------------------------------------------*/
/* Read a block from an AWSTAPE format file                          */
/*                                                                   */
/* If successful, return value is block length read. If a tapemark   */
/* was read, the return value is zero, and the current file number   */
/* is incremented.  On error a negative value is returned.       */
/*-------------------------------------------------------------------*/
int read_awstape(CARDS *cptr, int which, BYTE *buf) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 blklen;            /* Data length of block      */

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* Initialize current block position */
  blkpos = cptr->CType.AM600.nxtblkpos[which];

  /* Read the 6-byte block header */
  rc = readhdr_awstape(cptr, which, blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block length from the block header */
  blklen = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

  /* Calculate the offsets of the next and previous blocks */
  cptr->CType.AM600.nxtblkpos[which] = blkpos + sizeof(awshdr) + blklen;
  cptr->CType.AM600.prvblkpos[which] = blkpos;

  /* Increment file number and return zero if tapemark was read */
  if (blklen == 0) {
    cptr->CType.AM600.curfilen[which]++;
    return 0;
  }

  /* Read data block from tape file */
  rc = fread(buf, 1, blklen, cptr->CType.AM600.file[which]);
  if (rc < blklen) {
    return -4;
  }

  /* Return block length read */
  return blklen;
}

/*-------------------------------------------------------------------*/
/* Write a block to an AWSTAPE format file                           */
/*                                                                   */
/* If successful, return value is zero.                              */
/*-------------------------------------------------------------------*/
int write_awstape(CARDS *cptr, int which, BYTE *buf, U16 blklen) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 prvblkl;           /* Length of previous block  */

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* Initialize current block position and previous block length */
  blkpos = cptr->CType.AM600.nxtblkpos[which];
  prvblkl = 0;

  /* Determine previous block length if not at start of tape */
  if (blkpos > 0) {

    /* Reread the previous block header */
    rc = readhdr_awstape(cptr, which, cptr->CType.AM600.prvblkpos[which],
                         &awshdr);
    if (rc < 0) {
      return -1;
    }

    /* Extract the block length from the block header */
    prvblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

    /* Recalculate the offset of the next block */
    blkpos = cptr->CType.AM600.prvblkpos[which] + sizeof(awshdr) + prvblkl;
  }

  /* Reposition file to the new block header */
  rc = fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
  if (rc != 0) {
    return -2;
  }

  /* Build the 6-byte block header */
  awshdr.curblkl[0] = blklen & 0xFF;
  awshdr.curblkl[1] = (blklen >> 8) & 0xFF;
  awshdr.prvblkl[0] = prvblkl & 0xFF;
  awshdr.prvblkl[1] = (prvblkl >> 8) & 0xFF;
  awshdr.flags1 = AWSTAPE_FLAG1_NEWREC | AWSTAPE_FLAG1_ENDREC;
  awshdr.flags2 = 0;

  /* Write the block header */
  rc = fwrite(&awshdr, 1, sizeof(awshdr), cptr->CType.AM600.file[which]);
  fflush(cptr->CType.AM600.file[which]);
  if (rc < sizeof(awshdr)) {
    return -3;
  }

  /* Calculate the offsets of the next and previous blocks */
  cptr->CType.AM600.nxtblkpos[which] = blkpos + sizeof(awshdr) + blklen;
  cptr->CType.AM600.prvblkpos[which] = blkpos;

  /* Write the data block */
  rc = fwrite(buf, 1, blklen, cptr->CType.AM600.file[which]);
  fflush(cptr->CType.AM600.file[which]);
  if (rc < blklen) {
    return -4;
  }

  /* truncate if first write */
  if (cptr->CType.AM600.filefirstwrite[which]) {
    blkpos = ftell(cptr->CType.AM600.file[which]);
    ftruncate(fileno(cptr->CType.AM600.file[which]), blkpos);
    fclose(cptr->CType.AM600.file[which]);
    cptr->CType.AM600.file[which] =
        fopen(cptr->CType.AM600.filename[which], "r+");
    fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
    cptr->CType.AM600.filefirstwrite[which] = false;
  }

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Write a tapemark to an AWSTAPE format file                        */
/*                                                                   */
/* If successful, return value is zero.                              */
/*-------------------------------------------------------------------*/
int write_awsmark(CARDS *cptr, int which) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 prvblkl;           /* Length of previous block  */

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* Initialize current block position and previous block length */
  blkpos = cptr->CType.AM600.nxtblkpos[which];
  prvblkl = 0;

  /* Determine previous block length if not at start of tape */
  if (blkpos > 0) {

    /* Reread the previous block header */
    rc = readhdr_awstape(cptr, which, cptr->CType.AM600.prvblkpos[which],
                         &awshdr);
    if (rc < 0) {
      return -1;
    }

    /* Extract the block length from the block header */
    prvblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

    /* Recalculate the offset of the next block */
    blkpos = cptr->CType.AM600.prvblkpos[which] + sizeof(awshdr) + prvblkl;
  }

  /* Reposition file to the new block header */
  rc = fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
  if (rc != 0) {
    return -2;
  }

  /* Build the 6-byte block header */
  awshdr.curblkl[0] = 0;
  awshdr.curblkl[1] = 0;
  awshdr.prvblkl[0] = prvblkl & 0xFF;
  awshdr.prvblkl[1] = (prvblkl >> 8) & 0xFF;
  awshdr.flags1 = AWSTAPE_FLAG1_TAPEMARK;
  awshdr.flags2 = 0;

  /* Write the block header */
  rc = fwrite(&awshdr, 1, sizeof(awshdr), cptr->CType.AM600.file[which]);
  fflush(cptr->CType.AM600.file[which]);
  if (rc < sizeof(awshdr)) {
    return -3;
  }

  /* Calculate the offsets of the next and previous blocks */
  cptr->CType.AM600.nxtblkpos[which] = blkpos + sizeof(awshdr);
  cptr->CType.AM600.prvblkpos[which] = blkpos;

  /* truncate if first write */
  if (cptr->CType.AM600.filefirstwrite[which]) {
    blkpos = ftell(cptr->CType.AM600.file[which]);
    ftruncate(fileno(cptr->CType.AM600.file[which]), blkpos);
    fclose(cptr->CType.AM600.file[which]);
    cptr->CType.AM600.file[which] =
        fopen(cptr->CType.AM600.filename[which], "r+");
    fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
    cptr->CType.AM600.filefirstwrite[which] = false;
  }

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Forward space over next block of AWSTAPE format file              */
/*                                                                   */
/* If successful, return value is the length of the block skipped.   */
/* If the block skipped was a tapemark, the return value is zero,    */
/* and the current file number in the device block is incremented.   */
/*-------------------------------------------------------------------*/
int fsb_awstape(CARDS *cptr, int which) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 blklen;            /* Data length of block      */

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* Initialize current block position */
  blkpos = cptr->CType.AM600.nxtblkpos[which];

  /* Read the 6-byte block header */
  rc = readhdr_awstape(cptr, which, blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block length from the block header */
  blklen = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

  /* Calculate the offsets of the next and previous blocks */
  cptr->CType.AM600.nxtblkpos[which] = blkpos + sizeof(awshdr) + blklen;
  cptr->CType.AM600.prvblkpos[which] = blkpos;

  /* Increment current file number if tapemark was skipped */
  if (blklen == 0)
    cptr->CType.AM600.curfilen[which]++;

  /* Return block length or zero if tapemark */
  return blklen;
}

/*-------------------------------------------------------------------*/
/* Backspace to previous block of AWSTAPE format file                */
/*                                                                   */
/* If successful, return value is the length of the block.           */
/* If the block is a tapemark, the return value is zero,             */
/* and the current file number in the device block is decremented.   */
/*-------------------------------------------------------------------*/
int bsb_awstape(CARDS *cptr, int which) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  U16 curblkl;           /* Length of current block   */
  U16 prvblkl;           /* Length of previous block  */
  long blkpos;           /* Offset of block header    */

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* error if already at start of tape */
  if (cptr->CType.AM600.nxtblkpos[which] == 0) {
    return -1;
  }

  /* Backspace to previous block position */
  blkpos = cptr->CType.AM600.prvblkpos[which];

  /* Read the 6-byte block header */
  rc = readhdr_awstape(cptr, which, blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block lengths from the block header */
  curblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];
  prvblkl = ((U16)(awshdr.prvblkl[1]) << 8) | awshdr.prvblkl[0];

  /* Calculate the offset of the previous block */
  cptr->CType.AM600.prvblkpos[which] = blkpos - sizeof(awshdr) - prvblkl;
  cptr->CType.AM600.nxtblkpos[which] = blkpos;

  /* Decrement current file number if backspaced over tapemark */
  if (curblkl == 0)
    cptr->CType.AM600.curfilen[which]--;

  /* any future writes have to truncate again... */
  cptr->CType.AM600.filefirstwrite[which] = true;

  /* Return block length or zero if tapemark */
  return curblkl;
}

/*-------------------------------------------------------------------*/
/* Rewind AWSTAPE format file                                        */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int rew_awstape(CARDS *cptr, int which) {
  int rc;

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* rewind */
  rc = fseek(cptr->CType.AM600.file[which], 0L, SEEK_SET);
  if (rc != 0) {
    return -1;
  }

  /* Reset position counters to start of file */
  cptr->CType.AM600.curfilen[which] = 1;
  cptr->CType.AM600.nxtblkpos[which] = 0;
  cptr->CType.AM600.prvblkpos[which] = -1;

  cptr->CType.AM600.filepos[which] = 0;
  cptr->CType.AM600.filecnt[which] = 0;

  /* any future writes have to truncate again... */
  cptr->CType.AM600.filefirstwrite[which] = true;

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Rewind and Unload AWSTAPE format file                             */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int run_awstape(CARDS *cptr, int which) {
  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  rew_awstape(cptr, which);

  /* Close the file */
  if (cptr->CType.AM600.file[which] != NULL)
    fclose(cptr->CType.AM600.file[which]);
  cptr->CType.AM600.file[which] = NULL;

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Erase Gap AWSTAPE format file                                     */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int erg_awstape(CARDS *cptr, int which) {
  long blkpos;

  if (cptr->CType.AM600.file[which] == NULL)
    return -99;

  /* unconditional truncate */
  blkpos = ftell(cptr->CType.AM600.file[which]);
  ftruncate(fileno(cptr->CType.AM600.file[which]), blkpos);
  fclose(cptr->CType.AM600.file[which]);
  cptr->CType.AM600.file[which] =
      fopen(cptr->CType.AM600.filename[which], "r+");
  fseek(cptr->CType.AM600.file[which], blkpos, SEEK_SET);
  cptr->CType.AM600.filefirstwrite[which] = false;

  /* Return normal status */
  return 0;
}
