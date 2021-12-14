/* am300.c       (c) Copyright Mike Noel, 2001-2008                  */
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
#include "am300.h"
#include "memory.h"
#include "io.h"
#include "terms.h"
#include "telnet.h"

/*-------------------------------------------------------------------*/
/* This module presents an am300 hardware interface to the cpu,      */
/* and for each of the am300's 6 serial ports presents one of the    */
/* standard interfaces:  [[only CONSOLE is currently supported]]     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void am300_Init(unsigned int port,   // base port
                unsigned int intlvl, // interrupt level
                unsigned int fnum1, unsigned int fnum2, unsigned int fnum3,
                unsigned int fnum4, unsigned int fnum5, unsigned int fnum6) {
  CARDS *cptr, *cp;
  WINDOW *theWindow;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  int i, first = 0, fn[6], numterms, thePair;

  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("am300.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_am300;
  cptr->CType.AM300.port = port;
  cptr->CType.AM300.intlvl = intlvl;

  fn[0] = fnum1;
  fn[1] = fnum2;
  fn[2] = fnum3;
  fn[3] = fnum4;
  fn[4] = fnum5;
  fn[5] = fnum6;

  for (i = 0; i < 5; i++)
    cptr->CType.AM300.lastout[i] = 0;
  for (i = 0; i < 6; i++) {
    cptr->CType.AM300.inptr[i] = cptr->CType.AM300.inptr2[i] =
        cptr->CType.AM300.incnt[i] = cptr->CType.AM300.outptr[i] =
            cptr->CType.AM300.outptr2[i] = cptr->CType.AM300.outcnt[i] = 0;
    cptr->CType.AM300.IF[i] = 0;
    cptr->CType.AM300.thePanel[i] = NULL;
    if (fn[i] >= 0) {
      /* Create the terminal window/panel */
      theWindow = newwin(24, 80, 0, 0);
      nodelay(theWindow, TRUE);

      thePanel = cptr->CType.AM300.thePanel[i] = new_panel(theWindow);
      set_panel_userptr(thePanel, &(cptr->CType.AM300.panel_data[i]));
      thePanelData = (PANEL_DATA *)panel_userptr(thePanel);
      thePanelData->PANEL_DATA_NEXT = am100_state.panels;
      am100_state.panels = thePanelData;
      thePanelData->thePanel = thePanel;
      thePanelData->hide = TRUE;
      thePanelData->fnum = fn[i];
      thePanelData->theFG = COLOR_WHITE;
      thePanelData->theBG = COLOR_BLACK;
      thePanelData->sd = 0;
      thePanelData->telnet_lct = 0;
      thePanelData->teloutbufcnt = 0;
      thePanelData->telnet_ip[0] = 0;
      thePanelData->telnet_ip[1] = 0;
      thePanelData->telnet_ip[2] = 0;
      thePanelData->telnet_ip[3] = 0;
      thePair = 8 * thePanelData->theFG + thePanelData->theBG + 1;
      wbkgd(theWindow, COLOR_PAIR(thePair));

      /* if first panel show it, else leave hidden */
      cp = am100_state.cards;
      numterms = 1;
      do {
        if (cp->C_Type == C_Type_PS3)
          numterms++;
        if (cp->C_Type == C_Type_am300)
          numterms++;
        cp = cp->CARDS_NEXT;
      } while (cp != NULL);

      if ((numterms != 1) | (first > 0))
        hide_panel(thePanel);
      else {
        thePanelData->hide = FALSE;
        show_panel_by_fnum(fn[i]);
        first++;
      }
      update_panels();
      doupdate();
    }
  }

  /* register port(s) */
  regAMport(port + 0, &am300_Port0, (unsigned char *)cptr); // control reg 1
  regAMport(port + 1, &am300_Port1, (unsigned char *)cptr); // control reg 2
  regAMport(port + 2, &am300_Port2,
            (unsigned char *)cptr); // SYN-DLE & status reg
  regAMport(port + 3, &am300_Port3, (unsigned char *)cptr); // R/T holding reg
  regAMport(port + 4, &am300_Port4, (unsigned char *)cptr); // - MUX control reg

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = am100_state.cards;
  am100_state.cards = cptr;
}

