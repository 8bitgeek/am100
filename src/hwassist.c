/* hwassist.c         (c) Copyright Mike Noel, 2001-2008             */
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
#include "am-ddb.h"
#include "io.h"
#include "hwassist.h"
#include "memory.h"

//
// these are functions for 'hardware assist' of WD16 & AMOS services
//

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
  *word = (uint16_t)(((uint32_t)c - (uint32_t)packtbl) * 03100);
  cp++;
  c = (uint8_t *)strchr(packtbl, toupper(*cp));
  *word += ((uint32_t)c - (uint32_t)packtbl) * 050;
  cp++;
  c = (uint8_t *)strchr(packtbl, toupper(*cp));
  *word += ((uint32_t)c - (uint32_t)packtbl);
  cp++;
}

//
// following are routines supporting the 'virtual disk' driver...
//

FILE *vdkdsk[4];
long vdkmax[4]; // max byteoffset
char drv_name[4][255];

/*-------------------------------------------------------------------*/
/* Initialize the virtual disk driver...                             */
/*-------------------------------------------------------------------*/
void vdkdvr_start(void) {
  int drv;

  for (drv = 0; drv < 4; drv++) {
    sprintf((char *)&drv_name[drv][0], "dsk%d-container", drv);
    vdkdsk[drv] = fopen((char *)&drv_name[drv][0], "r+");
    vdkmax[drv] = 0;
    if (vdkdsk[drv] != NULL) {
      fseek(vdkdsk[drv], 0, SEEK_END);
      vdkmax[drv] = ftell(vdkdsk[drv]);
    } else
      drv_name[drv][0] = '\0';
  }
}

/*-------------------------------------------------------------------*/
/* Stop the virtual disk driver...                                   */
/*-------------------------------------------------------------------*/
void vdkdvr_stop(void) {
  int drv;

  for (drv = 0; drv < 4; drv++) {
    if (vdkdsk[drv] != NULL)
      fclose(vdkdsk[drv]);
    vdkdsk[drv] = NULL;
    vdkmax[drv] = 0;
  }
}

/*-------------------------------------------------------------------*/
/* reset the virtual disk driver...                                  */
/*-------------------------------------------------------------------*/
void vdkdvr_reset(void) {}

/*-------------------------------------------------------------------*/
/* Obtain list of currently mounted files...                         */
/*-------------------------------------------------------------------*/
void vdkdvr_getfiles(char *fn[]) {
  int drv;

  for (drv = 0; drv < 4; drv++)
    sprintf((char *)&fn[drv][0], "%-28s", drv_name[drv]);
}

/*-------------------------------------------------------------------*/
/* Mount a new file for the virtual disk driver...                   */
/*-------------------------------------------------------------------*/
int vdkdvr_mount(int unit, char *filename) {
  int itWorked = FALSE;

  vdkdvr_unmount(unit);

  sprintf((char *)&drv_name[unit][0], "%s", filename);
  vdkdsk[unit] = fopen((char *)&drv_name[unit][0], "r+");
  vdkmax[unit] = 0;
  if (vdkdsk[unit] != NULL) {
    fseek(vdkdsk[unit], 0, SEEK_END);
    vdkmax[unit] = ftell(vdkdsk[unit]);
    itWorked = TRUE;
  } else
    drv_name[unit][0] = '\0';

  return (itWorked);
}

/*-------------------------------------------------------------------*/
/* Un-Mount the file for the virtual disk driver...                  */
/*-------------------------------------------------------------------*/
void vdkdvr_unmount(int unit) {
  if (vdkdsk[unit] != NULL)
    fclose(vdkdsk[unit]);
  vdkdsk[unit] = NULL;
  vdkmax[unit] = 0;
  drv_name[unit][0] = '\0';
}

