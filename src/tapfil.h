/*-------------------------------------------------------------------*/
/* tapfil.h            (c) Copyright Mike Noel, 2001-2008            */
/*                                                                   */
/* This file is included in the filtap, tapfil, and tapdir programs  */
/* that provide host functionality parrelleling the guest emulated   */
/* functionality of AMOS programs of the same name.                  */
/*                                                                   */
/* This is part of an emulator for the Alpha-Micro AM-100 computer.  */
/* It is copyright by Michael Noel and licensed for non-commercial   */
/* hobbyist use under terms of the "Q public license", an open       */
/* source certified license.  A copy of that license may be found    */
/* here:       http://www.otterway.com/am100/license.html            */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include <stdint.h>

typedef uint8_t BYTE;
typedef uint8_t HWORD[2];
typedef uint8_t U8;
typedef uint16_t U16;

#define TRUE 1
#define FALSE 0

#define AM_LITTLE_ENDIAN 1234
#define AM_BIG_ENDIAN 4321

#if defined(__i386__) || defined(__amd64__) || defined(__alpha__) ||           \
    (defined(__mips__) && (defined(MIPSEL) || defined(__MIPSEL__)))
#define AM_BYTE_ORDER 1234
#elif defined(__mc68000__) || defined(__sparc__) || defined(__PPC__) ||        \
    defined(__ppc__) || defined(__BIG_ENDIAN__) ||                             \
    (defined(__mips__) && (defined(MIPSEB) || defined(__MIPSEB__)))
#define AM_BYTE_ORDER 4321
#else
#warning tapfil.h can not determine endian-ness, using Little Endian
#define AM_BYTE_ORDER 1234
#endif

/*-------------------------------------------------------------------*/
/* AWStape stuff                                                     */
/*-------------------------------------------------------------------*/

typedef struct _AWSTAPE_BLKHDR {
  HWORD curblkl; /* Length of this block      */
  HWORD prvblkl; /* Length of previous block  */
  BYTE flags1;   /* Flags byte 1              */
  BYTE flags2;   /* Flags byte 2              */
} AWSTAPE_BLKHDR;

/* Definitions for AWSTAPE_BLKHDR flags byte 1 */
#define AWSTAPE_FLAG1_NEWREC 0x80   /* Start of new record       */
#define AWSTAPE_FLAG1_TAPEMARK 0x40 /* Tape mark                 */
#define AWSTAPE_FLAG1_ENDREC 0x20   /* End of record             */

FILE *tapefile = NULL;
char fnamebuf[255], *tapefilename = &fnamebuf[0];
int filerw = 0, filefirstwrite;
long nxtblkpos, prvblkpos, filepos;
int curfilen, filecnt;

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
int open_awstape() {
  /* first try read update mode */
  tapefile = fopen(tapefilename, "r+");
  /* if that fails and mode is R/O we are done...    */
  /* but if not R/O, try to create new, write TM,    */
  /* then retry opening read update mode...          */
  if (tapefile == NULL) {
    if (filerw == 1)
      return -1;
    tapefile = fopen(tapefilename, "w");
    if (tapefile == NULL)
      return -1;
    fclose(tapefile);
    tapefile = fopen(tapefilename, "r+");
    if (tapefile == NULL)
      return -1;
    write_awsmark();
  }
  filefirstwrite = TRUE;

  /* finish open with rewind to setup prev/next */
  rew_awstape();
  return 0;
}

