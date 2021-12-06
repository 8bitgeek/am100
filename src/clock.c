/* clock.c         (c) Copyright Mike Noel, 2001-2008                */
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

int BG_check_terms(void);
int BG_check_others(void);

/*-------------------------------------------------------------------*/
/* This module is the line clock for the am100 emulator.  It also    */
/* handles such tasks as need to occur on a periodic basis           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the clock                                              */
/* The clock isn't a card, but it's convenient to treat it like one  */
/*-------------------------------------------------------------------*/
void clock_Init(unsigned int clkfrq) {
  CARDS *cptr;

  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("clock.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_clock;
  cptr->CType.clock.clkfrq = clkfrq;

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = cards;
  cards = cptr;

  /* make a thread for the line clock interrupts */
  pthread_create(&lineclock_t, &attR_t, (void *)&clock_thread, NULL);
  usleep(100000); // let the clock thread get going

  /* make a separate thread to handle the other periodic tasks */
  pthread_create(&background_t, &attR_t, (void *)&background_thread, NULL);
  usleep(100000); // let the background thread get going
}

/*-------------------------------------------------------------------*/
/* Terminate the clock(s)                                            */
/* The clock isn't a card, but it's convenient to treat it like one  */
/*-------------------------------------------------------------------*/
void clock_stop(void) {
  CARDS *cptr;

  /* find the clock on the card chain */
  cptr = cards;
  while (cptr->C_Type != C_Type_clock) {
    cptr = cptr->CARDS_NEXT;
    if (cptr == NULL) {
      assert("clock_stop can't find cptr...");
      exit(0);
    }
  }

  /* set ticks < 0 to shutdown the clock thread(s) */
  line_clock_ticks = -1;

  /* wait for clock tasks to die */
  pthread_join(lineclock_t, NULL);
  pthread_join(background_t, NULL);
}

/*-------------------------------------------------------------------*/
/* Reset the clock(s)                                                */
/* The clock isn't a card, but it's convenient to treat it like one  */
/*-------------------------------------------------------------------*/
void clock_reset(void) {
  CARDS *cptr;

  /* find the clock on the card chain */
  cptr = cards;
  while (cptr->C_Type != C_Type_clock) {
    cptr = cptr->CARDS_NEXT;
    if (cptr == NULL) {
      assert("clock_reset can't find cptr...");
      exit(0);
    }
  }
  regs.BOOTing = 1;
}

/*-------------------------------------------------------------------*/
/* Clock Thread                                                      */
/* sets a line clock interrupt every clkfrq of a second              */
/* also punch date and time into AMOS memory to force clock accuracy */
/*-------------------------------------------------------------------*/
void clock_thread(void) {
  CARDS *cptr;
  long clkfrq, lcladj = 0, wait_time;
  struct timeval tv;
  time_t tnow;
  struct tm *m;
  U16 tp;
  U8 mdy;

  set_pri_high();

  /* find the clock on the card chain */
  cptr = cards;
  while (cptr->C_Type != C_Type_clock) {
    cptr = cptr->CARDS_NEXT;
    if (cptr == NULL) {
      assert("clock thread init can't find cptr...");
      exit(0);
    }
  }
  clkfrq = cptr->CType.clock.clkfrq;

  /* compute the usleep time from the clkfrq */
  wait_time = line_clock_ticks = (1000000 / clkfrq);

  /* post non-vectored interrupt and sleep a clkfrq of a second */
  while (line_clock_ticks > 0) {
    if (regs.BOOTing == 1) {
      lcladj = 0;
      regs.BOOTing = 0;
    }
    gettimeofday(&tv, (struct timezone *)0);
    line_clock_ticks = tv.tv_sec - lcladj;
    line_clock_ticks *= 1000;
    if (line_clock_ticks > 86400000) {
      tnow = time((time_t *)0);
      m = localtime(&tnow);
      lcladj = tv.tv_sec - (m->tm_hour * 3600) - (m->tm_min * 60) - (m->tm_sec);
      line_clock_ticks = tv.tv_sec - lcladj;
      line_clock_ticks *= 1000;
      mdy = m->tm_mon + 1;
      putAMbyte((unsigned char *)&mdy, 0x56);
      mdy = m->tm_mday;
      putAMbyte((unsigned char *)&mdy, 0x57);
      tp = m->tm_year - 100;
      mdy = tp;
      putAMbyte((unsigned char *)&mdy, 0x58);
      // fprintf(stderr,"clock rollover\n");
    }
    line_clock_ticks += tv.tv_usec / 1000;
    line_clock_ticks *= clkfrq;
    line_clock_ticks /= 1000;
    line_clock_ticks++; // incase it's exactly midnight...

    tp = line_clock_ticks & 0x0ffff;
    putAMword((unsigned char *)&tp, 0x52); // time
    tp = (line_clock_ticks >> 16) & 0x0ffff;
    putAMword((unsigned char *)&tp, 0x54); // time+2
    pthread_mutex_lock(&intlock_t);
    regs.whichint[0] = 1;
    regs.intpending = 1;
    pthread_mutex_unlock(&intlock_t);

    usleep(wait_time);
  }

  /* get here when someone sets ticks == 0 */
  pthread_exit(0);
}

/*-------------------------------------------------------------------*/
/* Thread for background work                                        */
/*-------------------------------------------------------------------*/
void background_thread(void) {
  int did;

  set_pri_normal();

  while (line_clock_ticks > 0) {

    did = 0;

    did += BG_check_terms(); // check for I/O on console(s)...

    did += BG_check_others(); // go do any other regular checks...

    if (did > 0)
      usleep(500);
    else
      usleep(10000);

    // die if parent process (ie, shell window) has died...
    if (getppid() == 1)
      regs.halting = 1;
  }

  /* get here when someone sets ticks == 0 */
  pthread_exit(0);
}

/*-------------------------------------------------------------------*/
/* Routine to do other terminal I/O                                  */
/*-------------------------------------------------------------------*/
int BG_check_terms(void) {
  CARDS *cptr;
  PANEL *thePanel;
  PANEL_DATA *thePanelData;
  int did3 = 0, i, numUnhide = 0;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_PS3) {
      did3 += PS3_poll((unsigned char *)cptr);
      thePanel = cptr->CType.PS3.thePanel;
      if (thePanel != NULL) {
        thePanelData = panel_userptr(thePanel);
        if (!thePanelData->hide)
          numUnhide++;
      }
    }

    if (cptr->C_Type == C_Type_am300) {
      did3 += am300_poll((unsigned char *)cptr);
      for (i = 0; i < 6; i++) {
        thePanel = cptr->CType.AM300.thePanel[i];
        if (thePanel != NULL) {
          thePanelData = panel_userptr(thePanel);
          if (!thePanelData->hide)
            numUnhide++;
        }
      }
    }

    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);

  if (numUnhide == 0) // allow alt's if no one is listening...
    FP_key(InChars());

  return (did3);
}

/*-------------------------------------------------------------------*/
/* Routine to do other repetative tasks...                           */
/*-------------------------------------------------------------------*/
int BG_check_others(void) {
  CARDS *cptr;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am320) {
      am320_poll((unsigned char *)cptr);
    }

    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);

  return 0;
}