/*-------------------------------------------------------------------*/
/* Terminate the AM300                                               */
/*-------------------------------------------------------------------*/
void am300_stop() {}

/*-------------------------------------------------------------------*/
/* Reset the AM300                                                   */
/*-------------------------------------------------------------------*/
void am300_reset(unsigned char *sa) {
  CARDS *cptr;
  cptr = (CARDS *)sa;
}

/*-------------------------------------------------------------------*/
/* routine to interrupt if one's waiting and has just been enabled   */
/*-------------------------------------------------------------------*/
void MaybeDoInterrupt(unsigned char *sa) {
  CARDS *cptr;
  int i, IE;
  cptr = (CARDS *)sa;

  IE = cptr->CType.AM300.lastout[4] & 16; // interrupt enable

  if (IE == 0)
    return; // card level interrupts disabled
  for (i = 0; i < 6; i++) {
    if (((cptr->CType.AM300.IF[i] & 2) > 0) &
        (cptr->CType.AM300.outcnt[i] < 127)) { // channel open & output rdy
      pthread_mutex_lock(&am100_state.wd16_cpu_state->intlock_t);
      am100_state.wd16_cpu_state->regs.whichint[cptr->CType.AM300.intlvl] = 1;
      am100_state.wd16_cpu_state->regs.intpending = 1;
      pthread_mutex_unlock(&am100_state.wd16_cpu_state->intlock_t);
      return; // quit after first setting it...
    }
    if (((cptr->CType.AM300.IF[i] & 4) > 0) &
        (cptr->CType.AM300.incnt[i] > 0)) { // channel open & input ready
      pthread_mutex_lock(&am100_state.wd16_cpu_state->intlock_t);
      am100_state.wd16_cpu_state->regs.whichint[cptr->CType.AM300.intlvl] = 1;
      am100_state.wd16_cpu_state->regs.intpending = 1;
      pthread_mutex_unlock(&am100_state.wd16_cpu_state->intlock_t);
      return; // don't need to set it more than once...
    }
  }
}

/*-------------------------------------------------------------------*/
/* Poll the AM300                                                    */
/*-------------------------------------------------------------------*/
int am300_poll(unsigned char *sa) {
  CARDS *cptr;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  WINDOW *theWindow;
  int ch, didsomething, did2, did3 = 0, i;
  unsigned char c;

  cptr = (CARDS *)sa;

  for (i = 0; i < 6; i++) {
    did2 = 0;
    thePanel = cptr->CType.AM300.thePanel[i];
    if (thePanel != NULL) {
      theWindow = panel_window(thePanel);
      thePanelData = panel_userptr(thePanel);
      do {
        didsomething = 0;
        ch = ERR;
        if (!thePanelData->hide)
          if (cptr->CType.AM300.incnt[i] < 35)
            ch = InChars();
        if (ch == ERR)
          if (cptr->CType.AM300.incnt[i] < 35)
            ch = telnet_InChars(thePanel);
        if (ch != ERR) {
          pthread_mutex_lock(&am100_state.bfrlock_t);
          cptr->CType.AM300.inbuf[cptr->CType.AM300.inptr2[i]][i] = ch & 0x7f;
          cptr->CType.AM300.inptr2[i] =
              (cptr->CType.AM300.inptr2[i] + 1) & 0x7f;
          cptr->CType.AM300.incnt[i]++;
          pthread_mutex_unlock(&am100_state.bfrlock_t);
          didsomething++;
          did3++;
        }
        if (cptr->CType.AM300.outcnt[i] > 0) {
          pthread_mutex_lock(&am100_state.bfrlock_t);
          c = cptr->CType.AM300.outbuf[cptr->CType.AM300.outptr2[i]][i] & 0x7f;
          cptr->CType.AM300.outptr2[i] =
              (cptr->CType.AM300.outptr2[i] + 1) & 0x7f;
          cptr->CType.AM300.outcnt[i]--;
          if (cptr->CType.AM300.outcnt[i] == 0)
            cptr->CType.AM300.outptr[i] = cptr->CType.AM300.outptr2[i] = 0;
          pthread_mutex_unlock(&am100_state.bfrlock_t);
          OutChars(thePanel, (char *)&c);
          telnet_OutChars(thePanel, (char *)&c);
          didsomething++;
          did2++;
          did3++;
        }
      } while (didsomething > 0);
      // ----- undate panel if changed and not hidden -----
      if (did2 > 0)
        if (!thePanelData->hide) {
          wrefresh(theWindow);
          update_panels();
          doupdate();
        }
      // ----- set interrupt if necessary -----
      MaybeDoInterrupt((unsigned char *)cptr);
    }
  }
  return (did3);
}

