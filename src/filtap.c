/*-------------------------------------------------------------------*/
/* filtap.c            (c) Copyright Mike Noel, 2001-2008            */
/*                                                                   */
/* Like the AMOS filtap program, this one copies disk files (in the  */
/* windows or linux path) to tape (in this case an AWS tape file     */
/* also in the windows or linux filesystem).                         */
/*                                                                   */
/* But note that wildcarding is different.  The AMOS program uses    */
/* wildcard to specify a device|devnum:file.ext[p,pn] match but this */
/* program uses a directory path which is then expected to contain   */
/* folders named as dev|num: (ie "DSK0:") which in turn contain      */
/* folders named as ppn's (ie "[1,4]") which in turn contain files   */
/* ALL OF WHICH will be included.  Actually the enclosing dev|num    */
/* directories are optional; if not present (first level is one or   */
/* more [p,pn] directories) the assumed dev|num will be "XXX0:".     */
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
/* Future Enhancement: write list of files to intermediate file,     */
/*                     sort intermediate, write files to tape in     */
/*                     sorted order. '-s' option?                    */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "tapfil.h"

int traverse(char *path, int app);
void writeB(int app);
void writeT(char *dev, int num, int p, int pn, char *name, char *ext,
            char *path, int app);
void writeE(void);
void GetDateTime(U8 *eightbytes);

/*-------------------------------------------------------------------*/
/* mainline                                                          */
/*-------------------------------------------------------------------*/

void usage() {
  printf("Usage: filtap [-a] [directory] [tapefilename]\n");
  exit(999);
}

int main(int argc, char *argv[]) {
  char *directory;
  int app = FALSE;

  switch (argc) {
  case 1:
    directory = ".";
    tapefilename = "tape.aws";
    break;
  case 2:
    if (!strcmp(argv[1], "-a")) {
      app = TRUE;
      directory = ".";
    } else
      directory = argv[1];
    tapefilename = "tape.aws";
    break;
  case 3:
    if (!strcmp(argv[1], "-a")) {
      app = TRUE;
      directory = argv[2];
      tapefilename = "tape.aws";
    } else {
      directory = argv[1];
      tapefilename = argv[2];
    }
    break;
  case 4:
    if (!strcmp(argv[1], "-a"))
      app = TRUE;
    else
      usage();
    directory = argv[2];
    tapefilename = argv[3];
    break;
  default:
    usage();
  }

  traverse(directory, app);
  writeE();
}

/*-------------------------------------------------------------------*/
/* function to scan directory for file to write to tape              */
/*-------------------------------------------------------------------*/

int stridx(char *str, char *sub) {
  char *p;
  int i = 0;

  if ((p = strstr(str, sub)) != NULL)
    i = 1 + (int)(p - str);
  return i;
}

int parse_path(char *path, // full path including filename
               char *dev, int *num, int *p, int *pn, char *name, char *ext) {
  int i, j, k, l;

  // ----- preclear
  *p = *pn = -1;
  *num = 0;
  for (i = 0; i < 4; i++)
    ext[i] = '\0';
  for (i = 0; i < 7; i++)
    name[i] = '\0';
  dev[0] = dev[1] = dev[2] = 'X';
  dev[3] = '\0';

  // ----- first get name and extension
  for (i = strlen(path); ((i > 0) & (path[i] != '.') & (path[i] != '/')); i--)
    ;
  if (i == 0)
    return FALSE;            // no name?
  else if (path[i] == '.') { // name.ext (or at least .ext)
    for (j = i + 1, k = 0; k < 3; j++, k++)
      ext[k] = path[j];
    for (j = i - 1, k = 0; ((k < 6) & (path[j] != '/')); j--, k++) {
      for (l = 0; l < 6; l++)
        name[6 - l] = name[5 - l];
      name[0] = path[j];
    }
  } else if (path[i] == '/') { // name only, no ext
    for (j = i + 1, k = 0; k < 6; j++, k++)
      name[k] = path[j];
  }

  // ----- next get [p,pn]
  if (((i = stridx(path, "[")) > 0) & ((j = stridx(path, "]")) > 0))
    if (j > i)
      sscanf((char *)&path[i], "%d,%d", p, pn);
  if ((*p == -1) | (*pn == -1))
    return FALSE;

  // ----- finally get devname and devnumber
  i--; // point to '['
  i--; // point to '/'
  i--; // point to last char of dev/num
  for (j = i, k = 0; ((k < 6) & (path[j] != '/')); j--, k++)
    ;
  j++; // point to first char of dev/num
  if (k == 6)
    return TRUE; // just use default XXX0:
  if ((path[i] < '0') | (path[i] > '9'))
    return TRUE;
  if ((path[j] <= '9') & (path[j] >= '0'))
    return TRUE;
  dev[0] = dev[1] = dev[2] = dev[3] = '\0';
  for (k = 0; ((k < 3) & ((path[j] > '9') | (path[j] < '0'))); k++, j++)
    dev[k] = path[j];
  for (k = 0; ((k < 3) & (path[i] <= '9') & (path[i] >= '0')); k++, i--)
    ;
  for (k--, i++; k >= 0; k--, i++)
    *num = (*num * 10) + (int)(path[i] - '0');

  return TRUE;
}

