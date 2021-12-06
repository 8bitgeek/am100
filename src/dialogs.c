/* Dialogs.c         (c) Copyright Mike Noel, 2001-2008              */
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

/*-------------------------------------------------------------------*/
/*                                                                   */
/* This module is a set of dialogs for user interaction.       */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "am100.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*-------------------------------------------------------------------*/
/* Dialog to confirm Quit command.                                   */
/*-------------------------------------------------------------------*/
int Dialog_ConfirmQuit(char *msg) {
  return Dialog_YN(msg, "Are you sure you want to Quit now?");
}

/*-------------------------------------------------------------------*/
/* Dialog to provide help with ALT commands.                         */
/*-------------------------------------------------------------------*/
void Dialog_ALTHelp(void) {
  Dialog_OK("ALT commands are:\n"
            "  alt-t            toggle trace on/off\n"
            "  alt-s            toggle instr step on/off\n"
            "  alt-r            resume from instr step\n"
            "  alt-0, alt-f     access front panel\n"
            "  alt-1, 2, ...9   access console 1, 2, ...9\n"
            "  alt-m            mount/change disk, tape, printer\n"
            "  alt-z            repaint current screen\n");
}

/*-------------------------------------------------------------------*/
/* Dialog to mount/unmount devices.                                  */
/*-------------------------------------------------------------------*/
void Dialog_Mount(void) {
  WINDOW *theWindow;
  PANEL *thePanel;
  CARDS *cptr;

  ITEM **dev_items;
  MENU *dev_menu;
  int n_dev_choices, dev_item;
  char *dev_choices[] = {" DSK0 ", " DSK1 ",    " DSK2 ", " DSK3 ", " LPR0 ",
                         " LPR1 ", " LPR2 ",    " LPR3 ", " MTU0 ", " MTU1 ",
                         " MTU2 ", " MTU3 ",    " MTU4 ", " MTU5 ", " MTU6 ",
                         " MTU7 ", (char *)NULL};

  char *dev_fns[16], dev_fns_buf[480];

  ITEM **mu_items;
  MENU *mu_menu;
  int n_mu_choices, mu_item = 0;
  char *mu_choices[] = {"  Mount   ", " Un-Mount ", (char *)NULL};

  char filename[256];
  char devname[6];
  char msg[100];
  int filerw;
  int am320ports[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  int am600port = 0;

  int done = FALSE, c, i;
  int theBG, theFG, thePair;

  int h = 14, w = 34, bh = (24 - h) / 2, bw = (80 - w) / 2;
  // t     yh=3, yw=7, ybh=h-3, ybw=2;

  // first setup the dialog

  /* Create dialog window/panel */
  theWindow = newwin(h, w, bh, bw);
  nodelay(theWindow, TRUE);

  /* box window */
  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(theWindow, COLOR_PAIR(thePair));
  wclear(theWindow);
  box(theWindow, ACS_VLINE, ACS_HLINE);

  /* Create items */
  n_dev_choices = ARRAY_SIZE(dev_choices);
  dev_items = (ITEM **)calloc(n_dev_choices, sizeof(ITEM *));
  for (i = 0; i < n_dev_choices; i++)
    dev_items[i] = new_item(dev_choices[i], dev_choices[i]);

  n_mu_choices = ARRAY_SIZE(mu_choices);
  mu_items = (ITEM **)calloc(n_mu_choices, sizeof(ITEM *));
  for (i = 0; i < n_mu_choices; i++)
    mu_items[i] = new_item(mu_choices[i], mu_choices[i]);

  /* Create menu */
  dev_menu = new_menu((ITEM **)dev_items);
  mu_menu = new_menu((ITEM **)mu_items);

  /* Set menu option not to show the description */
  menu_opts_off(dev_menu, O_SHOWDESC);
  menu_opts_off(mu_menu, O_SHOWDESC);

  /* Set main window and sub windows */
  set_menu_win(dev_menu, theWindow);
  set_menu_sub(dev_menu, derwin(theWindow, 4, 28, 2, 3));
  set_menu_format(dev_menu, 4, 4);
  set_menu_mark(dev_menu, "");
  set_menu_back(dev_menu, COLOR_PAIR(thePair));

  set_menu_win(mu_menu, theWindow);
  set_menu_sub(mu_menu, derwin(theWindow, 1, 28, 8, 3));
  set_menu_format(mu_menu, 1, 2);
  set_menu_mark(mu_menu, "");
  set_menu_back(mu_menu, COLOR_PAIR(thePair));

  theBG = COLOR_WHITE;
  theFG = COLOR_RED;
  thePair = 8 * theFG + theBG + 1;
  set_menu_grey(dev_menu, COLOR_PAIR(thePair));

  theBG = COLOR_BLACK;
  theFG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  set_menu_fore(dev_menu, COLOR_PAIR(thePair));
  set_menu_fore(mu_menu, COLOR_PAIR(thePair));

  //      obtain current filenames and make missing devices non-selectable

  // init the filenames table
  for (i = 0; i < 16; i++) {
    dev_fns[i] = &dev_fns_buf[i * 30];
    dev_fns_buf[i * 30] = '\0';
  }

  // get the disk names
  vdkdvr_getfiles(dev_fns);

  // get the printer names
  cptr = cards;
  c = 3;
  do { // get first four am-320 cards...
    if (cptr->C_Type == C_Type_am320) {
      c++;
      strcpy(dev_fns[7], dev_fns[6]);
      am320ports[7] = am320ports[6];
      strcpy(dev_fns[6], dev_fns[5]);
      am320ports[6] = am320ports[5];
      strcpy(dev_fns[5], dev_fns[4]);
      am320ports[5] = am320ports[4];
      am320ports[4] = cptr->CType.AM320.port;
      am320_getfile(am320ports[4], dev_fns[4]);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
  for (i = 7; i > 3; i--)
    if (c < i) {
      item_opts_off(dev_items[i], O_SELECTABLE);
      strcpy(dev_fns[i], "                            ");
    }

  // get the tape names
  cptr = cards;
  c = 0;
  do { // get drives on first defined am-600 card...
    if (cptr->C_Type == C_Type_am600) {
      c++;
      am600port = cptr->CType.AM600.port;
      am600_getfiles(am600port, dev_fns);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
  if (c == 0)
    for (i = 9; i < 17; i++)
      item_opts_off(dev_items[i], O_SELECTABLE);

  // interact with user on device submenu

  /* Post the menu */
  post_menu(dev_menu);
  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wdbox(theWindow, 6, 30, 1, 2, "Select Device", COLOR_PAIR(thePair));

  wmove(theWindow, 11, 3);
  wattron(theWindow, A_REVERSE);
  wprintw(theWindow, "                            ");
  wmove(theWindow, 11, 3);
  wprintw(theWindow, "%s", dev_fns[0]);
  wmove(theWindow, 11, 3);
  wattroff(theWindow, A_REVERSE);

  /* show the whole panel */
  thePanel = new_panel(theWindow);
  show_panel(thePanel);
  update_panels();
  doupdate();

  /* make sure dialog nature globally understood */
  gDialog = TRUE;

  /* clear any pending chars */
  do {
    c = InChars();
  } while (c != ERR);
  done = FALSE;

  /* get and process response(s) */
  do {
    do {
      usleep(100000);
      c = InChars();
    } while (c == ERR);
    switch (c) {
    case 0x0a: // KEY_DOWN
      menu_driver(dev_menu, REQ_DOWN_ITEM);
      break;
    case 0x0b: // KEY_UP
      menu_driver(dev_menu, REQ_UP_ITEM);
      break;
    case 0x08: // KEY_LEFT
      menu_driver(dev_menu, REQ_LEFT_ITEM);
      break;
    case 0x0c: // KEY_RIGHT
    case 0x09: // tab
      menu_driver(dev_menu, REQ_RIGHT_ITEM);
      break;
    }

    dev_item = item_index(current_item(dev_menu));

    wmove(theWindow, 11, 3);
    wattron(theWindow, A_REVERSE);
    wprintw(theWindow, "                            ");
    wmove(theWindow, 11, 3);
    wprintw(theWindow, "%s", dev_fns[dev_item]);
    wmove(theWindow, 11, 3);
    wattroff(theWindow, A_REVERSE);

    if (c == 0x1b) {
      done = TRUE;
    }
    if (c == 0x0d) {
      c = item_opts(dev_items[dev_item]) & O_SELECTABLE;
      if (c != 0)
        done = TRUE;
      else {
        beep();
        flash();
      }
    }
    wrefresh(theWindow);
  } while (!done);
  dev_item = item_index(current_item(dev_menu));
  webox(theWindow, 6, 30, 1, 2, COLOR_PAIR(thePair));

  // interact with user on mount/un-mount submenu

  if (c != 0x1b) { // honor prior 'escape' exit...
    wdbox(theWindow, 3, 30, 7, 2, "Mount or Unmount?", COLOR_PAIR(thePair));
    refresh();

    post_menu(mu_menu);
    wrefresh(theWindow);

    /* clear any pending chars */
    do {
      c = InChars();
    } while (c != ERR);
    done = FALSE;

    /* get and process response(s) */
    do {
      do {
        usleep(100000);
        c = InChars();
      } while (c == ERR);
      switch (c) {
      case 0x08: // KEY_LEFT
      case 0x0c: // KEY_RIGHT
      case 0x09: // tab
        mu_item = item_index(current_item(mu_menu)) + 1;
        if (mu_item == 1)
          menu_driver(mu_menu, REQ_RIGHT_ITEM);
        else
          menu_driver(mu_menu, REQ_LEFT_ITEM);
        break;
      }
      if (c == 0x1b) {
        done = TRUE;
      }
      if (c == 0x0d) {
        done = TRUE;
      }
      wrefresh(theWindow);
    } while (!done);
    mu_item = item_index(current_item(mu_menu)) + 1;
    webox(theWindow, 3, 30, 7, 2, COLOR_PAIR(thePair));
  }

  // interact with user for filename

  if (c != 0x1b) { // honor prior 'escape' exit...
    strcpy(devname, item_name(dev_items[dev_item]));

    strncpy(filename, dev_fns[dev_item], 29);
    for (i = strlen(filename) - 1, done = FALSE; i >= 0; i--)
      if (filename[i] == ' ') {
        if (!done)
          filename[i] = '\0';
      } else
        done = TRUE;

    if (mu_item == 1) { // no filename for un-mount...
      wdbox(theWindow, 3, 30, 10, 2, "file name?", COLOR_PAIR(thePair));
      wmove(theWindow, 11, 3);
      wattron(theWindow, A_REVERSE);
      wprintw(theWindow, "                            ");
      wmove(theWindow, 11, 3);
      wprintw(theWindow, "%s", filename);
      wrefresh(theWindow);
      getns(filename, 29, theWindow);
      if (strlen(filename) == 0)
        c = 0x1b;
      wattroff(theWindow, A_REVERSE);
      webox(theWindow, 3, 30, 10, 2, COLOR_PAIR(thePair));
      wrefresh(theWindow);
    }
  }

  // interact with user on OK/cancel submenu

  if (c != 0x1b) { // honor prior 'escape' exit...
    if (mu_item == 1) {
      sprintf(msg, "%s on%s?", filename, devname);
      if (!Dialog_YN("Are you sure you want to mount", msg))
        c = 0x1b;
    } else {
      sprintf(msg, "un-mount%s?", devname);
      if (!Dialog_YN("Are you sure you want to", msg))
        c = 0x1b;
    }
  }

  // do requested mount or unmount...

  if (c != 0x1b) { // honor prior 'escape' exit...

    if (dev_item < 4) { // DSK's
      if (mu_item == 1)
        vdkdvr_mount(dev_item, filename);
      else
        vdkdvr_unmount(dev_item);
    }

    if ((dev_item > 3) && (dev_item < 8)) { // LPR's
      if (mu_item == 1)
        am320_mount(am320ports[dev_item], filename);
      else
        am320_unmount(am320ports[dev_item]);
    }

    if (dev_item > 7) { // MTU's
      filerw = 0;
      if (mu_item == 1)
        if (Dialog_YN("'TAPE's can be mounted R/W or R/O",
                      "Do you want to mount this one R/O?"))
          filerw = 1;
      if (mu_item == 1)
        am600_mount(am600port, (dev_item - 8), filename, filerw);
      else
        am600_unmount(am600port, (dev_item - 8));
    }

  } /* end 'honor prior...' */

  // then tear down the dialog

  /* hide the panel then delete everything */
  hide_panel(thePanel);
  update_panels();
  doupdate();
  del_panel(thePanel);

  unpost_menu(dev_menu);
  free_menu(dev_menu);
  for (i = 0; i < n_dev_choices; i++)
    free_item(dev_items[i]);

  unpost_menu(mu_menu);
  free_menu(mu_menu);
  for (i = 0; i < n_mu_choices; i++)
    free_item(mu_items[i]);

  delwin(theWindow);

  /* make sure dialog nature globally understood */
  gDialog = FALSE;
}

/*-------------------------------------------------------------------*/
/* Dialog to obtain a Yes (TRUE) or No (FALSE) answer.               */
/*-------------------------------------------------------------------*/
int Dialog_YN(char *msg1, char *msg2) {
  WINDOW *theWindow;
  PANEL *thePanel;

  ITEM **yn_items;
  MENU *yn_menu;
  int n_yn_choices, yn_item;
  char *yn_choices[] = {"  Yes  ", "  No   ", (char *)NULL};

  int theBG, theFG, thePair;
  int YorN = FALSE, done = FALSE;
  int c, i;

  int h = 8, w = 40, bh = (24 - h) / 2, bw = (80 - w) / 2;
  int yh = 3, yw = 7, ybh = h - 3, ybw = 2;

  // first setup the dialog

  /* Create dialog window/panel */
  theWindow = newwin(h, w, bh, bw);
  nodelay(theWindow, TRUE);

  /* box window */
  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(theWindow, COLOR_PAIR(thePair));
  wclear(theWindow);
  box(theWindow, ACS_VLINE, ACS_HLINE);

  /* draw the msg */
  wmove(theWindow, 1, 2);
  wprintw(theWindow, "%s", msg1);
  wmove(theWindow, 2, 2);
  wprintw(theWindow, "%s", msg2);

  /* the Y/N menu */
  n_yn_choices = ARRAY_SIZE(yn_choices);
  yn_items = (ITEM **)calloc(n_yn_choices, sizeof(ITEM *));
  for (i = 0; i < n_yn_choices; i++)
    yn_items[i] = new_item(yn_choices[i], yn_choices[i]);
  yn_menu = new_menu((ITEM **)yn_items);
  menu_opts_off(yn_menu, O_SHOWDESC);
  set_menu_win(yn_menu, theWindow);

  set_menu_sub(yn_menu, derwin(theWindow, yh, yw + 8, ybh, ybw));
  set_menu_format(yn_menu, 1, 2);
  set_menu_mark(yn_menu, "");
  set_menu_back(yn_menu, COLOR_PAIR(thePair));
  post_menu(yn_menu);
  menu_driver(yn_menu, REQ_RIGHT_ITEM); // preset "no"
  theFG = COLOR_WHITE;
  theBG = COLOR_BLACK;
  thePair = 8 * theFG + theBG + 1;
  set_menu_fore(yn_menu, COLOR_PAIR(thePair));
  wdbox(theWindow, yh, yw + 1, ybh - 1, ybw - 1 + 8, "", COLOR_PAIR(thePair));

  /* show the whole panel */
  thePanel = new_panel(theWindow);
  show_panel(thePanel);
  update_panels();
  doupdate();

  // then get user response

  /* make sure dialog nature globally understood */
  gDialog = TRUE;

  /* clear any pending chars */
  do {
    c = InChars();
  } while (c != ERR);

  /* get and process response(s) */
  do {
    do {
      usleep(100000);
      c = InChars();
    } while (c == ERR);
    if ((c == 'Y') | (c == 'y'))
      YorN = TRUE;
    if ((c == 'N') | (c == 'n'))
      YorN = FALSE;
    if ((c == 0x09) | (c == 0x0c) | (c == 0x08)) // tabs
    {
      if (YorN)
        YorN = FALSE;
      else
        YorN = TRUE;
    }
    yn_item = item_index(current_item(yn_menu)) + 1;
    if ((yn_item == 1) & (!YorN)) {
      webox(theWindow, yh, yw + 1, ybh - 1, ybw - 1, COLOR_PAIR(thePair));
      menu_driver(yn_menu, REQ_RIGHT_ITEM);
      wdbox(theWindow, yh, yw + 1, ybh - 1, ybw - 1 + 8, "",
            COLOR_PAIR(thePair));
      wrefresh(theWindow);
    }
    if ((yn_item == 2) & (YorN)) {
      webox(theWindow, yh, yw + 1, ybh - 1, ybw - 1 + 8, COLOR_PAIR(thePair));
      menu_driver(yn_menu, REQ_LEFT_ITEM);
      wdbox(theWindow, yh, yw + 1, ybh - 1, ybw - 1, "", COLOR_PAIR(thePair));
      wrefresh(theWindow);
    }
    if (c == 0x1b) {
      YorN = FALSE;
      done = TRUE;
    }
    if (c == 0x0d) {
      done = TRUE;
    }
  } while (!done);

  // then tear down the dialog

  /* hide the panel then delete everything */
  hide_panel(thePanel);
  update_panels();
  doupdate();
  del_panel(thePanel);
  unpost_menu(yn_menu);
  free_menu(yn_menu);
  for (i = 0; i < n_yn_choices; i++)
    free_item(yn_items[i]);
  delwin(theWindow);

  /* make sure dialog nature globally understood */
  gDialog = FALSE;

  // then return final user response

  return (YorN);
}

/*-------------------------------------------------------------------*/
/* Dialog to put up some info and wait for user dismiss              */
/*-------------------------------------------------------------------*/
void Dialog_OK(char *msg) {
  WINDOW *theWindow;
  PANEL *thePanel;
  ITEM **ok_items;
  MENU *ok_menu;

  int theBG, theFG, thePair;
  int done = FALSE;
  int c, i, n_ok_choices;

  int numChars, maxChars = 0, numLines = 0;
  char *lines[20], bufmsg[1000];

  int h = 6, w = 4, bh = (24 - h) / 2, bw = (80 - w) / 2; // revised below
  int yh = 3, yw = 6, ybh = h - 4, ybw = 2;

  char *ok_choices[] = {" OK ", (char *)NULL};

  // first scan the message, determine how
  // many lines and max line length...

  numChars = strlen(msg);
  if (numChars > 1000)
    assert("msg into Dialog_OK is too big");
  strcpy(bufmsg, msg);
  lines[numLines] = &bufmsg[0];
  numLines++;
  for (c = 1; c < numChars; c++)
    if (bufmsg[c] == '\n') {
      bufmsg[c] = 0;
      if (c < (numChars - 1)) {
        lines[numLines] = &bufmsg[c + 1];
        numLines++;
        if (numLines > 20)
          assert("lines array too small in Dialog_OK");
      }
    }
  for (c = 0; c < numLines; c++) {
    numChars = strlen(lines[c]);
    if (numChars > maxChars)
      maxChars = numChars;
  }

  /* adjust size of window and where OK button goes */
  h += numLines + 1;
  w += maxChars;
  bh = (24 - h) / 2;
  bw = (80 - w) / 2;
  ybh = h - 4;

  // then setup the dialog

  /* Create dialog window/panel */
  theWindow = newwin(h, w, bh, bw);
  nodelay(theWindow, TRUE);

  /* box window */
  theFG = COLOR_BLACK;
  theBG = COLOR_WHITE;
  thePair = 8 * theFG + theBG + 1;
  wbkgd(theWindow, COLOR_PAIR(thePair));
  wclear(theWindow);
  box(theWindow, ACS_VLINE, ACS_HLINE);

  /* draw the msg */
  for (c = 0; c < numLines; c++) {
    wmove(theWindow, 1 + c, 2);
    wprintw(theWindow, "%s", lines[c]);
  }
  wmove(theWindow, ybh, ybw);

  /* the OK menu */
  n_ok_choices = ARRAY_SIZE(ok_choices);
  ok_items = (ITEM **)calloc(n_ok_choices, sizeof(ITEM *));
  for (i = 0; i < n_ok_choices; i++)
    ok_items[i] = new_item(ok_choices[i], ok_choices[i]);
  ok_menu = new_menu((ITEM **)ok_items);
  menu_opts_off(ok_menu, O_SHOWDESC);
  set_menu_win(ok_menu, theWindow);
  set_menu_sub(ok_menu, derwin(theWindow, yh, yw, ybh, ybw));
  set_menu_format(ok_menu, 1, 1);
  set_menu_mark(ok_menu, "");
  set_menu_back(ok_menu, COLOR_PAIR(thePair));
  post_menu(ok_menu);
  theFG = COLOR_WHITE;
  theBG = COLOR_BLACK;
  thePair = 8 * theFG + theBG + 1;
  set_menu_fore(ok_menu, COLOR_PAIR(thePair));
  wdbox(theWindow, yh, yw, ybh - 1, ybw - 1, "", COLOR_PAIR(thePair));

  /* show the whole panel */
  thePanel = new_panel(theWindow);
  show_panel(thePanel);
  update_panels();
  doupdate();

  // then get user response

  /* make sure dialog nature globally understood */
  gDialog = TRUE;

  /* clear any pending chars */
  do {
    c = InChars();
  } while (c != ERR);

  /* get and process response(s) */
  do {
    do {
      usleep(100000);
      c = InChars();
    } while (c == ERR);
    if (c == 0x1b) {
      done = TRUE;
    }
    if (c == 0x0d) {
      done = TRUE;
    }
  } while (!done);

  // then tear down the dialog

  /* hide the panel then delete everything */
  hide_panel(thePanel);
  update_panels();
  doupdate();
  del_panel(thePanel);
  unpost_menu(ok_menu);
  free_menu(ok_menu);
  for (i = 0; i < n_ok_choices; i++)
    free_item(ok_items[i]);
  delwin(theWindow);

  /* make sure dialog nature globally understood */
  gDialog = FALSE;
}

/*-------------------------------------------------------------------*/
/* Function to draw a titled box anywhere...                         */
/*-------------------------------------------------------------------*/
void wdbox(WINDOW *win, int nlines, int ncols, int begin_y, int begin_x,
           char *text, int attrs) {
  int i;

  wattron(win, attrs);
  wattron(win, A_ALTCHARSET);
  wmove(win, begin_y, begin_x);
  for (i = 0; i < ncols; i++)
    wprintw(win, "%c", ACS_HLINE);
  wmove(win, begin_y + nlines - 1, begin_x);
  for (i = 0; i < ncols; i++)
    wprintw(win, "%c", ACS_HLINE);
  for (i = 0; i < nlines; i++) {
    wmove(win, begin_y + i, begin_x);
    wprintw(win, "%c", ACS_VLINE);
    wmove(win, begin_y + i, begin_x + ncols - 1);
    wprintw(win, "%c", ACS_VLINE);
  }
  wmove(win, begin_y, begin_x);
  wprintw(win, "%c", ACS_ULCORNER);
  wmove(win, begin_y, begin_x + ncols - 1);
  wprintw(win, "%c", ACS_URCORNER);
  wmove(win, begin_y + nlines - 1, begin_x);
  wprintw(win, "%c", ACS_LLCORNER);
  wmove(win, begin_y + nlines - 1, begin_x + ncols - 1);
  wprintw(win, "%c", ACS_LRCORNER);
  wattroff(win, A_ALTCHARSET);
  i = strlen(text);
  if (i > 0) {
    i = (ncols - strlen(text)) / 2;
    wmove(win, begin_y, begin_x + i - 1);
    wprintw(win, " %s ", text);
  }
  wmove(win, begin_y, begin_x);
}

/*-------------------------------------------------------------------*/
/* Function to erase a titled box anywhere...                        */
/*-------------------------------------------------------------------*/
void webox(WINDOW *win, int nlines, int ncols, int begin_y, int begin_x,
           int attrs) {
  int i;

  wattroff(win, attrs);
  wmove(win, begin_y, begin_x);
  for (i = 0; i < ncols; i++)
    wprintw(win, " ");
  wmove(win, begin_y + nlines - 1, begin_x);
  for (i = 0; i < ncols; i++)
    wprintw(win, " ");
  for (i = 0; i < nlines; i++) {
    wmove(win, begin_y + i, begin_x);
    wprintw(win, " ");
    wmove(win, begin_y + i, begin_x + ncols - 1);
    wprintw(win, " ");
  }
  wmove(win, begin_y, begin_x);
}
