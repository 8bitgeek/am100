/* config.c       (c) Copyright Mike Noel, 2001-2008                 */
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
#include "config.h"
#include "am600.h"
#include "am300.h"
#include "am320.h"
#include "ps3.h"
#include "ram.h"
#include "rom.h"
#include "memory.h"
#include "io.h"
#include "priority.h"
#include "clock.h"
#include "telnet.h"
#include "front-panel.h"
#include "terms.h"
#include "hwassist.h"
#include "dialog.h"
#include <stdio.h>

#define SEPCHARS " ,="
#define SEPCHAR2 " ,=\"\'"

int cmd_echo = 0, did_am100 = 0, did_am300 = 0, did_am300_panel = 0,
    did_ps3 = 0, did_ps3_panel = 0, did_ram = 0;

int vdk_drv = 0;           /* boot drive over ride */
long vdk_bootaddr = -1;    /* boot address override       */
char vdk_monName[16] = ""; /* boot monitor name over ride */
char vdk_iniName[16] = ""; /* boot ini filename over ride */
char vdk_dvrName[16] = ""; /* boot dsk drv file over ride */

#define CBUFSIZE 4096
char cbuf[CBUFSIZE], *cbptr = &cbuf[0];


/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file am100 command.         */
/*-------------------------------------------------------------------*/
int config_am100_cmd() {
  int errcnt = 0;
  char *next;
  long CF = 60;

  next = strtok(cbuf, SEPCHARS); // bypass the 'am100'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "clkfrq") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      CF = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am100 clkfrq arguement invalid.\r\n");
        errcnt++;
      }
      if ((CF != 50) && (CF != 60)) {
        fprintf(stderr, "!warning! am100 clkfrq arguement %ld unusual.\r\n",
                CF);
      }
    } else if (strstr(next, "svcc") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&am100_state.wd16_cpu_state->cpu4_svcctxt, next, 10);
      if (am100_state.wd16_cpu_state->cpu4_svcctxt[0] == 'l')
        ;
      else if (am100_state.wd16_cpu_state->cpu4_svcctxt[0] == 'h')
        ;
      else {
        fprintf(stderr, "?am100 svcc value invalid.\r\n");
        errcnt++;
      }
    } else if (strstr(next, "bootaddr") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      vdk_bootaddr = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am100 bootaddr arguement invalid.\r\n");
        errcnt++;
      }
      if ((vdk_bootaddr < 0) || (vdk_bootaddr > 65534) ||
          ((vdk_bootaddr & 1) != 0)) {
        fprintf(stderr, "?am100 bootaddr arguement %ld invalid.\r\n",
                vdk_bootaddr);
        errcnt++;
      }
    } else if (strstr(next, "dsk") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      vdk_drv = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am100 dsk arguement invalid.\r\n");
        errcnt++;
      }
      if ((vdk_drv < 0) || (vdk_drv > 3)) {
        fprintf(stderr, "?am100 dsk arguement %d invalid.\r\n", vdk_drv);
        errcnt++;
      }
    } else if (strstr(next, "mon") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&vdk_monName, next, 12);
    } else if (strstr(next, "ini") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&vdk_iniName, next, 12);
    } else if (strstr(next, "dvr") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&vdk_dvrName, next, 12);
    } else {
      fprintf(stderr, "?am100 arguement %s invalid.\r\n", next);
      errcnt++;
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (did_am100 != 0) {
    fprintf(stderr, "?already processed an am100 line!.\r\n");
    return (1);
  }

  if (errcnt == 0) {
    clock_Init(CF);
    did_am100++;
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file echo command.          */
/*-------------------------------------------------------------------*/
int config_echo_cmd() {
  int errcnt = 0;
  char *next;
  int onoff = 0;

  next = strtok(cbuf, SEPCHARS); // bypass the 'echo'
  next = strtok(0, SEPCHARS);    // get next word
  if (strcmp(next, "on") == 0) {
    onoff = 1;
  } else if (strcmp(next, "off") == 0) {
    onoff = 0;
  } else
    errcnt++;
  next = strtok(0, SEPCHARS); // get next word
  if (next != 0) {
    fprintf(stderr, "!warning! echo extraneous input %s ignored.\r\n", next);
  }
  if (errcnt == 0) {
    cmd_echo = onoff;
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file pause command.         */
/*-------------------------------------------------------------------*/
int config_pause_cmd() {
  int errcnt = 0;
  char *next;
  long SECS;

  next = strtok(cbuf, SEPCHARS); // bypass the 'pause'
  next = strtok(0, SEPCHARS);    // get next word
  errno = 0;
  SECS = strtol(next, 0, 0);
  if (errno != 0) {
    fprintf(stderr, "?pause arguement invalid.\r\n");
    return (1);
  }
  if ((SECS < 1) || (SECS > 60)) {
    fprintf(stderr, "? pause arguement %ld unusual.\r\n", SECS);
  }
  next = strtok(0, SEPCHARS); // get next word
  if (next != 0) {
    fprintf(stderr, "!warning! pause extraneous input %s ignored.\r\n", next);
  }

  if (errcnt == 0) {
    sleep(SECS);
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file chrslp command.        */
/*-------------------------------------------------------------------*/
int config_chrslp_cmd() {
  int errcnt = 0;
  char *next;
  long uSECS;

  next = strtok(cbuf, SEPCHARS); // bypass the 'chrslp'
  next = strtok(0, SEPCHARS);    // get next word
  errno = 0;
  uSECS = strtol(next, 0, 0);
  if (errno != 0) {
    fprintf(stderr, "? chrslp arguement invalid.\r\n");
    return (1);
  }
  if ((uSECS < 100000) || (uSECS > 10000000)) {
    fprintf(stderr, "? chrslp arguement %ld unusual.\r\n", uSECS);
  }
  next = strtok(0, SEPCHARS); // get next word
  if (next != 0) {
    fprintf(stderr, "!warning! chrslp extraneous input %s ignored.\r\n", next);
  }

  if (errcnt == 0) {
    am100_state.gCHRSLP = uSECS;
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file telnet command.        */
/*-------------------------------------------------------------------*/
int config_telnet_cmd() {
  int errcnt = 0;
  char *next;
  long PORT = -1, DEFPORT = 11349;

  next = strtok(cbuf, SEPCHARS); // bypass the 'telnet'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?telnet port arguement invalid.\r\n");
        return (1);
      }
#if 0   
    if ((PORT < 1024) || (PORT > 32000))
#else
      if ((PORT < 23) ||
          (PORT > 32000)) /* [RMS]/2015 - Enable lower port numbers */
#endif
      {
        fprintf(stderr, "?telnet port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else {
      fprintf(stderr, "?telnet arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "!telnet PORT not set, default = %ld.\r\n", DEFPORT);
      PORT = DEFPORT;
    }
  }
  if (errcnt == 0) {
    telnet_Init1(PORT);
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file ps3 command.           */
/*-------------------------------------------------------------------*/
int config_ps3_cmd() {
  int errcnt = 0;
  char *next;
  long PORT = -1, FNUM = -1;

  next = strtok(cbuf, SEPCHARS); // bypass the 'ps3'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ps3 port arguement invalid.\r\n");
        return (1);
      }
      if ((PORT < 1) || (PORT > 254) || ((PORT & 1) != 0)) {
        fprintf(stderr, "?ps3 port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else if (strstr(next, "fnum") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ps3 fnum arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM < 1) || (FNUM > 9)) {
        fprintf(stderr, "?ps3 fnum arguement %ld invalid.\r\n", FNUM);
        return (1);
      }
    } else {
      fprintf(stderr, "?ps3 arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "?ps3 PORT not set.\r\n");
      errcnt++;
    }
    if (FNUM == -1) {
      fprintf(stderr, "?ps3 FNUM not set.\r\n");
      errcnt++;
    }
  }
  if (errcnt == 0) {
    PS3_Init(PORT, FNUM);
    if (did_ps3 == 0) {
      did_ps3 = PORT;
      did_ps3_panel = FNUM;
    }
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file am300 command.         */
/*-------------------------------------------------------------------*/
int config_am300_cmd() {
  int errcnt = 0;
  char *next;
  long PORT = -1, INTLVL = -1;
  long FNUM1 = -1, FNUM2 = -1, FNUM3 = -1, FNUM4 = -1, FNUM5 = -1, FNUM6 = -1;

  next = strtok(cbuf, SEPCHARS); // bypass the 'am300'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 port arguement invalid.\r\n");
        return (1);
      }
      if ((PORT < 1) || (PORT > 255) || ((PORT % 8) != 0)) {
        fprintf(stderr, "?am300 port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else if (strstr(next, "intlvl") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      INTLVL = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 intlvl arguement invalid.\r\n");
        return (1);
      }
      if ((INTLVL < 1) || (INTLVL > 9)) {
        fprintf(stderr, "?am300 intlvl arguement %ld invalid.\r\n", INTLVL);
        return (1);
      }
    } else if (strstr(next, "fnum1") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM1 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum1 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM1 < 1) || (FNUM1 > 9)) {
        fprintf(stderr, "?am300 fnum1 arguement %ld invalid.\r\n", FNUM1);
        return (1);
      }
    } else if (strstr(next, "fnum2") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM2 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum2 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM2 < 1) || (FNUM2 > 9)) {
        fprintf(stderr, "?am300 fnum2 arguement %ld invalid.\r\n", FNUM2);
        return (1);
      }
    } else if (strstr(next, "fnum3") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM3 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum3 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM3 < 1) || (FNUM3 > 9)) {
        fprintf(stderr, "?am300 fnum3 arguement %ld invalid.\r\n", FNUM3);
        return (1);
      }
    } else if (strstr(next, "fnum4") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM4 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum4 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM4 < 1) || (FNUM4 > 9)) {
        fprintf(stderr, "?am300 fnum4 arguement %ld invalid.\r\n", FNUM4);
        return (1);
      }
    } else if (strstr(next, "fnum5") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM5 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum5 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM5 < 1) || (FNUM5 > 9)) {
        fprintf(stderr, "?am300 fnum5 arguement %ld invalid.\r\n", FNUM5);
        return (1);
      }
    } else if (strstr(next, "fnum6") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FNUM6 = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am300 fnum6 arguement invalid.\r\n");
        return (1);
      }
      if ((FNUM6 < 1) || (FNUM6 > 9)) {
        fprintf(stderr, "?am300 fnum6 arguement %ld invalid.\r\n", FNUM6);
        return (1);
      }
    } else {
      fprintf(stderr, "?am300 arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "?am300 PORT not set.\r\n");
      errcnt++;
    }
  }
  if (errcnt == 0) {
    if (INTLVL == -1) {
      fprintf(stderr, "?am300 INTLVL not set.\r\n");
      errcnt++;
    }
  }
  if (errcnt == 0) {
    am300_Init(PORT, INTLVL, FNUM1, FNUM2, FNUM3, FNUM4, FNUM5, FNUM6);
    if (did_am300 == 0) {
      did_am300 = PORT;
      if ((FNUM1 > 0) & (FNUM1 < did_am300_panel))
        did_am300_panel = FNUM1;
      if ((FNUM2 > 0) & (FNUM2 < did_am300_panel))
        did_am300_panel = FNUM2;
      if ((FNUM3 > 0) & (FNUM3 < did_am300_panel))
        did_am300_panel = FNUM3;
      if ((FNUM4 > 0) & (FNUM4 < did_am300_panel))
        did_am300_panel = FNUM4;
      if ((FNUM5 > 0) & (FNUM5 < did_am300_panel))
        did_am300_panel = FNUM5;
      if ((FNUM6 > 0) & (FNUM6 < did_am300_panel))
        did_am300_panel = FNUM6;
    }
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file am320 command.         */
/*-------------------------------------------------------------------*/
int config_am320_cmd() {
  int errcnt = 0;
  char *next;
  long PORT = -1, FCNT = 30; // default 30 seconds to 'tclose' printer
  char filename[255];

  filename[0] = '\0';

  next = strtok(cbuf, SEPCHARS); // bypass the 'am320'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am320 port arguement invalid.\r\n");
        return (1);
      }
      if ((PORT < 1) || (PORT > 255)) {
        fprintf(stderr, "?am320 port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else if (strstr(next, "file") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&filename, next, 254);
    } else if (strstr(next, "delay") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FCNT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am320 delay arguement invalid.\r\n");
        return (1);
      }
      if ((FCNT < 1) || (FCNT > 530)) {
        fprintf(stderr, "?am320 delay arguement %ld invalid.\r\n", FCNT);
        return (1);
      }
    } else {
      fprintf(stderr, "?am320 arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "?am320 PORT not set.\r\n");
      errcnt++;
    }
  }

  if (errcnt == 0) {
    // below assumes clkfrq = 60...
    am320_Init(PORT, 60 * FCNT, (char *)&filename);
  }
  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file am600 command.         */
/*-------------------------------------------------------------------*/
int config_am600_cmd() {
  int errcnt = 0;
  char *next;
  long PORT = -1, FDMA = -1;
  char filename[255];
  FILE *fp;

  filename[0] = '\0';

  next = strtok(cbuf, SEPCHARS); // bypass the 'am600'
  next = strtok(0, SEPCHARS);    // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am600 port arguement invalid.\r\n");
        return (1);
      }
      if ((PORT < 1) || (PORT > 255)) {
        fprintf(stderr, "?am600 port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else if (strstr(next, "mtlvl") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      FDMA = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?am600 mtlvl arguement invalid.\r\n");
        return (1);
      }
      if ((FDMA < 1) || (FDMA > 15)) {
        fprintf(stderr, "?am600 mtlvl arguement %ld invalid.\r\n", FDMA);
        return (1);
      }
    } else if (strstr(next, "file") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&filename, next, 254);
      fp = fopen((char *)&filename, "r");
      if (fp == NULL)
        filename[0] = '\0';
      else
        fclose(fp);
    } else {
      fprintf(stderr, "?am600 arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "?am600 PORT not set.\r\n");
      errcnt++;
    }
    //  if (FDMA == -1) {
    //      fprintf(stderr, "?am600 mtlvl not set.\r\n");
    //      errcnt++;
    //      }
  }

  if (errcnt == 0) {
    am600_Init(PORT, FDMA, filename, 1);
  }
  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function (private) to process an .ini file ram or rom command.    */