void strip_trailing_slashes(char *path) {
  char *last = path + strlen(path) - 1;
  while (last >= path && *last == '/')
    *last-- = '\0';
}

int traverse(char *path, int app) {
  DIR *cdir;
  struct dirent *dirp;
  char *filepath;
  char dbuf[4], nbuf[7], ebuf[4];
  struct stat buf;
  int length;
  int num, p, pn;

  /* opening the directory */
  if ((cdir = opendir(path)) == NULL) {
    perror(path);
    return 1;
  }

  /* for every file in the directory */
  while ((dirp = readdir(cdir)) != NULL) {
    strip_trailing_slashes(path);

    length = strlen(path) + strlen(dirp->d_name) + 2;
    filepath = (char *)malloc(length);
    *filepath = '\0';
    strcat(strcat(strcat(filepath, path), "/"), dirp->d_name);

    if (lstat(filepath, &buf) == -1) {
      perror(filepath);
      continue;
    }

    if (S_ISREG(buf.st_mode)) {
      if (!parse_path(filepath, &dbuf[0], &num, &p, &pn, &nbuf[0], &ebuf[0]))
        continue;
      writeT(&dbuf[0], num, p, pn, &nbuf[0], &ebuf[0], filepath, app);
    } else if (S_ISDIR(buf.st_mode) && strcmp(dirp->d_name, ".") &&
               strcmp(dirp->d_name, "..")) {
      traverse(filepath, app);
    }

    free(filepath);
  }

  closedir(cdir);
  return 0;
}

/*-------------------------------------------------------------------*/
/* function to write file to AMOS formated aws tape                  */
/*-------------------------------------------------------------------*/

void writeB(int app) // Begin - open file and position
{
  int rc;
  U8 buf[5000];

  // open the tape file
  rc = open_awstape();
  if (rc != 0) {
    fprintf(stderr, "error opening tape file: %s\n", strerror(errno));
    exit(101);
  }

  // position tape for append between trailing pair of tape marks
  if (!app)
    return;
  while (read_awstape((unsigned char *)&buf) >= 0)
    ;
  bsb_awstape();
}

