/* terms.c         (c) Copyright Mike Noel, 2001-2008                */
/* ----------------------------------------------------------------- */
/*                                                                   */
/* This software is an emulator for the Alpha-Micro AM-100 computer. */
/* It is copyright by Michael Noel and licensed for non-commercial   */
/* hobbyist use under terms of the "Q public license", an open 	     */
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
#include "terms.h"
#include "dialog.h"
#include "front-panel.h"

/*-------------------------------------------------------------------*/
/* This module contains the console support (ncurses) routines.      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Routine to get the panel number of the un-hidden panel            */
/*-------------------------------------------------------------------*/
int get_panelnumber(void) {
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  int unhidden = -1;

  thePanelData = am100_state.panels;
  do {
    thePanel = thePanelData->thePanel;
    if (!thePanelData->hide)
      unhidden = thePanelData->fnum;
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);

  return unhidden;
}

/*-------------------------------------------------------------------*/
/* Routine to hide all the panels                                    */
/*-------------------------------------------------------------------*/
void hide_all_panels(void) {
  PANEL *thePanel;
  PANEL_DATA *thePanelData;

  thePanelData = am100_state.panels;
  do {
    thePanel = thePanelData->thePanel;
    thePanelData->hide = TRUE;
    hide_panel(thePanel);
    update_panels();
    doupdate();
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);
}

/*-------------------------------------------------------------------*/
/* Routine to show a panel (by fnum)                                 */
/*-------------------------------------------------------------------*/
void show_panel_by_fnum(int fnum) {
  PANEL *thePanel;
  WINDOW *theWindow;
  PANEL_DATA *thePanelData;

  hide_all_panels();

  thePanelData = am100_state.panels;
  do {
    thePanel = thePanelData->thePanel;
    theWindow = panel_window(thePanel);
    if (thePanelData->fnum == fnum) {
      thePanelData->hide = FALSE;
      show_panel(thePanel);
      update_panels();
      doupdate();
    }
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);
}

/*-------------------------------------------------------------------*/
/* Routine to refresh the current panels                             */
/*-------------------------------------------------------------------*/
void refresh_panel(void) {
  int fnum = 20;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;

  thePanelData = am100_state.panels;
  do {
    thePanel = thePanelData->thePanel;
    if (thePanelData->hide == FALSE)
      fnum = thePanelData->fnum;
    thePanelData = thePanelData->PANEL_DATA_NEXT;
  } while (thePanelData != NULL);

  if (fnum < 20) {
    show_panel_by_fnum(0);
    show_panel_by_fnum(fnum);
  }
}