/*-------------------------------------------------------------------*/
int config_ram_cmd() {
  int errcnt = 0, i;
  char *ramrom, *next;
  long PORT = -1, PINIT = -1, SIZE = -1, WHICHBASE, WORK;
  long BASE[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
  unsigned char IMAGE[255];
  FILE *fp;

  IMAGE[0] = 0;

  ramrom = strtok(cbuf, SEPCHARS); // bypass the 'ram/rom'
  next = strtok(0, SEPCHARS);      // get next word
  while (next != 0) {
    if (strstr(next, "port") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PORT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ram/rom port arguement invalid.\r\n");
        return (1);
      }
      if ((PORT < 1) || (PORT > 255)) {
        fprintf(stderr, "?ram/rom port arguement %ld invalid.\r\n", PORT);
        return (1);
      }
    } else if (strstr(next, "pinit") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      PINIT = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ram/rom pinit arguement invalid.\r\n");
        return (1);
      }
      if ((PINIT < 0) || (PINIT > 255)) {
        fprintf(stderr, "?ram/rom pinit arguement %ld invalid.\r\n", PINIT);
        return (1);
      }
    } else if (strstr(next, "size") != 0) {
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      SIZE = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ram/rom size arguement invalid.\r\n");
        return (1);
      }
      if ((SIZE < 2) || (SIZE > 512) || ((SIZE & 1) != 0)) {
        fprintf(stderr, "?ram/rom size arguement %ld invalid.\r\n", SIZE);
        return (1);
      }
    } else if (strncmp(next, "base", 4) == 0) {
      if ((next[4] < '0') || (next[4] > '7')) {
        fprintf(stderr, "?ram/rom %s arguement invalid.\r\n", next);
        return (1);
      }
      WHICHBASE = next[4] - '0';
      next = strtok(0, SEPCHARS); // get next word
      errno = 0;
      WORK = strtol(next, 0, 0);
      if (errno != 0) {
        fprintf(stderr, "?ram/rom base%ld arguement invalid.\r\n", WHICHBASE);
        return (1);
      }
      if ((WORK < 0) || (WORK > 65535)) {
        fprintf(stderr, "?ram/rom size arguement %ld invalid.\r\n", WORK);
        return (1);
      }
      BASE[WHICHBASE] = WORK;
    } else if (strstr(next, "image") != 0) {
      next = strtok(0, SEPCHAR2); // get next word
      strncpy((char *)&IMAGE, next, 254);
      if (strncmp(ramrom, "ram", 3) == 0) {
        fprintf(stderr, "?ram/rom image arguement invalid for ram.\r\n");
        return (1);
      }
      fp = fopen((char *)IMAGE, "r");
      if (fp == NULL) {
        fprintf(stderr, "?rom image %s not found.\r\n", IMAGE);
        errcnt++;
      } else
        fclose(fp);
    } else {
      fprintf(stderr, "?ram/rom arguement %s invalid.\r\n", next);
      return (1);
    }
    next = strtok(0, SEPCHARS); // get next word
  }

  if (errcnt == 0) {
    if (PORT == -1) {
      fprintf(stderr, "?ram/rom PORT not set.\r\n");
      errcnt++;
    }
    if (PINIT == -1) {
      fprintf(stderr, "?ram/rom PINIT not set.\r\n");
      errcnt++;
    }
    if (SIZE == -1) {
      fprintf(stderr, "?ram/rom SIZE not set.\r\n");
      errcnt++;
    }
    for (i = 0; i < 8; i++)
      if (BASE[i] == -1) {
        fprintf(stderr, "?ram/rom BASE%d not set.\r\n", i);
        errcnt++;
      }
  }

  if (errcnt == 0) {
    if (strncmp(ramrom, "ram", 3) == 0) {
      ram_Init(PORT, PINIT, 0, SIZE, BASE);
      did_ram++;
    } else
      rom_Init(PORT, PINIT, 0, SIZE, BASE, IMAGE);
  }

  return (errcnt);
}

