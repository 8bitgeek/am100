/* front-panel.c    (c) Copyright Mike Noel, 2001-2008               */
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
#include "front-panel.h"
#include "dialog.h"

WINDOW *FPWindow;
WINDOW *am_subwin, *vam_subwin;
WINDOW *onoff_subwin, *reset_subwin, *led_subwin;

PANEL *FPPanel;
PANEL_DATA FPPanel_data;

int FP$power, FP$reset;

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This module draws the 'front panel' of the emulator on the        */
/* ncurses stdsrc (under all the am100_state.panels).                            */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the front panel                                        */
/*-------------------------------------------------------------------*/
void FP_init() {
  int theFG, theBG, thePair;
  PANEL_DATA *thePanelData;

  /* the main FP window */
  FPWindow = newwin(24, 80, 0, 0);
  FPPanel = new_panel(FPWindow);
  set_panel_userptr(FPPanel, &FPPanel_data);
  thePanelData = (PANEL_DATA *)panel_userptr(FPPanel);
  thePanelData->PANEL_DATA_NEXT = am100_state.panels;
  am100_state.panels = thePanelData;
  thePanelData->thePanel = FPPanel;
  thePanelData->hide = FALSE;
  thePanelData->fnum = 0;
  theFG = thePanelData->theFG = COLOR_BLACK;
  theBG = thePanelData->theBG = COLOR_YELLOW;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(FPWindow, COLOR_PAIR(thePair));
  wclear(FPWindow);
  box(FPWindow, ACS_VLINE, ACS_HLINE);

  /* the 'AlphaMicro' decoration (sub)window */
  am_subwin = subwin(FPWindow, 8, 16, 1, 1);
  theFG = COLOR_RED;
  theBG = COLOR_BLACK;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(am_subwin, COLOR_PAIR(thePair));
  wclear(am_subwin);
  theFG = COLOR_GREEN;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wmove(am_subwin, 0, 0);
  wprintw(am_subwin, "                ");
  wmove(am_subwin, 1, 0);
  wprintw(am_subwin, " V");

  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wprintw(am_subwin, "irtual        ");

  theFG = COLOR_BLUE;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wmove(am_subwin, 2, 0);
  wprintw(am_subwin, " A");

  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wprintw(am_subwin, "lpha-");

  theFG = COLOR_RED;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wprintw(am_subwin, "M");

  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(am_subwin, COLOR_PAIR(thePair));
  wprintw(am_subwin, "icro    ");

  wmove(am_subwin, 3, 0);
  wprintw(am_subwin, " AM100 emulator ");
  wmove(am_subwin, 4, 0);
  wprintw(am_subwin, "                ");
  wmove(am_subwin, 5, 0);
  wprintw(am_subwin, "                ");
  wmove(am_subwin, 5, 0);
  wprintw(am_subwin, " version %s", MSTRING(VERSION));
  wmove(am_subwin, 6, 0);
  wprintw(am_subwin, "                ");
  wmove(am_subwin, 7, 0);
  wprintw(am_subwin, "                ");

  /* the 'VAM' decoration (sub)window */
  vam_subwin = subwin(FPWindow, 15, 16, 8, 1);
  theFG = COLOR_YELLOW;
  theBG = COLOR_BLACK;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(vam_subwin, COLOR_PAIR(thePair));
  wclear(vam_subwin);
  theFG = COLOR_GREEN;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(vam_subwin, COLOR_PAIR(thePair));
  wmove(vam_subwin, 0, 0);
  wprintw(vam_subwin, "                ");
  wmove(vam_subwin, 1, 0);
  wprintw(vam_subwin, " v   v          ");
  wmove(vam_subwin, 2, 0);
  wprintw(vam_subwin, " v   v          ");
  wmove(vam_subwin, 3, 0);
  wprintw(vam_subwin, " v   v          ");
  wmove(vam_subwin, 4, 0);
  wprintw(vam_subwin, "  v v           ");
  wmove(vam_subwin, 5, 0);
  wprintw(vam_subwin, "   v            ");
  wmove(vam_subwin, 6, 0);
  wprintw(vam_subwin, "   v            ");
  theFG = COLOR_BLUE;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(vam_subwin, COLOR_PAIR(thePair));
  wmove(vam_subwin, 4, 6);
  wprintw(vam_subwin, "a         ");
  wmove(vam_subwin, 5, 6);
  wprintw(vam_subwin, "a         ");
  wmove(vam_subwin, 6, 0);
  wprintw(vam_subwin, "     a a        ");
  wmove(vam_subwin, 7, 0);
  wprintw(vam_subwin, "     a a        ");
  wmove(vam_subwin, 8, 0);
  wprintw(vam_subwin, "    aaaaa       ");
  wmove(vam_subwin, 9, 0);
  wprintw(vam_subwin, "    a   a       ");
  theFG = COLOR_RED;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wattron(vam_subwin, COLOR_PAIR(thePair));
  wmove(vam_subwin, 9, 10);
  wprintw(vam_subwin, "m   m ");
  wmove(vam_subwin, 10, 0);
  wprintw(vam_subwin, "          mm mm ");
  wmove(vam_subwin, 11, 0);
  wprintw(vam_subwin, "          m m m ");
  wmove(vam_subwin, 12, 0);
  wprintw(vam_subwin, "          m   m ");
  wmove(vam_subwin, 13, 0);
  wprintw(vam_subwin, "          m   m ");
  wmove(vam_subwin, 14, 0);
  wprintw(vam_subwin, "                ");

  /* the LED (sub)window */
  wmove(FPWindow, 2, 56);
  wprintw(FPWindow, "Diagnostic LED");
  led_subwin = subwin(FPWindow, 1, 2, 2, 75);
  theFG = COLOR_RED;
  theBG = COLOR_BLACK;
  thePair = 8 * theFG + theBG + 1;
  wattron(led_subwin, COLOR_PAIR(thePair));
  FP_led();

  /* the Reset (sub)window */
  wmove(FPWindow, 16, 56);
  wprintw(FPWindow, "Reset");
  reset_subwin = subwin(FPWindow, 2, 4, 16, 73);
  theFG = COLOR_BLACK;
  theBG = COLOR_GREEN;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(reset_subwin, COLOR_PAIR(thePair));
  wclear(reset_subwin);

  /* the Power (sub)window */
  wmove(FPWindow, 20, 56);
  wprintw(FPWindow, "Power (on/off)");
  onoff_subwin = subwin(FPWindow, 2, 4, 20, 73);
  theFG = COLOR_BLACK;
  theBG = COLOR_BLUE;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(onoff_subwin, COLOR_PAIR(thePair));
  wclear(onoff_subwin);

  wmove(FPWindow, 21, 74); // move cursor
  FP$power = TRUE;
  FP$reset = FALSE;

  FP_refresh();
}

