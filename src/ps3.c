/* ps3.c         (c) Copyright Mike Noel, 2001-2008                  */
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

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This module presents a 3p+s hardware interface to the cpu,      */
/* and for the serial port presents a connection to the emulator     */
/* console.                  */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void PS3_Init(unsigned int port, // base port
              unsigned int fnum) // f(num) to bring up screen
{
  CARDS *cptr, *cp;
  WINDOW *theWindow;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  int numPS3, thePair;

  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("ps3.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_PS3;
  cptr->CType.PS3.port = port;
  cptr->CType.PS3.inptr = cptr->CType.PS3.inptr2 = cptr->CType.PS3.incnt =
      cptr->CType.PS3.outptr = cptr->CType.PS3.outptr2 =
          cptr->CType.PS3.outcnt = 0;

  /* Create the terminal window/panel */
  theWindow = newwin(24, 80, 0, 0);
  nodelay(theWindow, TRUE);

  thePanel = cptr->CType.PS3.thePanel = new_panel(theWindow);
  set_panel_userptr(thePanel, &(cptr->CType.PS3.panel_data));
  thePanelData = (PANEL_DATA *)panel_userptr(thePanel);
  thePanelData->PANEL_DATA_NEXT = panels;
  panels = thePanelData;
  thePanelData->thePanel = thePanel;
  thePanelData->hide = TRUE;
  thePanelData->fnum = fnum;
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
  cp = cards;
  numPS3 = 1;
  do {
    if (cp->C_Type == C_Type_PS3)
      numPS3++;
    if (cp->C_Type == C_Type_am300)
      numPS3++;
    cp = cp->CARDS_NEXT;
  } while (cp != NULL);
  if (numPS3 != 1)
    hide_panel(thePanel);
  else {
    thePanelData->hide = FALSE;
    show_panel_by_fnum(fnum);
  }
  update_panels();
  doupdate();

  /* register port(s) */
  regAMport(port + 0, &PS3_Port0, (unsigned char *)cptr); // control reg
  regAMport(port + 1, &PS3_Port1, (unsigned char *)cptr); // data    reg

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = cards;
  cards = cptr;
}

/*-------------------------------------------------------------------*/
/* Stop the board emulator                                           */
/*-------------------------------------------------------------------*/
void PS3_stop() {}

/*-------------------------------------------------------------------*/
/* Stop the board emulator                                           */
/*-------------------------------------------------------------------*/
void PS3_reset(unsigned char *sa) {
  CARDS *cptr;
  cptr = (CARDS *)sa;
}

/*-------------------------------------------------------------------*/
/* Poll the board emulator                                           */
/*-------------------------------------------------------------------*/
int PS3_poll(unsigned char *sa) {
  CARDS *cptr;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  WINDOW *theWindow;
  int ch, didsomething, did2 = 0, did3 = 0;
  unsigned char c;

  cptr = (CARDS *)sa;

  did2 = 0;
  thePanel = cptr->CType.PS3.thePanel;
  theWindow = panel_window(thePanel);
  thePanelData = panel_userptr(thePanel);
  do {
    didsomething = 0;
    ch = ERR;
    if (!thePanelData->hide)
      if (cptr->CType.PS3.incnt < 35)
        ch = InChars();
    if (ch == ERR)
      if (cptr->CType.PS3.incnt < 35)
        ch = telnet_InChars(thePanel);
    if (ch != ERR) {
      pthread_mutex_unlock(&bfrlock_t);
      cptr->CType.PS3.inbuf[cptr->CType.PS3.inptr2] = ch & 0x7f;
      cptr->CType.PS3.inptr2 = (cptr->CType.PS3.inptr2 + 1) & 0x7f;
      cptr->CType.PS3.incnt++;
      pthread_mutex_unlock(&bfrlock_t);
      didsomething++;
      did3++;
    }
    if (cptr->CType.PS3.outcnt > 0) {
      pthread_mutex_unlock(&bfrlock_t);
      c = cptr->CType.PS3.outbuf[cptr->CType.PS3.outptr2] & 0x7f;
      cptr->CType.PS3.outptr2 = (cptr->CType.PS3.outptr2 + 1) & 0x7f;
      cptr->CType.PS3.outcnt--;
      if (cptr->CType.PS3.outcnt == 0)
        cptr->CType.PS3.outptr = cptr->CType.PS3.outptr2 = 0;
      pthread_mutex_unlock(&bfrlock_t);
      OutChars(thePanel, (char *)&c);
      telnet_OutChars(thePanel, (char *)&c);
      didsomething++;
      did2++;
      did3++;
    }
  } while (didsomething > 0);
  if (did2 > 0)
    if (!thePanelData->hide) {
      wrefresh(theWindow);
      update_panels();
      doupdate();
    }

  return (did3);
}

/*-------------------------------------------------------------------*/
/* Port Handlers                                                     */
/*-------------------------------------------------------------------*/
void PS3_Port0(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //                              -------- control reg ---------------
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    *chr = 0;
    if (cptr->CType.PS3.incnt > 0)
      *chr |= 0x02;
    if (cptr->CType.PS3.outcnt < 127)
      *chr |= 0x01;
    break;

  case 1: /* write */
    break;

  default:
    assert("ps3.c - invalid rwflag arg to PS3_Port0...");
  }
}

void PS3_Port1(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //                              -------- data reg ---------------
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    if (cptr->CType.PS3.incnt > 0) {
      pthread_mutex_lock(&bfrlock_t);
      *chr = cptr->CType.PS3.inbuf[cptr->CType.PS3.inptr] & 0x7f;
      cptr->CType.PS3.inptr = (cptr->CType.PS3.inptr + 1) & 0x7f;
      cptr->CType.PS3.incnt--;
      if (cptr->CType.PS3.incnt == 0)
        cptr->CType.PS3.inptr = cptr->CType.PS3.inptr2 = 0;
      pthread_mutex_unlock(&bfrlock_t);
    } else
      *chr = 0xff;
    break;

  case 1: /* write */
    while (cptr->CType.PS3.outcnt > 126) {
      usleep(10000);
    }
    pthread_mutex_lock(&bfrlock_t);
    cptr->CType.PS3.outbuf[cptr->CType.PS3.outptr] = *chr & 0x7f;
    cptr->CType.PS3.outptr = (cptr->CType.PS3.outptr + 1) & 0x7f;
    cptr->CType.PS3.outcnt++;
    pthread_mutex_unlock(&bfrlock_t);
    break;

  default:
    assert("ps3.c - invalid rwflag arg to PS3_Port1...");
  }
}
