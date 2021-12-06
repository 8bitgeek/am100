#include "am100.h"

int main() {
  int bg, fg, pair, xmax, ymax;
  int theBG, theFG, thePair;

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

  // theBG = COLOR_BLACK;
  // theFG = COLOR_WHITE;
  // thePair = 8 * theFG + theBG + 1;
  // attron (COLOR_PAIR (thePair));
  attron(COLOR_PAIR(8 * COLOR_WHITE + COLOR_BLACK + 1));

  /* NCURSES - Get keyboard into raw mode but let it map escape sequences */
  raw();
  noecho();
  nodelay(stdscr, TRUE);

  /* NCURSES - init my list of panels */
  panels = NULL;

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

  update_panels();
  doupdate();

  sleep(3);

  endwin();
}
