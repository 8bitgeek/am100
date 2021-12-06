/*-------------------------------------------------------------------*/
/* tapfil.c            (c) Copyright Mike Noel, 2001-2008            */
/*                                                                   */
/* Like the AMOS tapfil program, this one copies disk files (in the  */
/* windows or linux path) from tape (in this case an AWS tape file   */
/* also in the windows or linux filesystem).                         */
/*                                                                   */
/* But note that wildcarding is different.  The AMOS program uses    */
/* wildcard to specify a device|devnum:file.ext[p,pn] match but this */
/* program uses a directory path which will then contain             */
/* folders named as dev|num: (ie "DSK0:") which in turn contain      */
/* folders named as ppn's (ie "[1,4]") which in turn contain         */
/* ALL OF THE FILES on the tape.                                     */
/*                                                                   */
/* Also note that AMOS random files will end up as sequential when   */
/* tapfil puts them into the above structure and will still be       */
/* sequential when filtap is run to send them back to AMOS.  This    */
/* will need to be handled manually, perhaps by running a program    */
/* like RSSR against the file(s) after restoring them to AMOS.       */
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

void readT(char *path);
void GetDateTime(U8 *eightbytes, int *mm, int *dd, int *yy, int *HH, int *MM,
                 int *SS);

/*-------------------------------------------------------------------*/
/* mainline                                                          */
/*-------------------------------------------------------------------*/

void usage() {
  printf("Usage: tapfil [directory] [tapefilename]\n");
  exit(999);
}

int main(int argc, char *argv[]) {
  char *directory;

  switch (argc) {
  case 1:
    directory = ".";
    tapefilename = "tape.aws";
    break;
  case 2:
    directory = argv[1];
    tapefilename = "tape.aws";
    break;
  case 3:
    directory = argv[1];
    tapefilename = argv[2];
    break;
  default:
    usage();
  }

  readT(directory);
}

/*-------------------------------------------------------------------*/
/* function to read AMOS formated aws tape                           */
/*-------------------------------------------------------------------*/

void readT(char *path) {
  uint16_t rdev, rnam1, rnam2, rext;
  int done, i, num, numblks, numlast, rc, rc2, ppn, p, pn;
  int mm, dd, yy, HH, MM, SS;
  int filenum = 0;
  long sumblks = 0;
  char C, dbuf[5], nbuf[8], ebuf[5];
  char fnbuf[300], *fn;
  char pxbuf[300], *px;
  FILE *disk;

  U8 buf[5000], *bbuf;

  // open the tape file
  filerw = 1; // make Read Only
  rc = open_awstape();
  if (rc != 0) {
    fprintf(stderr, "error opening tape file: %s\n", strerror(errno));
    exit(101);
  }

  // loop reading headers and writing associated files to disk
  do {
    // read the header
    rc = read_awstape((unsigned char *)&buf);
    if (rc < 0) {
      fprintf(stderr, "error reading tape file: %s\n", strerror(errno));
      exit(203);
    }

    // check for end (second tape mark where data expected)
    if (rc == 0) {
      fprintf(stdout, "Restored total of %d files in %ld blocks\n", filenum,
              sumblks);
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

    for (i = 5; ((i >= 0) & (nbuf[i] == ' ')); i--)
      nbuf[i] = '\0';

    p = ppn >> 8;
    pn = ppn & 255;

    sumblks += numblks;
    C = 'L';
    if (numlast > 32767)
      C = 'C';
    filenum++;

    fn = &fnbuf[0];
    fnbuf[0] = '\0';
    sprintf(fn, "%s/%s%d/[%o,%o]/%s.%s", path, &dbuf[0], num, p, pn, &nbuf[0],
            &ebuf[0]);
    fprintf(stdout, "%s\n", fn);

    // open the disk file on this system...
    disk = fopen(fn, "w");
    if (disk == NULL) {
      // if open fails try creating dirs on path...
      px = &pxbuf[0];
      pxbuf[0] = '\0';
      sprintf(px, "%s/%s%d", path, &dbuf[0], num);
      mkdir(px, 0755);
      sprintf(px, "%s/%s%d/[%o,%o]", path, &dbuf[0], num, p, pn);
      mkdir(px, 0755);
      // retry open after creating dirs...
      disk = fopen(fn, "w");
      if (disk == NULL) {
        fprintf(stderr, "%s open error: %s\n", fn, strerror(errno));
        exit(202);
      }
    }

    // dump tape file to disk
    done = FALSE;
    do {
      // read the block
      rc = read_awstape((unsigned char *)&buf);
      if (rc == 0)
        done = TRUE;
      if (rc < 0) {
        fprintf(stderr, "error reading tape file: %s\n", strerror(errno));
        exit(203);
      }

      // write data in blocks to disk file.
      // note: AMOS disk blocks are 'blocked' up to 4k
      //       and the first 2 bytes of each 512 byte record of LINKED
      //       files are junk and should not be written.  However all bytes
      //       of RANDOM files should be written. Linked = L, RANDOM = C.

      if (!done)
        for (i = 0; i < rc; i++) {
          if ((C == 'L') & ((i % 512) == 0)) {
            i++;
            i++;
          }
          rc2 = fwrite((char *)&buf[i], 1, 1, disk);
          if (rc2 != 1) {
            fprintf(stderr, "error writting disk file: %s\n", strerror(errno));
            exit(203);
          }
        }
    } while (!done);

    // close the disk file
    fclose(disk);

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