/*-------------------------------------------------------------------*/
/* Function to build system configuration                            */
/*-------------------------------------------------------------------*/
int config_start(char *config_file_name) {
  FILE *cfile;
  char fbuf[255], *fptr;
  int bg, bootok, errcnt = 0, fg, i, j, k, l, m, pair, xmax, ymax;
  uint16_t JJ, KK;
  uint8_t jjj, kkk;
  long long check_ticks;
  int unhidden = -1;

  /* NCURSES - Initialize curses */
  initscr();
  getmaxyx(stdscr, ymax, xmax);
  if ((ymax < 24) || (xmax < 80)) {
    fprintf(stderr, "?MAX lines: %d\r\nMAX rows: %d\r\n\r\n", ymax, xmax);
    if ((ymax < 24)) {
      fprintf(stderr,
              "\r\nThis program requires at least 24 (prefer 25) lines\r\n");
    }
    if ((xmax < 80)) {
      fprintf(stderr, "\r\nThis program requires at least 80 rows\r\n");
    }
    sleep(3);
    return (false);
  }

  /* NCURSES - Initialize color */
  start_color();
  for (bg = 0; bg < 8; bg++)
    for (fg = 0; fg < 8; fg++) {
      pair = 8 * fg + bg + 1;
      init_pair(pair, fg, bg);
    }
  attron(COLOR_PAIR(8 * COLOR_WHITE + COLOR_BLACK + 1));

  /* NCURSES - Get keyboard into raw mode but let it map escape sequences */
  raw();
  noecho();
  nodelay(stdscr, TRUE);

  /* NCURSES - init my list of am100_state.panels */
  am100_state.panels = NULL;

  /* NCURSES - flash the copyright, then put up front panel */
  printw("\nVirtual Alpha Micro AM-100, version %s \n"
         "  (c) Copyright 2001-2008, Michael Noel, all rights reserved. \n"
         "  see LICENSE for allowed use. \n\n",
         MSTRING(VERSION));
  printw("  http://www.otterway.com/am100/license.html \n\n");
  printw("\nWARNING NOTICE \n"
         "  This software is licensed for non-commercial hobbyist use \n"
         "  only.  There exist known serious discrepancies between \n"
         "  it's functioning and that of a real AM-100, as well  \n"
         "  as between it and the WD-1600 manual describing the\n"
         "  functionality of a real AM-100, and even between it and\n"
         "  the comments in the code describing what it is intended\n"
         "  to do!  Use at your own risk. \n\n\n");
  refresh();
  sleep(3);
  FP_init();

  /* Initialize the multi-char wait time */
  // 1/3 default, except one full second for OSX
  // so the ALT sequences can be done manually...
#if defined(__APPLE_CC__)
  am100_state.gCHRSLP = 1000000;
#else
  am100_state.gCHRSLP = 300000;
#endif

  /* init am100_state.wd16_cpu_state->oldPCs table */
  for (am100_state.wd16_cpu_state->oldPCindex = 0; am100_state.wd16_cpu_state->oldPCindex < 256; am100_state.wd16_cpu_state->oldPCindex++)
    am100_state.wd16_cpu_state->oldPCs[am100_state.wd16_cpu_state->oldPCindex] = 0;

  /* Initialize card list */
  am100_state.cards = NULL;

  /* Initialize registers */
  memset(&am100_state.wd16_cpu_state->regs, 0, sizeof(REGS));
  am100_state.wd16_cpu_state->regs.gpr = (uint16_t *)&am100_state.wd16_cpu_state->regs.R0;
  am100_state.wd16_cpu_state->regs.spr = (int16_t *)&am100_state.wd16_cpu_state->regs.R0;
  am100_state.wd16_cpu_state->regs.waiting = 1;            // main (or POST) reset this...
  strcpy(am100_state.wd16_cpu_state->cpu4_svcctxt, "LOW"); // preset SVCC table low...

  /* Initialize memory structures */
  initAMmemory();

  /* Initialize port structures */
  initAMports();

  /* Initialize locks */
  pthread_attr_init(&am100_state.attR_t);
  pthread_mutex_init(&am100_state.wd16_cpu_state->intlock_t, NULL);
  pthread_mutex_init(&am100_state.bfrlock_t, NULL);
  pthread_mutex_init(&am100_state.clocklock_t, NULL);

  /* register the LED port */
  regAMport(0, &LEDPort, NULL);

  /*-------------------------------------------------------------------*/

  /* processs the configuration file */
  cfile = fopen(config_file_name, "r");
  if (cfile == NULL) {
    fprintf(stderr, "?configurtion file %s did not open.\r\n",
            config_file_name);
    return (false);
  }

  cbuf[0] = 0; // no command yet...
  while (!feof(cfile)) {
    fptr = &fbuf[0];
    *fptr = 0;
    fgets((char *)&fbuf[0], 255, cfile);
    l = strlen((char *)&fbuf[0]);
    if (cmd_echo == 1)
      fprintf(stderr, "%s", fbuf);

    // replace all tabs (and other controls) with spaces.
    for (i = 0; i <= l; i++)
      if ((fbuf[i] > 0) && (fbuf[i] < ' '))
        fbuf[i] = ' ';
      else
        fbuf[i] = tolower(fbuf[i]); // and make l.c.    <-----

    // trim trailing comment
    for (i = 0; i < l; i++)
      if (fbuf[i] == '#')
        fbuf[i] = 0;
    l = strlen((char *)&fbuf[0]);

    // trim trailing whitespace
    for (i = (l - 1), j = 0; ((i >= 0) & (j == 0)); i--)
      if (fbuf[i] == ' ')
        fbuf[i] = 0;
      else
        j++;
    l = strlen((char *)&fbuf[0]);

    // trim leading whitespace
    do {
      m = 0;
      for (i = 0, j = 0; ((i < l) & (j == 0)); i++)
        if (fbuf[i] == ' ') {
          for (k = i; k < l; k++)
            fbuf[k] = fbuf[k + 1];
          l--;
          m++;
          break;
        } else
          j++;
    } while (m > 0);
    l = strlen((char *)&fbuf[0]);

    // trim extra intermediate whitespace
    do {
      m = 0;
      for (i = 0; i < l; i++)
        if ((fbuf[i] == ' ') && (fbuf[i + 1] == ' ')) {
          for (k = i; k < l; k++)
            fbuf[k] = fbuf[k + 1];
          l--;
          m++;
          break;
        }
    } while (m > 0);
    l = strlen((char *)&fbuf[0]);

    // make sure we have something left!
    if (l == 0)
      continue;

    // append to multi-line command
    if ((strlen(cbuf) + strlen(fbuf)) > CBUFSIZE)
      assert("Config.c - cbuf overflow getting multi-line command.");
    strcat(cbuf, fbuf);
    if (fbuf[l - 1] == '\\') { // not complete yet...
      l = strlen(cbuf);
      cbuf[l - 1] = ' ';
    }

    else { // command is complete now...
      if (strstr(cbuf, "am100 ") != 0)
        errcnt += config_am100_cmd();
      else if (strstr(cbuf, "am300 ") != 0)
        errcnt += config_am300_cmd();
      else if (strstr(cbuf, "am320 ") != 0)
        errcnt += config_am320_cmd();
      else if (strstr(cbuf, "am600 ") != 0)
        errcnt += config_am600_cmd();
      else if (strstr(cbuf, "telnet ") != 0)
        errcnt += config_telnet_cmd();
      else if (strstr(cbuf, "echo ") != 0)
        errcnt += config_echo_cmd();
      else if (strstr(cbuf, "pause ") != 0)
        errcnt += config_pause_cmd();
      else if (strstr(cbuf, "chrslp ") != 0)
        errcnt += config_chrslp_cmd();
      else if (strstr(cbuf, "ps3 ") != 0)
        errcnt += config_ps3_cmd();
      else if (strstr(cbuf, "rom ") != 0)
        errcnt += config_ram_cmd();
      else if (strstr(cbuf, "ram ") != 0)
        errcnt += config_ram_cmd();
      else {
        // unknown configuration command
        fprintf(stderr, "?unknow configuration command\r\n");
        errcnt++;
      }
      cbuf[0] = 0;
    }
  }

  /* we reached EOF on ini file, any partial caommand left over? */
  if (strlen(cbuf) > 0) {
    fprintf(stderr, "?mulit-line initialization command aborted by eof\r\n");
    errcnt++;
  }

  /* did we get all the commands we need? */
  if ((did_am100 * (did_ps3 + did_am300) * did_ram) == 0) {
    fprintf(stderr,
            "? .ini file didn't define am100, ps3 or am300, and/or ram\r\n");
    errcnt++;
  }

  /* done with ini file */
  fclose(cfile);
  fprintf(stderr, "\r\n\r\n");
  if (errcnt != 0) {
    fprintf(stderr, "error in .ini file! \r\n");
    endwin();
    return (false);
  }

  /* do telnet phase 2 */
  telnet_Init2();

  /* get current unhidden panel id */
  unhidden = get_panelnumber();

  /* start the cpu */
  pthread_create(&am100_state.wd16_cpu_state->cpu_t, &am100_state.attR_t, (void *)&cpu_thread, NULL);
  usleep(100000); // let cpu thread get going...

  /* if enabled, run POST (power on self test) */
  if (am100_state.gPOST > 0) {

    // check for 32k ram at zero...
    fprintf(stderr,
            "POST - memtst - making sure at least 32k of memory is at 0\r\n");
    fflush(stderr);
    for (i = 0; i < 32; i++) {
      getAMword((unsigned char *)&JJ, i * 1024);
      JJ = ~JJ;
      putAMword((unsigned char *)&JJ, i * 1024);
      getAMword((unsigned char *)&KK, i * 1024);
      if (JJ != KK) {
        fprintf(stderr, "POST - memtst --- less than 32k!\r\n");
        return (false);
      }
    }

    // check to be sure clock is ticking...
    fprintf(stderr, "POST - clktst - making sure the clock ticks\r\n");
    fflush(stderr);
    check_ticks = am100_state.line_clock_ticks;
    usleep(100000); // sleep 1/10 second, s.b. lots of ticks!
    if (am100_state.line_clock_ticks == check_ticks) {
      fprintf(stderr, "POST - clktst --- clock isn't ticking!\r\n");
      return (false);
    }

    // check to be sure cpu runs...
    fprintf(stderr, "POST - cputst - making sure the cpu executes\r\n");
    fflush(stderr);
    am100_state.wd16_cpu_state->regs.PC = 0;
    am100_state.wd16_cpu_state->regs.PS.I2 = 0;
    putAMword((unsigned char *)&am100_state.wd16_cpu_state->regs.PC, 0); // put a NOP at 0
    putAMword((unsigned char *)&am100_state.wd16_cpu_state->regs.PC, 2); // put a NOP at 2
    putAMword((unsigned char *)&am100_state.wd16_cpu_state->regs.PC, 4); // put a NOP at 4
    am100_state.wd16_cpu_state->regs.stepping = 1;
    am100_state.wd16_cpu_state->regs.waiting = 0; // exec 1 instruction
    usleep(100000);   // sleep 1/10 second, (s.b. time to run 1 instruction!)
    if (am100_state.wd16_cpu_state->regs.PC != 2) {
      fprintf(stderr, "POST - cputst --- cpu isn't runnng!\r\n");
      return (false);
    }
    am100_state.wd16_cpu_state->regs.PC = 0;
    am100_state.wd16_cpu_state->regs.PS.I2 = 0;

    // check to be sure keyboard works...
    if (did_ps3 > 0) {
      fprintf(stderr, "POST - kbdtst - making sure the keyboard works\r\n");
      fprintf(stderr, "                (i/o to panel, not stderr)\r\n");
      fprintf(stderr, "                please press any key!!! \r\n");
      fflush(stderr);
      check_ticks = am100_state.line_clock_ticks + 5;
      getAMbyte((unsigned char *)&jjj, 0xFF00 + did_ps3); // get status
      if (am100_state.line_clock_ticks > check_ticks) {
        fprintf(stderr, "POST - kbdtst --- status check1 blocked(%lld)!\r\n",
                am100_state.line_clock_ticks - check_ticks + 5);
        return (false);
      }
      switch (jjj) {
      case 1:
      case 3:
        break;
      default:
        fprintf(stderr, "POST - kbdtst --- invalid status returned!\r\n");
        return (false);
      }
      fprintf(stderr, "                ");
      fflush(stderr);
      do {
        getAMbyte((unsigned char *)&kkk, 0xFF00 + did_ps3 + 1); // get data
        getAMbyte((unsigned char *)&jjj, 0xFF00 + did_ps3);     // get status
      } while (jjj > 1);
      fptr = "Please press any letter on keyboard ";
      do {
        putAMbyte((unsigned char *)fptr, 0xFF00 + did_ps3 + 1); // send data
        fptr++;
      } while (*fptr != '\0');
      do {
        getAMbyte((unsigned char *)&jjj, 0xFF00 + did_ps3); // get status
      } while (jjj != 3);
      do {
        getAMbyte((unsigned char *)&kkk, 0xFF00 + did_ps3 + 1); // get data
        getAMbyte((unsigned char *)&jjj, 0xFF00 + did_ps3);     // get status
      } while (jjj > 1);
    } else if (did_am300 > 0) {
      fprintf(stderr, "POST - kbdtst - making sure the keyboard works\r\n");
      fprintf(stderr, "                (POST does not support am300 yet)\r\n");
      fprintf(stderr,
              "                (Please define PS3 to run this test)\r\n");
      fflush(stderr);
    }
  }

  /* initialize vdk even if using bootaddr... */
  vdkdvr_start();

  /* boot from vdk unless prom boot address is specified */
  if (vdk_bootaddr < 0) { /* load mon at 0 */
    bootok = vdkdvr_boot(vdk_drv, vdk_monName, vdk_iniName, vdk_dvrName);
    if (!bootok) {
      fprintf(stderr, "?vdk boot failed! Problem with 'dsk0-container'?\r\n");
      sleep(3);
      return (false);
    }
  } else {
    if (am100_state.gPOST > 0) {
      fprintf(stderr,
              "POST - using vdk_bootaddr = %04lx instead of vdkdvr_boot\r\n",
              vdk_bootaddr);
      fflush(stderr);
    }
    am100_state.wd16_cpu_state->regs.PC = vdk_bootaddr;
  }

  /* set initial trace flag if that's what user wanted... */
  if (am100_state.gTRACE > 0) {
    am100_state.wd16_cpu_state->regs.tracing = true;
  }

  /* if POSTING delay a few seconds to see all results... */
  if (am100_state.gPOST > 0) {
    sleep(3);
  }

  show_panel_by_fnum(unhidden);
  return (true);
}

