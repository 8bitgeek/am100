/* telnet.c        (c) Copyright Mike Noel, 2001-2008                */
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
#include "telnet.h"

int get_telc(PANEL *panel, int wait);

/*-------------------------------------------------------------------*/
/* This module contains the telnet support routines.                 */
/*-------------------------------------------------------------------*/

int telnetport = 0; /* base port for telnet operations */
int QLEN;

int negotiate(int sd);

/*-------------------------------------------------------------------*/
/* Initialize (phase 1) telnet                                       */
/*  -- phase 1 just records the (base) port number                   */
/*-------------------------------------------------------------------*/
void telnet_Init1(unsigned int port) { telnetport = port; }

/*-------------------------------------------------------------------*/
/* Initialize (phase 2) telnet                                       */
/*  -- phase 2 is called after all ini file processing is done and   */
/*     sets up telnet for fnums 2-9 iff phase 1 got called...        */
/*-------------------------------------------------------------------*/
void telnet_Init2(void) {
  if (telnetport == 0)
    return;

  QLEN = 16; // fnum 2-9 times 2

  /* make a thread for telnet listener */
  pthread_create(&am100_state.telnet_t, &am100_state.attR_t, (void *)&telnet_thread, NULL);
  usleep(100000); // let the telnet thread get going
}

/*-------------------------------------------------------------------*/
/* Telnet Thread                                                     */
/*-------------------------------------------------------------------*/
void sig_pipe(int n) { fprintf(stderr, "Broken pipe signal\n"); }

void doit(int sd) {
  static uint8_t FAILNEG[] = {"Telnet parameter negotiation failed!\r\n"};
  static uint8_t GOODNEWS[] = {"Connected to VAM am-100 emulator!\r\n"};
  static uint8_t BADNEWS[] = {"Sorry, all VAM am-100 emulator port busy!\r\n"};

  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  int rc, theChar;

  /* Negotiate telnet parameters */
  rc = negotiate(sd);
  if (rc != 0) {
    send(sd, FAILNEG, sizeof(FAILNEG), 0);
    close(sd);
    return;
  }

  /* find a port for the connection... */
  thePanelData = am100_state.panels;
  do {
    if ((thePanelData->fnum > 1) && (thePanelData->sd == 0)) {
      send(sd, GOODNEWS, sizeof(GOODNEWS), 0);
      thePanelData->sd = sd;
      thePanelData->teloutbufcnt = 0;
      thePanelData->telnet_lct = am100_state.line_clock_ticks + (60 * 10);
      thePanel = thePanelData->thePanel;
      do {
        theChar = get_telc(thePanel, 0);
      } while (theChar != ERR);
      return;
    }
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);

  send(sd, BADNEWS, sizeof(BADNEWS), 0);
  close(sd);
  return;
}