/*-------------------------------------------------------------------*/
/* Read an AWSTAPE block header                                      */
/*                                                                   */
/* If successful, return value is zero, and buffer contains header.  */
/*-------------------------------------------------------------------*/
int readhdr_awstape(long blkpos, AWSTAPE_BLKHDR *buf) {
  int rc;

  /* Reposition file to the requested block header */
  rc = fseek(tapefile, blkpos, SEEK_SET);
  if (rc != 0) {
    return -1;
  }

  /* Read the 6-byte block header */
  rc = fread(buf, 1, sizeof(AWSTAPE_BLKHDR), tapefile);
  if (rc < sizeof(AWSTAPE_BLKHDR)) {
    if ((rc == 0) & feof(tapefile)) {
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
int read_awstape(BYTE *buf) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 blklen;            /* Data length of block      */

  /* Initialize current block position */
  blkpos = nxtblkpos;

  /* Read the 6-byte block header */
  rc = readhdr_awstape(blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block length from the block header */
  blklen = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

  /* Calculate the offsets of the next and previous blocks */
  nxtblkpos = blkpos + sizeof(awshdr) + blklen;
  prvblkpos = blkpos;

  /* Increment file number and return zero if tapemark was read */
  if (blklen == 0) {
    curfilen++;
    return 0;
  }

  /* Read data block from tape file */
  rc = fread(buf, 1, blklen, tapefile);
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
int write_awstape(BYTE *buf, U16 blklen) {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 prvblkl;           /* Length of previous block  */

  /* Initialize current block position and previous block length */
  blkpos = nxtblkpos;
  prvblkl = 0;

  /* Determine previous block length if not at start of tape */
  if (blkpos > 0) {

    /* Reread the previous block header */
    rc = readhdr_awstape(prvblkpos, &awshdr);
    if (rc < 0) {
      return -1;
    }

    /* Extract the block length from the block header */
    prvblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

    /* Recalculate the offset of the next block */
    blkpos = prvblkpos + sizeof(awshdr) + prvblkl;
  }

  /* Reposition file to the new block header */
  rc = fseek(tapefile, blkpos, SEEK_SET);
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
  rc = fwrite(&awshdr, 1, sizeof(awshdr), tapefile);
  fflush(tapefile);
  if (rc < sizeof(awshdr)) {
    return -3;
  }

  /* Calculate the offsets of the next and previous blocks */
  nxtblkpos = blkpos + sizeof(awshdr) + blklen;
  prvblkpos = blkpos;

  /* Write the data block */
  rc = fwrite(buf, 1, blklen, tapefile);
  fflush(tapefile);
  if (rc < blklen) {
    return -4;
  }

  /* truncate if first write */
  if (filefirstwrite) {
    blkpos = ftell(tapefile);
    ftruncate(fileno(tapefile), blkpos);
    fclose(tapefile);
    tapefile = fopen(tapefilename, "r+");
    fseek(tapefile, blkpos, SEEK_SET);
    filefirstwrite = FALSE;
  }

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Write a tapemark to an AWSTAPE format file                        */
/*                                                                   */
/* If successful, return value is zero.                              */
/*-------------------------------------------------------------------*/
int write_awsmark() {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 prvblkl;           /* Length of previous block  */

  /* Initialize current block position and previous block length */
  blkpos = nxtblkpos;
  prvblkl = 0;

  /* Determine previous block length if not at start of tape */
  if (blkpos > 0) {

    /* Reread the previous block header */
    rc = readhdr_awstape(prvblkpos, &awshdr);
    if (rc < 0) {
      return -1;
    }

    /* Extract the block length from the block header */
    prvblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

    /* Recalculate the offset of the next block */
    blkpos = prvblkpos + sizeof(awshdr) + prvblkl;
  }

  /* Reposition file to the new block header */
  rc = fseek(tapefile, blkpos, SEEK_SET);
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
  rc = fwrite(&awshdr, 1, sizeof(awshdr), tapefile);
  fflush(tapefile);
  if (rc < sizeof(awshdr)) {
    return -3;
  }

  /* Calculate the offsets of the next and previous blocks */
  nxtblkpos = blkpos + sizeof(awshdr);
  prvblkpos = blkpos;

  /* truncate if first write */
  if (filefirstwrite) {
    blkpos = ftell(tapefile);
    ftruncate(fileno(tapefile), blkpos);
    fclose(tapefile);
    tapefile = fopen(tapefilename, "r+");
    fseek(tapefile, blkpos, SEEK_SET);
    filefirstwrite = FALSE;
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
int fsb_awstape() {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  long blkpos;           /* Offset of block header    */
  U16 blklen;            /* Data length of block      */

  /* Initialize current block position */
  blkpos = nxtblkpos;

  /* Read the 6-byte block header */
  rc = readhdr_awstape(blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block length from the block header */
  blklen = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];

  /* Calculate the offsets of the next and previous blocks */
  nxtblkpos = blkpos + sizeof(awshdr) + blklen;
  prvblkpos = blkpos;

  /* Increment current file number if tapemark was skipped */
  if (blklen == 0)
    curfilen++;

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
int bsb_awstape() {
  int rc;                /* Return code               */
  AWSTAPE_BLKHDR awshdr; /* AWSTAPE block header      */
  U16 curblkl;           /* Length of current block   */
  U16 prvblkl;           /* Length of previous block  */
  long blkpos;           /* Offset of block header    */

  /* error if already at start of tape */
  if (nxtblkpos == 0) {
    return -1;
  }

  /* Backspace to previous block position */
  blkpos = prvblkpos;

  /* Read the 6-byte block header */
  rc = readhdr_awstape(blkpos, &awshdr);
  if (rc < 0) {
    return -1;
  }

  /* Extract the block lengths from the block header */
  curblkl = ((U16)(awshdr.curblkl[1]) << 8) | awshdr.curblkl[0];
  prvblkl = ((U16)(awshdr.prvblkl[1]) << 8) | awshdr.prvblkl[0];

  /* Calculate the offset of the previous block */
  prvblkpos = blkpos - sizeof(awshdr) - prvblkl;
  nxtblkpos = blkpos;

  /* Decrement current file number if backspaced over tapemark */
  if (curblkl == 0)
    curfilen--;

  /* any future writes have to truncate again... */
  filefirstwrite = TRUE;

  /* Return block length or zero if tapemark */
  return curblkl;
}

/*-------------------------------------------------------------------*/
/* Rewind AWSTAPE format file                                        */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int rew_awstape() {
  int rc;

  /* rewind */
  rc = fseek(tapefile, 0L, SEEK_SET);
  if (rc != 0) {
    return -1;
  }

  /* Reset position counters to start of file */
  curfilen = 1;
  nxtblkpos = 0;
  prvblkpos = -1;

  filepos = 0;
  filecnt = 0;

  /* any future writes have to truncate again... */
  filefirstwrite = TRUE;

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Rewind and Unload AWSTAPE format file                             */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int run_awstape() {
  rew_awstape();

  /* Close the file */
  if (tapefile != NULL)
    fclose(tapefile);
  tapefile = NULL;

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* Erase Gap AWSTAPE format file                                     */
/*                                                                   */
/* If successful, return value is zero                               */
/*-------------------------------------------------------------------*/
int erg_awstape() {
  long blkpos;

  /* unconditional truncate */
  blkpos = ftell(tapefile);
  ftruncate(fileno(tapefile), blkpos);
  fclose(tapefile);
  tapefile = fopen(tapefilename, "r+");
  fseek(tapefile, blkpos, SEEK_SET);
  filefirstwrite = FALSE;

  /* Return normal status */
  return 0;
}

/*-------------------------------------------------------------------*/
/* RAD50 stuff                                                       */
/*-------------------------------------------------------------------*/

char packtbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$. 0123456789";

void pack(uint16_t *word,
          uint8_t *cp) { // like AMOS PACK call, turns ascii into R50
  uint8_t ic, *c, ch[3];

  for (ic = 0; ic < 3; ic++) {
    ch[ic] = *cp;
    cp++;
    if (ch[ic] == '.')
      ch[ic] = 0;
  }
  if (ch[2] == 0)
    ch[2] = ' ';
  if (ch[1] == 0) {
    ch[1] = ' ';
    ch[2] = ' ';
  }
  if (ch[0] == 0) {
    ch[0] = ' ';
    ch[1] = ' ';
    ch[2] = ' ';
  }
  cp = &ch[0];
  c = (uint8_t *)strchr(packtbl, toupper(*cp));
  *word = ((int)c - (int)packtbl) * 03100;
  cp++;
  c = (uint8_t *)strchr(packtbl, toupper(*cp));
  *word += ((int)c - (int)packtbl) * 050;
  cp++;
  c = (uint8_t *)strchr(packtbl, toupper(*cp));
  *word += ((int)c - (int)packtbl);
  cp++;
}

void unpack(uint16_t *word,
            uint8_t *cp) { // like AMOS UNPK call, turns R50 into ascii
  if (*word < 0xfa00) {
    cp[0] = packtbl[*word / 03100];
    cp[1] = packtbl[(*word / 050) % 050];
    cp[2] = packtbl[*word % 050];
  } else {
    cp[0] = ' ';
    cp[1] = ' ';
    cp[2] = ' ';
  }
  cp[3] = 0;
}