/*-------------------------------------------------------------------*/
/* Function to reboot                                                */
/*-------------------------------------------------------------------*/
void config_reset() {
  int bootok, ggPOST;

  am100_state.wd16_cpu_state->regs.PC = 0;
  am100_state.wd16_cpu_state->regs.PS.I2 = 0;
  am100_state.wd16_cpu_state->regs.waiting = true;
  memory_reset();
  io_reset();
  telnet_reset();
  vdkdvr_reset();

  ggPOST = am100_state.gPOST;
  am100_state.gPOST = 0; // no post stuff on subsequent boots

  if (vdk_bootaddr < 0) {
    bootok = vdkdvr_boot(vdk_drv, vdk_monName, vdk_iniName, vdk_dvrName);
    if (bootok)
      am100_state.wd16_cpu_state->regs.waiting = false;
    else {
      Dialog_OK("?vdk boot failed! Problem with 'dsk0-container'?");
    }
  } else {
    am100_state.wd16_cpu_state->regs.PC = vdk_bootaddr;
  }

  am100_state.gPOST = ggPOST;
}

/*-------------------------------------------------------------------*/
/* Function to load a file into memory                               */
/*-------------------------------------------------------------------*/
void config_fileload(unsigned char *filename, uint16_t where, uint16_t *fsize) {
  FILE *fp;
  unsigned char fbuf[5];
  int j;

  fp = fopen((char *)filename, "r");
  if (fp == NULL)
    assert("mongen.c - failed to open file");

  *fsize = j = 0;

  while (!feof(fp)) {
    fread(&fbuf[0], 1, 1, fp);
    if (!feof(fp)) {
      putAMbyte(&fbuf[0], where + j);
      j++;
      (*fsize)++;
    }
  }

  fclose(fp);
}