/*-------------------------------------------------------------------*/
/* Refresh the front panel    (only when showing)                    */
/*-------------------------------------------------------------------*/
void FP_refresh() {
  if (FPPanel_data.hide)
    return;

  wrefresh(FPWindow);
  wrefresh(am_subwin);
  wrefresh(vam_subwin);
  wrefresh(led_subwin);
  wrefresh(reset_subwin);
  wrefresh(onoff_subwin);

  update_panels();
  doupdate();
}

/*-------------------------------------------------------------------*/
/* process front panel key strokes                                   */
/*-------------------------------------------------------------------*/
void FP_key(int c) {
  if (c == ERR)
    return;
  if (FPPanel_data.hide)
    return;

  if ((c > 7) & (c < 13)) { // covers tab, 4 arrows
    if (FP$power)
      c = 'r';
    else
      c = 'p';
  }
  if ((c == 'P') | (c == 'p')) { // power button
    FP$power = TRUE;
    FP$reset = FALSE;
    wmove(FPWindow, 21, 74);
  }
  if ((c == 'R') | (c == 'r')) { // reset button
    FP$power = FALSE;
    FP$reset = TRUE;
    wmove(FPWindow, 17, 74);
  }
  if ((c == ' ') | (c == 0x0d)) { // PRESS button...
    if (FP$power)
      if (Dialog_ConfirmQuit("Pressing the POWER button means QUIT!"))
        am100_state.wd16_cpu_state->regs.halting = true;
    if (FP$reset)
      if (Dialog_YN("Pressing the RESET button means reboot!",
                    "Are you sure you want to reboot now?")) {
        config_reset();
      }
  }

  FP_refresh();
}

/*-------------------------------------------------------------------*/
/* Refresh the front panel led                                       */
/*-------------------------------------------------------------------*/
void FP_led() {
  wmove(led_subwin, 0, 0);
  if (am100_state.wd16_cpu_state->regs.LED == 0)
    wprintw(led_subwin, "  ");
  else if (am100_state.wd16_cpu_state->regs.LED < 16)
    wprintw(led_subwin, " %0x1d", am100_state.wd16_cpu_state->regs.LED);
  else
    wprintw(led_subwin, "%0x2d", am100_state.wd16_cpu_state->regs.LED);
  FP_refresh();
}

/*-------------------------------------------------------------------*/
/* Stop the front panel                                              */
/*-------------------------------------------------------------------*/
void FP_stop() {}
