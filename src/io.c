/* io.c       (c) Copyright Mike Noel, 2001-2008                     */
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
/* This module does all I/O access and control.  Since the AM100     */
/* uses memory mapped I/O this routines in this module are only      */
/* called from the routines in memory.c                              */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize port handlers                                          */
/*-------------------------------------------------------------------*/
void initAMports() {
  int i;

  for (i = 0; i < 256; i++) /* turn off all ports */
  {
    am_ports[i] = &DeadPort;
    am_portbase[i] = (unsigned char *)i; // DeadPort doesn't need storage,
    //                                     uses pointer to know own addr.
  }
}

/*-------------------------------------------------------------------*/
/* Shutdown IO subsystem                                             */
/*-------------------------------------------------------------------*/
void io_stop() {}

/*-------------------------------------------------------------------*/
/* put io back into initial configuration                            */
/*-------------------------------------------------------------------*/
void io_reset() {
  CARDS *cptr;

  cptr = cards;
  do { // reset each card
    if (cptr->C_Type == C_Type_am100) {
      /* no action */
    }
    if (cptr->C_Type == C_Type_am200) {
      /* no action */
    }
    if (cptr->C_Type == C_Type_am300) {
      am300_reset((unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_am320) {
      am320_reset((unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_am600) {
      am600_reset((unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_PS3) {
      PS3_reset((unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_ram) {
      /* no action (handled in 'memory_reset') */
    }
    if (cptr->C_Type == C_Type_rom) {
      /* no action (handled in 'memory_reset') */
    }
    if (cptr->C_Type == C_Type_clock) {
      clock_reset();
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Register a port                                                   */
/*-------------------------------------------------------------------*/
void regAMport(int portnum, PortPtr GoodPort, unsigned char *sa) {
  am_ports[portnum] = GoodPort;
  am_portbase[portnum] = sa;
}

/*-------------------------------------------------------------------*/
/* Read Byte from I/O port                                           */
/*-------------------------------------------------------------------*/
void getAMIObyte(unsigned char *chr, unsigned long PORT) {
  am_ports[PORT](chr, 0, am_portbase[PORT]);
}

/*-------------------------------------------------------------------*/
/* Write Byte to I/O port                                            */
/*-------------------------------------------------------------------*/
void putAMIObyte(unsigned char *chr, unsigned long PORT) {
  am_ports[PORT](chr, 1, am_portbase[PORT]);
}

/*-------------------------------------------------------------------*/
/* Diag LED Port Handler (port 0)                                    */
/*-------------------------------------------------------------------*/
void LEDPort(unsigned char *chr, int rwflag, unsigned char *sa) {
  if (rwflag == 0)
    //   *chr = regs.LED;   // (fails diag3 this way)
    *chr = 0xff; // read on real one probably always got 0xff...
  else {
    regs.LED = *chr; // write saves value
    FP_led();

#ifdef _TRACE_INST_
    if (regs.tracing)
      fprintf(stderr, "\n%02x written to LED", regs.LED);
#endif

    if (regs.LED == 0) { /* assume monitor is booting if we just */
      regs.BOOTing = 1;  /* wrote a zero into the LED.           */
    }
  }
}

/*-------------------------------------------------------------------*/
/* Dummy Port Handler                                                */
/*-------------------------------------------------------------------*/
void DeadPort(unsigned char *chr, int rwflag, unsigned char *sa) {

#ifdef _TRACE_INST_
  if (regs.tracing)
    fprintf(stderr, "\nIO to unassigned port %02x, data %02x, rw %d", (int)sa,
            *chr, rwflag);
#endif

  if (rwflag == 0)
    *chr = 0xff; /* read gives ff */
  /* other calls ignored */
}