/*-------------------------------------------------------------------*/
/* Function to save a file from memory                               */
/*-------------------------------------------------------------------*/
void config_filesave(unsigned char *filename, uint16_t where, uint16_t *fsize) {
  FILE *fp;
  unsigned char fbuf[5];
  int j;

  fp = fopen((char *)filename, "w");
  if (fp == NULL)
    assert("mongen.c - failed to open file");

  j = 0;

  while (j < *fsize) {
    getAMbyte(&fbuf[0], where + j);
    j++;
    fwrite(&fbuf[0], 1, 1, fp);
  }

  fclose(fp);
}

/*-------------------------------------------------------------------*/
/* Function to write a human readable dump of memory                 */
/*-------------------------------------------------------------------*/
void config_memdump(uint16_t where, uint16_t fsize) {
  int i, whereend, fs, bl, buf = 0;
  char cp[4];
  /* RAD50 translation table  */
  char r50tbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$. 0123456789";

  for (fs = (where / 16) * 16, bl = fs, whereend = where + fsize; bl < whereend;
       bl += 16) {
    fprintf(stderr, "\r\n%04x: ", bl);
    for (i = 0; i < 16; i++, i++) {
      getAMword((unsigned char *)&buf, bl + i);
      fprintf(stderr, "%04x ", buf);
    }
    for (buf = 0, i = 0; i < 16; i++) {
      getAMbyte((unsigned char *)&buf, bl + i);
      if (!isprint(buf))
        buf = '.';
      fprintf(stderr, "%c", buf);
    }
    fprintf(stderr, " ");
    for (i = 0; i < 16; i++, i++) {
      getAMword((unsigned char *)&buf, bl + i);
      if (buf < 0xfa00) {
        cp[0] = r50tbl[buf / 03100];
        cp[1] = r50tbl[(buf / 050) % 050];
        cp[2] = r50tbl[buf % 050];
      } else {
        cp[0] = ' ';
        cp[1] = ' ';
        cp[2] = ' ';
      }
      cp[3] = 0;
      fprintf(stderr, "%s", cp);
    }
  }
  fprintf(stderr, "\r\n");
}

/*-------------------------------------------------------------------*/
/* Function to tear down system configuration                        */
/*-------------------------------------------------------------------*/
void config_stop() {
  cpu_stop();
  vdkdvr_stop();
  clock_stop();
  am320_stop();
  am600_stop();
  telnet_stop();
  io_stop();
  memory_stop();

  endwin();

} /* end function config_stop */