/*-------------------------------------------------------------------*/
/* Routine to send output to a panel mapping ansi escapes to ncurses */
/*-------------------------------------------------------------------*/
void OutChars(PANEL *panel, char *chr) {
  PANEL_DATA *thePanelData;
  WINDOW *win;
  int row, col, theoutcnt, thePair, theFG, theBG;
  char thechar, *theoutbuf;

  thechar = *chr & 0x7f;

  thePanelData = panel_userptr(panel);
  win = panel_window(panel);

  theoutcnt = thePanelData->theoutcnt;
  theoutbuf = thePanelData->theoutbuf;

  theFG = thePanelData->theFG;
  theBG = thePanelData->theBG;

  switch (theoutcnt) {
  case 0: // ----- initial sequence detect! -----
    if (thechar == 0x1b) {
      theoutbuf[theoutcnt] = thechar;
      theoutcnt = 1;
      break;
    }
    switch (thechar) {
    case 0x07: // console bell
      if (thePanelData->hide == FALSE) {
        beep();
        flash();
      }
      break;
    case 0x08: // cursor left
      getyx(win, row, col);
      if (col > 0)
        col--;
      wmove(win, row, col);
      break;
    case 0x0a: // cursor down (aka linefeed)
      getyx(win, row, col);
      if (row < 23) {
        row++;
        wmove(win, row, col);
      } else {
        scrollok(win, TRUE);
        wscrl(win, 1);
        scrollok(win, FALSE);
      }
      break;
    case 0x0d: // carriage return
      getyx(win, row, col);
      col = 0;
      wmove(win, row, col);
      break;
    case 0x0e: // enable alt char set
      wattron(win, A_ALTCHARSET);
      break;
    case 0x0f: // enable std char set
      wattroff(win, A_ALTCHARSET);
      break;
    default:
      if (thechar < 0x20)
        break;
      waddch(win, thechar);
      getyx(win, row, col);
      if (col == 0) {
        col = 79;
        row--;
        wmove(win, row, col);
      }
    }
    break;
  case 1: // ----- there are no 2 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 2;
    if (theoutbuf[1] != '[') {
      theoutcnt = 0;
    }
    break;
  case 2: // ----- there are some 3 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 3;
    switch (thechar) {
    case 'A': // cursor up
      getyx(win, row, col);
      if (row > 0)
        row--;
      wmove(win, row, col);
      theoutcnt = 0;
      break;
    case 'C': // cursor right
      getyx(win, row, col);
      if (col < 79)
        col++;
      wmove(win, row, col);
      theoutcnt = 0;
      break;
    case 'H': // cursor home
      wmove(win, 0, 0);
      theoutcnt = 0;
      break;
    case 'J': // erase to end of screen
      getyx(win, row, col);
      if ((row == 0) && (col == 0)) {
        wattrset(win, A_NORMAL);
        theBG = COLOR_BLACK;
        theFG = COLOR_WHITE;
        thePair = 8 * theFG + theBG + 1;
        wattrset(win, COLOR_PAIR(thePair));
      }
      wclrtobot(win);
      theoutcnt = 0;
      break;
    case 'K': // erase to end of line
      wclrtoeol(win);
      theoutcnt = 0;
      break;
    case 'L': // line insert
      winsertln(win);
      theoutcnt = 0;
      break;
    case 'M': // line delete
      wdeleteln(win);
      theoutcnt = 0;
      break;
    case 'P': // character delete
      wdelch(win);
      theoutcnt = 0;
      break;
    case '@': // character insert
      winsch(win, ' ');
      theoutcnt = 0;
      break;
    }
    break;
  case 3: // ----- there are some 4 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 4;
    if (thechar == 'm')
      switch (theoutbuf[2]) {
      case '0': // all attributes off
        wattrset(win, A_NORMAL);
        theoutcnt = 0;
        break;
      case '1': // bold
        wattron(win, A_BOLD);
        theoutcnt = 0;
        break;
      case '4': // underline
        wattron(win, A_UNDERLINE);
        theoutcnt = 0;
        break;
      case '5': // blinking
        wattron(win, A_BLINK);
        theoutcnt = 0;
        break;
      case '7': // reverse
        wattron(win, A_REVERSE);
        theoutcnt = 0;
        break;
      case '8': // blank (non display)
        wattron(win, A_INVIS);
        theoutcnt = 0;
        break;
      }
    break;
  case 4: // ----- there are some 5 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 5;
    if (thechar == 'm') {
      if (theoutbuf[2] == '3')
        switch (theoutbuf[3]) {
        case '0': // black foreground
          theFG = COLOR_BLACK;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '1': // red foreground
          theFG = COLOR_RED;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '2': // green foreground
          theFG = COLOR_GREEN;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '3': // yellow foreground
          theFG = COLOR_YELLOW;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '4': // blue foreground
          theFG = COLOR_BLUE;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '5': // magenta foreground
          theFG = COLOR_MAGENTA;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '6': // cyan foreground
          theFG = COLOR_CYAN;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '7': // white foreground
          theFG = COLOR_WHITE;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        }
      else if (theoutbuf[2] == '4')
        switch (theoutbuf[3]) {
        case '0': // black background
          theBG = COLOR_BLACK;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '1': // red background
          theBG = COLOR_RED;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '2': // green background
          theBG = COLOR_GREEN;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '3': // yellow background
          theBG = COLOR_YELLOW;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '4': // blue background
          theBG = COLOR_BLUE;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '5': // magenta background
          theBG = COLOR_MAGENTA;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '6': // cyan background
          theBG = COLOR_CYAN;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        case '7': // white background
          theBG = COLOR_WHITE;
          thePair = 8 * theFG + theBG + 1;
          wattron(win, COLOR_PAIR(thePair));
          theoutcnt = 0;
          break;
        }
    }
    break;
  case 5: // ----- there are no 6 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 6;
    break;
  case 6: // ----- there are no 7 char sequences -----
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 7;
    break;
  case 7: // only valid 8 char sequence is cursor move
    theoutbuf[theoutcnt] = thechar;
    theoutcnt = 8;
    // ----- there are some 8 char sequences -----
    if (theoutbuf[7] == 'H') {
      theoutbuf[2] -= '0';
      theoutbuf[3] -= '0';
      theoutbuf[5] -= '0';
      theoutbuf[6] -= '0';
      row = 10 * theoutbuf[2] + theoutbuf[3] - 1;
      col = 10 * theoutbuf[5] + theoutbuf[6] - 1;
      wmove(win, row, col);
      theoutcnt = 0;
    }
    break;
  default:
    theoutcnt = 0;
  }

  thePanelData->theoutcnt = theoutcnt;
  thePanelData->theFG = theFG;
  thePanelData->theBG = theBG;
  return;
}

/*-------------------------------------------------------------------*/
/* Routine to get a string from key board                            */
/*-------------------------------------------------------------------*/
char *getns(char *str, int nn, WINDOW *win) {
  int c, done = FALSE, e, row, col;

  /* make sure dialog nature globally understood */
  am100_state.gDialog = TRUE; // caller will reset...

  /* assume cursor is located 1 past end of string */
  e = strlen(str);

  /* get and process response(s) */
  do {
    do {
      c = InChars();
    } while (c == ERR);
    if ((c > 0x1f) & (c < 0x7f) & (e < (nn - 1))) {
      str[e++] = c;
      waddch(win, c);
      wrefresh(win);
    }
    if (((c == 0x7f) | (c == 0x08)) & (e > 0)) {
      e--;
      getyx(win, row, col);
      col--;
      wmove(win, row, col);
      waddch(win, ' ');
      wmove(win, row, col);
      wrefresh(win);
    }
    str[e] = '\0';
    if (c == 0x1b) {
      str[0] = '\0';
      done = TRUE;
    }
    if (c == 0x0d) {
      done = TRUE;
    }
  } while (!done);

  return (str);
}