/*-------------------------------------------------------------------*/
/* Port Handlers                                                     */
/*-------------------------------------------------------------------*/

void am300_Port0(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int i, ip, MUX, BAUD, IE, RI;
  //                              -------- control reg 1 ---------------
  cptr = (CARDS *)sa;

  MUX = cptr->CType.AM300.lastout[4] & 7; // 0 = read int
  // 1-6 = select port
  // 7 = invalid
  BAUD = cptr->CType.AM300.lastout[4] & 8; // program baud rate
  IE = cptr->CType.AM300.lastout[4] & 16;  // interrupt enable
  RI = cptr->CType.AM300.lastout[4] & 32;  // read int bit, needs mux=0

  switch (MUX) {
  case 0: /* read int, write baud */
    switch (rwflag) {

    case 0: /* read int */
      if (RI == 0)
        assert("am300.c - MUX and RI both zero in am300_Port0...");
      for (i = 5, ip = 0; i >= 0; i--) {
        if (((cptr->CType.AM300.IF[i] & 2) > 0) &
            (cptr->CType.AM300.outcnt[i] < 127)) {
          ip = i + 1;
          ip = ip << 3;
        }
        if (((cptr->CType.AM300.IF[i] & 4) > 0) &
            (cptr->CType.AM300.incnt[i] > 0)) {
          ip = i + 1;
          ip = ip << 3;
          ip |= 4;
        }
      }
      *chr = ip;
      break;

    case 1: /* write baud */
      cptr->CType.AM300.lastout[0] = *chr;
      break;

    default:
      assert("am300.c - invalid rwflag arg to am300_Port0a...");
    }
    break;

  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6: /* select */
    switch (rwflag) {

    case 0: /* read */
      *chr = 0xff;
      break;

    case 1: /* write */
      cptr->CType.AM300.lastout[0] = *chr;
      cptr->CType.AM300.IF[MUX - 1] = *chr & 6;
      MaybeDoInterrupt(sa);
      break;

    default:
      assert("am300.c - invalid rwflag arg to am300_Port0b...");
    }
    break;

  default:
    assert("am300.c - invalid MUX in am300_Port0...");
  }
}

void am300_Port1(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int MUX, BAUD, IE, RI;
  //                              -------- control reg 2 ---------------
  cptr = (CARDS *)sa;

  MUX = cptr->CType.AM300.lastout[4] & 7; // 0 = read int
  // 1-6 = select port
  // 7 = invalid
  BAUD = cptr->CType.AM300.lastout[4] & 8; // program baud rate
  IE = cptr->CType.AM300.lastout[4] & 16;  // interrupt enable
  RI = cptr->CType.AM300.lastout[4] & 32;  // read int bit, needs mux=0

  switch (MUX) {

  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:

    switch (rwflag) {

    case 0: /* read */
      *chr = 0xff;
      break;

    case 1: /* write */
      cptr->CType.AM300.lastout[1] = *chr;
      break;

    default:
      assert("am300.c - invalid rwflag arg to am300_Port1...");
    }
    break;

  default:
    assert("am300.c - invalid MUX in am300_Port1...");
  }
}