void telnet_thread(void) {
  PANEL *thePanel;
  PANEL_DATA *thePanelData;

  struct sockaddr_in sad; /* structure to hold server's address */
  int sd;                 /* listening socket descriptor       */
  int port;               /* protocol port number              */
  int option_value;       /* needed for setsockopt             */

  struct sockaddr_in cad;          /* structure to hold client's address */
  unsigned int alen = sizeof(cad); /* length of address                 */
  int sd2;                         /* connected socket descriptor       */

  int fileflags;

  char c = 0;

  port = telnetport;

  /* clear and initialize server's sockaddr structure */
  memset((char *)&sad, 0, sizeof(sad));
  sad.sin_family = AF_INET;            /* set family to Internet */
  sad.sin_addr.s_addr = INADDR_ANY;    /* set the local IP address */
  sad.sin_port = htons((u_short)port); /* Set port */

  /* Create socket */
  if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "telnet.c - telnet_thread - socket creation failed\n");
    return; // thread dies!
  }

  /* Make listening socket's port reusable - must appear before bind */
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&option_value,
                 sizeof(option_value)) < 0) {
    fprintf(stderr, "telnet.c - telnet_thread - setsockopt failure\n");
    return; // thread dies!
  }

  /* Bind a local address to the socket */
  if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
    fprintf(stderr, "telnet.c - telnet_thread - bind failed\n");
    return; // thread dies!
  }

  /* Specify size of request queue */
  if (listen(sd, QLEN) < 0) {
    fprintf(stderr, "telnet.c - telnet_thread - listen failed\n");
    return; // thread dies!
  }

  /* Establish handling of the SIGPIPE signal */
  if (signal(SIGPIPE, sig_pipe) == SIG_ERR) {
    fprintf(stderr,
            "telnet.c - telnet_thread - Unable to set up signal handler\n");
    return; // thread dies!
  }

  /* set non-blocking */
  fileflags = fcntl(sd, F_GETFL, 0);
  fcntl(sd, F_SETFL, fileflags | FNDELAY);

  for (;;) {
    // snooze awhile
    usleep(100000);
    pthread_testcancel();

    // flush pending output and
    // detect any dropped connections by sending null
    thePanelData = am100_state.panels;
    do {
      if (thePanelData->sd != 0)
        if ((am100_state.line_clock_ticks > thePanelData->telnet_lct) ||
            (thePanelData->teloutbufcnt > 0)) {
          thePanel = thePanelData->thePanel;
          telnet_OutChars(thePanel, &c);
        }
      thePanelData = thePanelData->PANEL_DATA_NEXT;
    } while (thePanelData != NULL);

    // check for a new connection request
    if ((sd2 = accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN)
        continue;
      if (errno == EWOULDBLOCK)
        continue;
      fprintf(stderr, "telnet.c - telnet_thread - accept failed\n");
      return; // thread dies!
    }

    // fell thru, so try to hook up new user
    doit(sd2);
  }
}

/*-------------------------------------------------------------------*/
/* Terminate telnet                                                  */
/*-------------------------------------------------------------------*/
void telnet_stop(void) {
  PANEL_DATA *thePanelData;
  int s;

  if (telnetport == 0)
    return;

  /* close all telnet connections */
  thePanelData = am100_state.panels;
  do {
    s = thePanelData->sd;
    thePanelData->sd = 0;
    if (s != 0)
      close(s);
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);

  /* wait for telnet listener to die */
  pthread_cancel(am100_state.telnet_t);
  pthread_join(am100_state.telnet_t, NULL);
}

/*-------------------------------------------------------------------*/
/* Reset telnet                                                      */
/*-------------------------------------------------------------------*/
void telnet_reset(void) {

  if (telnetport == 0)
    return;
}

/*-------------------------------------------------------------------*/
/* Routine to send output to a telnet port selected by panel fnum    */
/* (buffers up to 30 chars, dumps buf when full or when chr = null)  */
/*-------------------------------------------------------------------*/
void telnet_OutChars(PANEL *panel, char *chr) {
  PANEL_DATA *thePanelData;
  int fnum, sd, rc;

  if (telnetport == 0)
    return;

  thePanelData = panel_userptr(panel);
  fnum = thePanelData->fnum;
  if ((fnum < 2) || (fnum > 9))
    return;
  sd = thePanelData->sd;
  if (sd == 0)
    return;

  thePanelData->teloutbuf[thePanelData->teloutbufcnt++] = *chr;
  if ((*chr == 0) & (thePanelData->teloutbufcnt > 1))
    thePanelData->teloutbufcnt--;
  if ((thePanelData->teloutbufcnt < 30) & (*chr != 0))
    return;

  rc = write(sd, thePanelData->teloutbuf, thePanelData->teloutbufcnt);
  if (rc < 1) {
    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
      ;
    else {
      close(sd);
      thePanelData->sd = 0;
      return;
    }
  }
  thePanelData->telnet_lct = am100_state.line_clock_ticks + (60 * 10);
  thePanelData->teloutbufcnt = 0;
  return;
}

