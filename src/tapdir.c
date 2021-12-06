/*-------------------------------------------------------------------*/
/* tapdir.c            (c) Copyright Mike Noel, 2001-2008            */
/*                                                                   */
/* Like the AMOS tapdir program, this one lists the contents         */
/* of tapfil/filtap backup tape, but from the host side looking      */
/* at the AWS file instead of from the guest (AMOS) side looking at  */
/* the tape drive.                                                   */
/*                                                                   */
/* Does not understand wildcards, just dumps whole listing...        */
/*                                                                   */
/* This is part of an emulator for the Alpha-Micro AM-100 computer.  */
/* It is copyright by Michael Noel and licensed for non-commercial   */
/* hobbyist use under terms of the "Q public license", an open       */
/* source certified license.  A copy of that license may be found    */
/* here:       http://www.otterway.com/am100/license.html            */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "tapfil.h"

void readT(void);
void GetDateTime(U8 *eightbytes, int *mm, int *dd, int *yy, int *HH, int *MM,
                 int *SS);

/*-------------------------------------------------------------------*/
/* mainline                                                          */
/*-------------------------------------------------------------------*/

void usage() {
  printf("Usage: tapdir [tapefilename]\n");
  exit(999);
}

int main(int argc, char *argv[]) {

  switch (argc) {
  case 1:
    tapefilename = "tape.aws";
    break;
  case 2:
    tapefilename = argv[1];
    break;
  default:
    usage();
  }

  readT();
}

/*-------------------------------------------------------------------*/
/* function to read and print headers on AMOS formated aws tape      */
/*-------------------------------------------------------------------*/

void readT() {
  uint16_t rdev, rnam1, rnam2, rext;
  int i, num, numblks, numlast, rc, ppn, p, pn;
  int mm, dd, yy, HH, MM, SS;
  int filenum = 0;
  long sumblks = 0;
  char C, dbuf[5], nbuf[8], ebuf[5];

  U8 buf[5000], *bbuf;

  // open the tape file
  filerw = 1; // make Read Only
  rc = open_awstape();
  if (rc != 0) {
    fprintf(stderr, "error opening tape file: %s\n", strerror(errno));
    exit(101);
  }

  // loop reading and printing headers
  do {
    // read the header
    rc = read_awstape((unsigned char *)&buf);
    if (rc < 0) {
      fprintf(stderr, "error reading tape file: %s\n", strerror(errno));
      exit(203);
    }

    // check for end (second tape mark where data expected)
    if (rc == 0) {
      fprintf(stdout, "Total of %d files in %ld blocks\n", filenum, sumblks);
      if (tapefile != NULL)
        run_awstape();
      return;
    }

    // extract data from the header

#define swpbyt(theword)                                                        \
  bbuf++;                                                                      \
  bbuf++;
#define norbyt(theword)                                                        \
  bbuf++;                                                                      \
  theword = *bbuf--;                                                           \
  theword *= 256;                                                              \
  theword |= *bbuf++;                                                          \
  bbuf++;

    bbuf = &buf[0];
    swpbyt(0xfffe); // 0  ?
    swpbyt(0xfd00); // 2  ?
    swpbyt(0x00fc); // 4  ?
    swpbyt(0xfb00); // 6  ?
    swpbyt(0xfaff); // 8  ?
    norbyt(rdev);   // A  dvc, r50, 031c='dsk'
    norbyt(num);    // C  drv, ie, dsk1:
    norbyt(rnam1);  // E  filename
    norbyt(rnam2);
    norbyt(rext);     // 12 extension
    norbyt(ppn);      // 14 p,pn
    norbyt(numblks);  // 16 blocks
    norbyt(numlast);  // 18 bytes in last block
    GetDateTime(bbuf, // 1A DATE (MMDDYY00) TIME (ticks)
                &mm, &dd, &yy, &HH, &MM, &SS);

    for (i = 0; i < 5; i++)
      dbuf[i] = '\0';
    for (i = 0; i < 8; i++)
      nbuf[i] = '\0';
    for (i = 0; i < 5; i++)
      ebuf[i] = '\0';

    unpack(&rdev, (unsigned char *)&dbuf[0]);
    unpack(&rnam1, (unsigned char *)&nbuf[0]);
    unpack(&rnam2, (unsigned char *)&nbuf[3]);
    unpack(&rext, (unsigned char *)&ebuf[0]);

    p = ppn >> 8;
    pn = ppn & 255;

    sumblks += numblks;
    C = 'L';
    if (numlast > 32767)
      C = 'C';
    filenum++;

    fprintf(stdout,
            "%5d   %3s%d: %6s %3s   %3o,%-3o %6d %c  %02d/%02d/%02d  "
            "%02d:%02d:%02d\n",
            filenum, &dbuf[0], num, &nbuf[0], &ebuf[0], p, pn, numblks, C, mm,
            dd, yy, HH, MM, SS);

    // fsf to position for next read
    while (fsb_awstape() > 0)
      ;

  } while (TRUE);
}

/*-------------------------------------------------------------------*/
/* obtain date/time for headers (assumes clkfrq 60)                  */
/*-------------------------------------------------------------------*/

void GetDateTime(U8 *eightbytes, int *mm, int *dd, int *yy, int *HH, int *MM,
                 int *SS) {
  long ticks;

  *mm = eightbytes[0];
  *dd = eightbytes[1];
  *yy = eightbytes[2];

  ticks = (eightbytes[7] << 24) + (eightbytes[6] << 16) + (eightbytes[5] << 8) +
          eightbytes[4];
  ticks /= 60; // clkfrq

  *HH = ticks / 3600;
  *MM = (ticks - (*HH * 3600)) / 60;
  *SS = ticks - (*HH * 3600) - (*MM * 60);
}