/*-------------------------------------------------------------------*/
/* This is the driver SVCC calls...                                  */
/*-------------------------------------------------------------------*/
void vdkdvr(void) {
  //
  // VDKDSK disk driver. 'DMA' to caller's bank is OK because this routine
  // doesn't return until I/O is done.
  //
  // on entry am100_state.wd16_cpu_state->regs.R0 -> 'surface' and am100_state.wd16_cpu_state->regs.R4 -> ddb.  of course the ddb has
  // the buffer address, buffer size, r/w flag, record number, rtn code, etc.
  //

  uint8_t c;
  uint8_t drv, rw, err;
  uint16_t recno, recsz, ambuf, i;
  long recnum;

  getAMbyte((uint8_t *)&rw, am100_state.wd16_cpu_state->regs.R4 + DB$FLG);
  rw = (rw >> 5) & 1; // 0=read, 1=write
  getAMword((uint8_t *)&ambuf, am100_state.wd16_cpu_state->regs.R4 + DB$BAD);
  // AMbyte((uint8_t *)&drv,   am100_state.wd16_cpu_state->regs.R4+DB$DRI);
  drv = am100_state.wd16_cpu_state->regs.R0 & 3; // drive 0, 1, 2, 3
  getAMword((uint8_t *)&recno, am100_state.wd16_cpu_state->regs.R4 + DB$RNM);
  recnum = (recno << 9); // turn recno into byte offset
  getAMword((uint8_t *)&recsz, am100_state.wd16_cpu_state->regs.R4 + DB$RSZ);

  if ((recnum + recsz) > vdkmax[drv]) {
    err = 016; // 'illegal block number'
  } else if (vdkdsk[drv] != NULL) {
    fseek(vdkdsk[drv], recnum, SEEK_SET);
    for (i = 0; i < recsz; i++) {
      if (rw == 1) {
        getAMbyte(&c, (ambuf + i));
        fwrite(&c, 1, 1, vdkdsk[drv]);
        fflush(vdkdsk[drv]);
      } else {
        fread(&c, 1, 1, vdkdsk[drv]);
        putAMbyte(&c, (ambuf + i));
      }
    }
    err = 000;
  } else
    err = 007; // 'device error'

  putAMbyte((uint8_t *)&err, am100_state.wd16_cpu_state->regs.R4 + DB$ERR);

  if (am100_state.wd16_cpu_state->regs.tracing)
    if (!am100_state.wd16_cpu_state->regs.utrace) {
      config_memdump(am100_state.wd16_cpu_state->regs.R4, SIZ$DB);
      config_memdump(ambuf, recsz);
    }
}

/*-------------------------------------------------------------------*/
/* vdkdvr_boot calls this to load monitor into memory                */
/*-------------------------------------------------------------------*/
int vdkload(int drv, char *name, int ppn, long where_to_put_it)
//
// returns size of loaded module or zero if error
//
{
  long seekto;
  int currentrec, i, tstufd, numlast, rc;
  uint16_t where, rnam[3], unam[3];
  uint8_t cbuf[512], *cptr;
  char *ii;

  // make the r50 name and extension
  ii = strstr(name, ".");
  pack(&rnam[0], (uint8_t *)&name[0]);
  if (ii > &name[2])
    pack(&rnam[1], (uint8_t *)&name[3]);
  else
    pack(&rnam[1], (uint8_t *)"   ");
  pack(&rnam[2], (uint8_t *)++ii);

  // get the UFD record
  /* read MFD sector */
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - read mfd.\r\n");
    fflush(stderr);
  }
  fseek(vdkdsk[drv], 1 * 512, SEEK_SET);
  rc = fread((char *)&cbuf[0], 512, 1, vdkdsk[drv]);
  if (rc == 0)
    return (0);

  /* find UFD */
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - find ufd.\r\n");
    fflush(stderr);
  }
  for (i = 0; i < 512; i += 8) {
    tstufd = cbuf[i] + 256 * cbuf[i + 1];
    if (tstufd == ppn)
      break; // found
  }
  if (i == 512)
    return (0);

  // find file in UFD
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - find %s entry in ufd.\r\n", name);
    fflush(stderr);
  }
  seekto = 512 * (cbuf[i + 2] + 256 * cbuf[i + 3]);
  do {
    fseek(vdkdsk[drv], seekto, SEEK_SET);
    rc = fread((char *)&cbuf[0], 512, 1, vdkdsk[drv]);
    if (rc == 0)
      return (0);
    cptr = &cbuf[2];
    i = 0;
    do {
      unam[0] = *(cptr + 0) + 256 * (*(cptr + 1));
      unam[1] = *(cptr + 2) + 256 * (*(cptr + 3));
      unam[2] = *(cptr + 4) + 256 * (*(cptr + 5));
      if ((unam[0] == rnam[0]) & (unam[1] == rnam[1]) & (unam[2] == rnam[2]))
        break;
      cptr += 12;
      i += 1;
    } while (i < 42);
    if (i == 42)
      seekto = 512 * (cbuf[0] + 256 * cbuf[1]);
  } while ((cbuf[0] + cbuf[1] != 0) && (i == 42));
  if (i == 42)
    return (0);

  // get info from UFD entry
  numlast = *(cptr + 8) + 256 * (*(cptr + 9));
  currentrec = *(cptr + 10) + 256 * (*(cptr + 11));

  // copy file to memory
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - copy %s into memory.\r\n", name);
    fflush(stderr);
  }
  seekto = 512 * currentrec;
  where = where_to_put_it;
  do {
    fseek(vdkdsk[drv], seekto, SEEK_SET);
    rc = fread((char *)&cbuf[0], 512, 1, vdkdsk[drv]);
    if (rc == 0)
      return (0);
    seekto = 512 * (cbuf[0] + 256 * cbuf[1]);
    if (seekto == 0)
      rc = numlast;
    else
      rc = 512;
    for (i = 2; i < rc; i++, where += 1)
      putAMbyte((uint8_t *)&cbuf[i], where);
  } while (seekto != 0);

  return (where - where_to_put_it); // returns size of module loaded
} // (or zero if problem...)