/*-------------------------------------------------------------------*/
/* Routine to get a character from key board, returns ERR if none    */
/* (will wait if requested...)                                       */
/*-------------------------------------------------------------------*/
int getxh(int wait) {
  int theChar;

  theChar = getch();
  if (theChar != ERR)
    return (theChar);
  if (wait > 0) {
    usleep(wait);
    theChar = getxh(0); // call self, no sleep...
    return (theChar);
  } else
    return (ERR);
}

/*-------------------------------------------------------------------*/
/* Routine to get curses input mapping ansi escapes                  */
/*                                                                   */
/* Curses keypad mapping not used due to need to map ALT keys, need  */
/* for native escape, and desire for reduced multi key seq waiting.  */
/*-------------------------------------------------------------------*/
int InChars(void) {
  int ch, ch2, ch3, ch4, altNum = 0;

  ch = getxh(0);

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
    ch2 = getxh(am100_state.gCHRSLP);
    switch (ch2) {
    case ERR: // <-- native ESC returned here
      break;

    case 'C':
    case 'c': // ALT C was quit
    case 'Q':
    case 'q': // ALT Q was also quit
      if (am100_state.gDialog)
        break;
      Dialog_OK("  alt-c, alt-q  to quit is depreciated\n"
                "  --->  use the front panel instead...\n");
      ch = ERR;
      break;

    case 'F':
    case 'f': // ALT F is front panel
    case '0': // ALT 0 (zero) also
      if (am100_state.gDialog)
        break;
      show_panel_by_fnum(0);
      update_panels();
      doupdate();
      FP_refresh();
      ch = ERR;
      break;

    case '9': // ALT 1-9 select terminal panel
      altNum++;
    case '8':
      altNum++;
    case '7':
      altNum++;
    case '6':
      altNum++;
    case '5':
      altNum++;
    case '4':
      altNum++;
    case '3':
      altNum++;
    case '2':
      altNum++;
    case '1':
      altNum++;
      if (am100_state.gDialog)
        break;
      show_panel_by_fnum(altNum);
      update_panels();
      doupdate();
      ch = ERR;
      break;

    case 'M':
    case 'm': // ALT M is mount/unmount device
      if (am100_state.gDialog)
        break;
      Dialog_Mount();
      ch = ERR;
      break;

    case 'R':
    case 'r': // ALT R is resume
      if (am100_state.gDialog)
        break;
      if (am100_state.wd16_cpu_state->regs.waiting) {
        fprintf(stderr, "\n\rYou are exiting single step mode.\n\r");
      }
      am100_state.wd16_cpu_state->regs.stepping = false;
      am100_state.wd16_cpu_state->regs.tracing = false;
      am100_state.wd16_cpu_state->regs.waiting = false;
      ch = ERR;
      break;

    case 'S':
    case 's': // ALT S is step
      if (am100_state.gDialog)
        break;
      if (!am100_state.wd16_cpu_state->regs.waiting) {
        fprintf(stderr, "\n\rYou have entered single step mode.  ");
        fprintf(stderr, "ALT-S to step.  ");
        fprintf(stderr, "ALT-R to resume..\n\r");
      }
      am100_state.wd16_cpu_state->regs.stepping = true;
      am100_state.wd16_cpu_state->regs.tracing = true;
      am100_state.wd16_cpu_state->regs.waiting = false;
      ch = ERR;
      break;

    case 'T':
    case 't': // ALT T is trace
      if (am100_state.gDialog)
        break;
      am100_state.wd16_cpu_state->regs.tracing = ~am100_state.wd16_cpu_state->regs.tracing;
      ch = ERR;
      break;

    case 'Z':
    case 'z': // ALT Z is repaint screen
      if (am100_state.gDialog)
        break;
      refresh_panel();
      ch = ERR;
      break;

    case '[': // [ opens a longer 3+ char sequence
      ch3 = getxh(am100_state.gCHRSLP);
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
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '2': // KEY_IC
        ch = 0x06;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '3': // KEY_DC
        ch = 0x04;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '4': // KEY_END
        ch = 0x05;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '5': // KEY_PPAGE
        ch = 0x12;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '6': // KEY_NPAGE
        ch = 0x14;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;

      case '7': // another possible KEY_HOME
        ch = 0x1e;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;
      case '8': // another possible KEY_END
        ch = 0x05;
        ch4 = getxh(am100_state.gCHRSLP);
        if (ch4 != '~')
          ch = ERR;
        break;

      default: // 3 or 3+ char seq unhandled
        ch = ERR;
        break;
      }
      break;

    default: // 2 char (ALT) unhandled
      if (am100_state.gDialog)
        break;
      Dialog_ALTHelp();
      ch = ERR;
      break;
    }
    break;

  default: // <-- normal characters returned here
    break;
  }

  return (ch);
}