#define CHRSLP 30000

/*-------------------------------------------------------------------*/
/* Routine to get a character from telnet, returns ERR if none       */
/* (will wait if requested...)                                       */
/*-------------------------------------------------------------------*/
int get_telc(PANEL *panel, int wait) {
  PANEL_DATA *thePanelData;
  int theChar, fnum, sd, rc;
  unsigned char c;

  if (telnetport == 0)
    return (ERR);

  thePanelData = panel_userptr(panel);
  fnum = thePanelData->fnum;
  if ((fnum < 2) || (fnum > 9))
    return (ERR);
  sd = thePanelData->sd;
  if (sd == 0)
    return (ERR);

  rc = read(sd, &c, 1);
  theChar = c;
  if (rc != 1)
    theChar = ERR;
  if (rc < 0) {
    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
      ;
    else {
      close(sd);
      thePanelData->sd = 0;
      return (ERR);
    }
  }
  if (theChar != ERR) {
    thePanelData->telnet_lct = am100_state.line_clock_ticks + (60 * 10);
    return (theChar);
  }
  if (wait > 0) {
    usleep(wait);
    theChar = get_telc(panel, 0); // call self, no sleep...
    return (theChar);
  } else
    return (ERR);
}

/*-------------------------------------------------------------------*/
/* Routine to get telnet input mapping ansi escapes                  */
/*-------------------------------------------------------------------*/
int telnet_InChars(PANEL *panel) {
  int ch, ch2, ch3, ch4;

  if (telnetport == 0)
    return (ERR);

  ch = get_telc(panel, 0);

  switch (ch) {
  case ERR: // <-- no char waiting returned here
    break;

  case 0x0a:
    ch = 0x0d; // remap LF to CR
    break;

  case 0x08: // remap BS to rubout
    ch = 0x7f;
    break;

  case 0x1b: // start escape sequence
    ch2 = get_telc(panel, CHRSLP);
    switch (ch2) {
    case ERR: // <-- native ESC returned here
      break;

    case '[': // [ opens a longer 3+ char sequence
      ch3 = get_telc(panel, CHRSLP);
      switch (ch3) {
      case ERR:
        break;

      case 'A': // KEY_UP
        ch = 0x0b;
        break;
      case 'B': // KEY_DOWN
        ch = 0x0a;
        break;
      case 'C': // KEY_RIGHT
        ch = 0x0c;
        break;
      case 'D': // KEY_LEFT
        ch = 0x08;
        break;

      case '1': // KEY_HOME
        ch = 0x1e;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '2': // KEY_IC
        ch = 0x06;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '3': // KEY_DC
        ch = 0x04;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '4': // KEY_END
        ch = 0x05;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '5': // KEY_PPAGE
        ch = 0x12;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '6': // KEY_NPAGE
        ch = 0x14;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;

      case '7': // another possible KEY_HOME
        ch = 0x1e;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '8': // another possible KEY_END
        ch = 0x05;
        ch4 = get_telc(panel, CHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;

      default: // 3 or 3+ char seq unhandled
        ch = ERR;
        break;
      }
      break;

    default: // 2 char (ALT) unhandled
      ch = ERR;
      break;
    }
    break;

  default: // <-- normal characters returned here
    break;
  }

  return (ch);
}

/*-------------------------------------------------------------------*/
/*                                                                   */
/* Telnet session negotiation stuff...                               */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Some Telnet command definitions                                   */
/*-------------------------------------------------------------------*/
#define BINARY 0         /* Binary Transmission */
#define IS 0             /* Used by terminal-type negotiation */
#define SEND 1           /* Used by terminal-type negotiation */
#define MODE 1           /* Used by linemode negotiation */
#define ECHO_OPTION 1    /* Echo option */
#define SUPPRESS_GA 3    /* Suppress go-ahead option */
#define TERMINAL_TYPE 24 /* Terminal type option */
#define LINEMODE 34      /* Linemode option */
#define SE 240           /* End of subnegotiation parameters */
#define NOP 241          /* No operation */
#define GA 249           /* Go ahead */
#define SB 250           /* Subnegotiation of indicated option */

#define WILL                                                                   \
  251 /* Indicates the desire to begin                                         \
         performing, or confirmation that                                      \
         you are now performing, the                                           \
         indicated option */
#define WONT                                                                   \
  252 /* Indicates the refusal to perform,                                     \
         or continue performing, the                                           \
         indicated option */
#define DO                                                                     \
  253 /* Indicates the request that the                                        \
         other party perform, or                                               \
         confirmation that you are expecting                                   \
         the other party to perform, the                                       \
         indicated option */
#define DONT                                                                   \
  254           /* Indicates the demand that the                               \
                   other party stop performing,                                \
                   or confirmation that you are no                             \
                   longer expecting the other party                            \
                   to perform, the indicated option */
#define IAC 255 /* Interpret as Command */

/*-------------------------------------------------------------------*/
/* SEND A 1-6 CHAR DATA PACKET TO THE CLIENT                   */
/*-------------------------------------------------------------------*/
int send_cmd(int sd, int len, char c0, char c1, char c2, char c3, char c4,
             char c5) {
  int rc; /* Return code               */
  char bb[6];

  bb[0] = c0;
  bb[1] = c1;
  bb[2] = c2;
  bb[3] = c3;
  bb[4] = c4;
  bb[5] = c5;

  rc = send(sd, &bb[0], len, 0);
  return rc;
}

/*-------------------------------------------------------------------*/
/* RECEIVE AND PROCESS A DATA PACKET FROM THE CLIENT IFF ONE WAITING */
/*-------------------------------------------------------------------*/
int get_packet_ifavail(int sd) {

#define TMAX 45

  unsigned char iac, cmd, cmdtype, se, is, ter, term[TMAX];
  int rc, t;

  /* get an IAC */
  do {
    pthread_testcancel();
    rc = read(sd, &iac, 1);
    if (rc < 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        return 0;
      else
        return rc;
    }
  } while (iac != IAC);

  /* get a DO, DONT, WILL, WONT, SB or SE */
  do {
    pthread_testcancel();
    rc = read(sd, &cmd, 1);
    if (rc < 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        ;
      else
        return rc;
    }
  } while (rc < 1);
  if (cmd == SE)
    return 0;

  /* get a cmd type */
  do {
    pthread_testcancel();
    rc = read(sd, &cmdtype, 1);
    if (rc < 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        ;
      else
        return rc;
    }
  } while (rc < 1);

  /* process the command */
  switch (cmd) {
  case DONT:
  case WONT:
    // (cmdtype == TERMINAL_TYPE) return -1; // if he dont-wont something
    // (cmdtype == ECHO_OPTION) return -1;   // I need he's toast...
    // (cmdtype == SUPPRESS_GA) return -1;   // ** or at least I should     **
    // (cmdtype == LINEMODE) return -1;      // ** but today I'm a nice guy **
    break;
  case WILL:
    if (cmdtype == TERMINAL_TYPE) {
      rc = send_cmd(sd, 6, IAC, SB, TERMINAL_TYPE, SEND, IAC, SE);
      if (rc < 0)
        return -1;
    } // ignore echo, line, sga
    if (cmdtype == ECHO_OPTION)
      break; // ('cause we already sent)
    if (cmdtype == SUPPRESS_GA)
      break;
    if (cmdtype == LINEMODE)
      break; // refuse others
    rc = send_cmd(sd, 3, IAC, DONT, cmdtype, 0, 0, 0);
    if (rc < 0)
      return -1;
    break;
  case DO:
    if (cmdtype == TERMINAL_TYPE)
      break; // ignore term, echo, line, sga
    if (cmdtype == ECHO_OPTION)
      break; // ('cause we already sent)
    if (cmdtype == SUPPRESS_GA)
      break;
    if (cmdtype == LINEMODE)
      break; // refuse others
    rc = send_cmd(sd, 3, IAC, WONT, cmdtype, 0, 0, 0);
    if (rc < 0)
      return -1;
    break;
  case SB:
    if (cmdtype != TERMINAL_TYPE)
      break; // only sub negotiate term...

    /* get a IS */
    do {
      pthread_testcancel();
      rc = read(sd, &is, 1);
      if (rc < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
          ;
        else
          return rc;
      }
    } while (rc < 1);
    if (is != IS)
      return 0;

    /* collect terminal type (and trailing IAC) */
    term[0] = '\0';
    t = 0;
    do {
      do {
        pthread_testcancel();
        rc = read(sd, &ter, 1);
        if (rc < 0) {
          if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            ;
          else
            return rc;
        }
      } while (rc < 1);
      if (ter != IAC) {
        if (t < TMAX) {
          term[t++] = ter;
          term[t] = '\0';
        } else
          return 0;
      }
    } while (ter != IAC);

    /* get the SE */
    do {
      pthread_testcancel();
      rc = read(sd, &se, 1);
      if (rc < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
          ;
        else
          return rc;
      }
    } while (rc < 1);
    if (se != SE)
      return 0;

    /* if we like this terminal type do nothing, else get another */
    if ((memcmp(term, "ANSI", 4) == 0) || (memcmp(term, "XTERM", 5) == 0) ||
        (memcmp(term, "VT100", 5) == 0) ||
        (memcmp(term, "DEC-VT100", 9) == 0) || (memcmp(term, "VTNT", 4) == 0) ||
        (memcmp(term, "CYGWIN", 6) == 0)) {
      //        fprintf(stderr, "%s accepted\r\n", term);
      return 999;
    } else {
      //        fprintf(stderr, "%s rejected\r\n", term);
      rc = send_cmd(sd, 6, IAC, SB, TERMINAL_TYPE, SEND, IAC, SE);
      if (rc < 0)
        return -1;
    }
  }

  return 0;
}

/*-------------------------------------------------------------------*/
/* NEGOTIATE TELNET PARAMETERS                                       */
/*                                                                   */
/* we want an ANSI-like terminal, char at a time, with remote echo.  */
/*-------------------------------------------------------------------*/
int negotiate(int sd) {
  long long lct;
  int rc;

  /* set non-blocking */
  rc = fcntl(sd, F_GETFL, 0);
  fcntl(sd, F_SETFL, rc | FNDELAY);

  /* send out my command prompts */
  rc = send_cmd(sd, 3, IAC, WILL, ECHO_OPTION, 0, 0, 0);
  if (rc < 0)
    return -1;
  rc = send_cmd(sd, 3, IAC, DO, LINEMODE, 0, 0, 0);
  if (rc < 0)
    return -1;
  rc = send_cmd(sd, 3, IAC, WILL, SUPPRESS_GA, 0, 0, 0);
  if (rc < 0)
    return -1;
  rc = send_cmd(sd, 3, IAC, DO, SUPPRESS_GA, 0, 0, 0);
  if (rc < 0)
    return -1;
  rc = send_cmd(sd, 3, IAC, DO, TERMINAL_TYPE, 0, 0, 0);
  if (rc < 0)
    return -1;

  /* deal with his prompts and replys -         */
  /* loop until (1) no more replys, and         */
  /*            (2) term neg complete, or       */
  /*            (3) time out occurs.            */
  lct = am100_state.line_clock_ticks;
  lct += 60 * 5; // 5 seconds to do negotiations...
  do {
    rc = get_packet_ifavail(sd);
    if (rc < 0)
      return -1;
    if (rc == 999)
      return 0;
    usleep(1000);
  } while (lct > am100_state.line_clock_ticks);

  return 0;
}