void am300_Port2(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int MUX, BAUD, IE, RI;
  //                      ---- SYN-DLE (rcv) & status reg (trn)---------
  cptr = (CARDS *)sa;

  MUX = cptr->CType.AM300.lastout[4] & 7; // 0 = read int
  // 1-6 = select port
  // 7 = invalid
  BAUD = cptr->CType.AM300.lastout[4] & 8; // program baud rate
  IE = cptr->CType.AM300.lastout[4] & 16;  // interrupt enable
  RI = cptr->CType.AM300.lastout[4] & 32;  // read int bit, needs mux=0

  MUX--; // set MUX 0-5 instead of 1-6...

  switch (rwflag) {

  case 0: /* read */
    *chr = 0;
    if (cptr->CType.AM300.incnt[MUX] > 0)
      *chr |= 0x02;
    if (cptr->CType.AM300.outcnt[MUX] < 127)
      *chr |= 0x41;
    break;

  case 1: /* write */
    cptr->CType.AM300.lastout[2] = *chr;
    break;

  default:
    assert("am300.c - invalid rwflag arg to am300_Port2...");
  }
}

void am300_Port3(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int MUX, BAUD, IE, RI;
  //                      ---- holding register (trn & rcv) ------------
  cptr = (CARDS *)sa;

  MUX = cptr->CType.AM300.lastout[4] & 7; // 0 = read int
  // 1-6 = select port
  // 7 = invalid
  BAUD = cptr->CType.AM300.lastout[4] & 8; // program baud rate
  IE = cptr->CType.AM300.lastout[4] & 16;  // interrupt enable
  RI = cptr->CType.AM300.lastout[4] & 32;  // read int bit, needs mux=0

  MUX--; // set MUX 0-5 instead of 1-6...

  switch (rwflag) {

  case 0: /* read */
    if (cptr->CType.AM300.incnt[MUX] > 0) {
      pthread_mutex_lock(&am100_state.bfrlock_t);
      *chr = cptr->CType.AM300.inbuf[cptr->CType.AM300.inptr[MUX]][MUX] & 0x7f;
      cptr->CType.AM300.inptr[MUX] = (cptr->CType.AM300.inptr[MUX] + 1) & 0x7f;
      cptr->CType.AM300.incnt[MUX]--;
      if (cptr->CType.AM300.incnt[MUX] == 0)
        cptr->CType.AM300.inptr[MUX] = cptr->CType.AM300.inptr2[MUX] = 0;
      pthread_mutex_unlock(&am100_state.bfrlock_t);
    } else
      *chr = 0xff;
    break;

  case 1: /* write */
    if (cptr->CType.AM300.thePanel[MUX] != NULL) {
      while (cptr->CType.AM300.outcnt[MUX] > 126) {
        usleep(10000);
      }
      pthread_mutex_lock(&am100_state.bfrlock_t);
      cptr->CType.AM300.outbuf[cptr->CType.AM300.outptr[MUX]][MUX] =
          *chr & 0x7f;
      cptr->CType.AM300.outptr[MUX] =
          (cptr->CType.AM300.outptr[MUX] + 1) & 0x7f;
      cptr->CType.AM300.outcnt[MUX]++;
      pthread_mutex_unlock(&am100_state.bfrlock_t);
    }
    cptr->CType.AM300.lastout[3] = *chr;
    break;

  default:
    assert("am300.c - invalid rwflag arg to am300_Port3...");
  }
}

void am300_Port4(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;

  //                      ---- mux control register --------------------
  cptr = (CARDS *)sa;
  switch (rwflag) {

  case 0: /* read */
    *chr = 0xff;
    break;

  case 1: /* write */
    cptr->CType.AM300.lastout[4] = *chr;
    MaybeDoInterrupt(sa);
    break;

  default:
    assert("am300.c - invalid rwflag arg to am300_Port4...");
  }
}