void writeT(char *dev, int num, int p, int pn, char *name, char *ext,
            char *path, int app) {
  uint16_t rdev, rnam1, rnam2, rext;
  int numblks, numlast, rc, i, j;
  long numbytes;
  FILE *disk;

  U8 buf[5000], *bbuf;

  // check for tape file open
  if (tapefile == NULL)
    writeB(app);
  if (tapefile == NULL) {
    fprintf(stderr, "%s did not open!\n", tapefilename);
    exit(201);
  }

  // open the disk file on this system...
  disk = fopen(path, "r");
  if (disk == NULL) {
    fprintf(stderr, "%s open error: %s\n", path, strerror(errno));
    exit(202);
  }

  // obtain disk file stats
  fseek(disk, 0L, SEEK_END);
  numbytes = ftell(disk);
  fseek(disk, 0L, SEEK_SET);
  numblks = numbytes / 510;
  numlast = numbytes % 510;
  if (numlast == 0) {
    if (numblks != 0)
      numlast = 510;
  } else
    numblks++;

  // make disk file r50 name
  pack(&rdev, (unsigned char *)dev);
  pack(&rnam1, (unsigned char *)name);
  if (strlen(name) > 2)
    pack(&rnam2, (unsigned char *)name + 3);
  else
    pack(&rnam2, (unsigned char *)"   ");
  pack(&rext, (unsigned char *)ext);

  // make p, pn octal (they come in decimal)
  sprintf((char *)&buf, "%d %d", p, pn);
  sscanf((char *)&buf, "%o %o", &p, &pn);

  // build the header
  memset(buf, 0, 512);
  bbuf = (U8 *)&buf[0];

#define swpbyt(theword)                                                        \
  *bbuf++ = (theword) >> 8;                                                    \
  *bbuf++ = (theword)&255;
#define norbyt(theword)                                                        \
  *bbuf++ = (theword)&255;                                                     \
  *bbuf++ = (theword) >> 8;

  swpbyt(0xfffe); // 0  ?
  swpbyt(0xfd00); // 2  ?
  swpbyt(0x00fc); // 4  ?
  swpbyt(0xfb00); // 6  ?
  swpbyt(0xfaff); // 8  ?
  norbyt(rdev);   // A  dvc, r50, 031c='dsk'
  norbyt(num);    // C  drv, ie, dsk1:
  norbyt(rnam1);  // E  filename
  norbyt(rnam2);
  norbyt(rext);         // 12 extension
  norbyt(p * 256 + pn); // 14 p,pn
  norbyt(numblks);      // 16 blocks
  norbyt(numlast);      // 18 bytes in last block
  GetDateTime(bbuf);    // 1A DATE (MMDDYY00) TIME (ticks)

  // write the header
  rc = write_awstape((unsigned char *)&buf, 512);
  if (rc != 0) {
    fprintf(stderr, "error writing tape file: %s\n", strerror(errno));
    exit(203);
  }

  // write the file, 4k blocks...
  for (i = 0, j = 0; i < (numblks - 1); i++) {
    memset(&buf[j], 0, 512);
    rc = fread((char *)&buf[j + 2], 510, 1, disk);
    if (rc == 0) {
      fprintf(stderr, "host file read error: %s\n", strerror(errno));
      exit(211);
    }
    j += 512;
    if (j == 4096) {
      rc = write_awstape((unsigned char *)&buf, 4096);
      if (rc != 0) {
        fprintf(stderr, "error writing tape file: %s\n", strerror(errno));
        exit(212);
      }
      j = 0;
    }
  }
  if (numlast > 0) {
    memset(&buf[j], 0, 512);
    rc = fread((char *)&buf[j + 2], numlast, 1, disk);
    if (rc == 0) {
      fprintf(stderr, "host file read error: %s\n", strerror(errno));
      exit(221);
    }
    j += numlast + 2;
    rc = write_awstape((unsigned char *)&buf, j);
    if (rc != 0) {
      fprintf(stderr, "error writing tape file: %s\n", strerror(errno));
      exit(222);
    }
  }

  // write a tapemark
  write_awsmark();

  // close the file
  fclose(disk);
}

void writeE(void) // End - close file
{
  if (tapefile == NULL)
    return;
  write_awsmark();
  run_awstape();
}

/*-------------------------------------------------------------------*/
/* obtain date/time for headers (assumes clkfrq 60)                  */
/*-------------------------------------------------------------------*/

void GetDateTime(U8 *eightbytes) {
  struct timeval tv;
  time_t tnow;
  struct tm *m;
  long lcladj;
  long long line_clock_ticks;

  gettimeofday(&tv, (struct timezone *)0);

  tnow = time((time_t *)0);
  m = localtime(&tnow);
  eightbytes[0] = (m->tm_mon + 1);
  eightbytes[1] = (m->tm_mday);
  eightbytes[2] = (m->tm_year - 100);
  eightbytes[3] = 0;

  lcladj = tv.tv_sec - (m->tm_hour * 3600) - (m->tm_min * 60) - (m->tm_sec);

  line_clock_ticks = tv.tv_sec - lcladj;
  line_clock_ticks *= 1000;
  line_clock_ticks += tv.tv_usec / 1000;
  line_clock_ticks *= 60; // clkfrq=60
  line_clock_ticks /= 1000;

  eightbytes[4] = line_clock_ticks & 0x0ff;
  eightbytes[5] = (line_clock_ticks >> 8) & 0x0ff;
  eightbytes[6] = (line_clock_ticks >> 16) & 0x0ff;
  eightbytes[7] = (line_clock_ticks >> 24) & 0x0ff;
}