/*-------------------------------------------------------------------*/
/* config_start calls this to boot AMOS from virtual disk            */
/*-------------------------------------------------------------------*/
int vdkdvr_boot(long drv, char *monName, char *iniName, char *dvrName)
//
// returns true if it loaded the monitor, else false
//
{
  char *ii;
  int i, j, whatufd;
  uint16_t where;
  long wheretoputit;
  uint16_t monsize, dvrsize, membas, zsydsk;

  // make sure container opened...
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - make sure container open.\r\n");
    fflush(stderr);
  }
  if (vdkdsk[drv] == NULL)
    return (false);

  // ************ XXXLOD stuff - load the specified monitor ************
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot --- XXXLOD - load monitor.\r\n");
    fflush(stderr);
  }
  whatufd = 0x0104;
  wheretoputit = 0;
  if (strlen(monName) > 0)
    monsize = vdkload(drv, monName, whatufd, wheretoputit);
  else
    monsize = vdkload(drv, "system.mon", whatufd, wheretoputit);
  if (monsize == 0)
    return (false);

  // *********** MONGEN stuff - load the specified disk driver ***********
  if (strlen(dvrName) > 0) {
    if (am100_state.gPOST > 0) {
      fprintf(stderr, "POST - vdkboot --- MONGEN - load disk driver.\r\n");
      fflush(stderr);
    }
    getAMword((uint8_t *)&zsydsk, 0x88); // ZSYDSK = 0x88
    whatufd = 0x0106;
    wheretoputit = zsydsk;
    dvrsize = vdkload(drv, dvrName, whatufd, wheretoputit);
    if (dvrsize == 0)
      return (false);
    membas = zsydsk + dvrsize + 2;
    putAMword((uint8_t *)&membas, 0x46); // MEMBAS = 0x46
  }

  // ********* MONTST stuff - stuff in the specified INI file name *********
  if (strlen(iniName) > 0) {
    if (am100_state.gPOST > 0) {
      fprintf(stderr, "POST - vdkboot ---  MONTST - stuff ");
      fprintf(stderr, "'%s' text into monitor.\r\n", iniName);
      fflush(stderr);
    }
    getAMword((uint8_t *)&where, 2);
    where++;
    where++;
    ii = iniName;
    i = strlen(ii);
    for (j = 0; j < i; j++)
      putAMbyte((uint8_t *)ii++, where++);
    putAMbyte((uint8_t *)"\r", where++);
    putAMbyte((uint8_t *)"\n", where++);
  }

  // set PC to zero and disable interrupts
  am100_state.wd16_cpu_state->regs.PC = 0;
  am100_state.wd16_cpu_state->regs.PS.I2 = 0;

  // all done!
  if (am100_state.gPOST > 0) {
    fprintf(stderr, "POST - vdkboot - boot complete.\r\n\r\n");
    fflush(stderr);
  }
  return (true);
}
