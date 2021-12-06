/* am320.c         (c) Copyright Mike Noel, 2001-2008                */
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
/* This module presents an am320 hardware interface to the cpu,      */
/* and presents a connection to a printer 'file'.                    */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void am320_Init(unsigned int port,   // base port
                unsigned int intlvl, // interrupt level
                char *filename)      // filename
{
  CARDS *cptr;

  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("am320.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_am320;
  cptr->CType.AM320.port = port;
  cptr->CType.AM320.file = NULL;
  cptr->CType.AM320.filename[0] = '\0';
  cptr->CType.AM320.cntdown = 32000;
  cptr->CType.AM320.delay = intlvl;

  /* register port(s) */
  regAMport(port + 0, &am320_Port0, (unsigned char *)cptr); // 'P', 'I' reg
  regAMport(port + 1, &am320_Port1, (unsigned char *)cptr); // 'C', 'G' reg

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = cards;
  cards = cptr;

  am320_mount(port, filename); // mount file if specified
}

/*-------------------------------------------------------------------*/
/* Stop the board emulator                                           */
/*-------------------------------------------------------------------*/
void am320_stop() {
  CARDS *cptr;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am320) {

      /* don't let the port code try to write while we try to close! */
      pthread_mutex_lock(&clocklock_t);
      if (cptr->CType.AM320.file != NULL) {
        if (cptr->CType.AM320.filename[0] != '|')
          fclose(cptr->CType.AM320.file);
        else
          pclose(cptr->CType.AM320.file);
        cptr->CType.AM320.file = NULL;
      }
      pthread_mutex_unlock(&clocklock_t);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Reset the board emulator                                          */
/*-------------------------------------------------------------------*/
void am320_reset(unsigned char *sa) {
  CARDS *cptr;
  cptr = (CARDS *)sa;

  if (cptr->C_Type == C_Type_am320) {

    /* don't let the port code try to write while we try to close! */
    pthread_mutex_lock(&clocklock_t);
    if (cptr->CType.AM320.file != NULL) {
      if (cptr->CType.AM320.filename[0] != '|')
        fclose(cptr->CType.AM320.file);
      else
        pclose(cptr->CType.AM320.file);
      cptr->CType.AM320.file = NULL;
    }
    pthread_mutex_unlock(&clocklock_t);
  }
}

/*-------------------------------------------------------------------*/
/* Poll the board emulator                                           */
/*-------------------------------------------------------------------*/
int am320_poll(unsigned char *sa) {
  CARDS *cptr;
  int tp, oldtp;
  cptr = (CARDS *)sa;

  // see if 'ticks' has changed...
  oldtp = cptr->CType.AM320.tp;
  getAMword((unsigned char *)&tp, 0x52); // time
  cptr->CType.AM320.tp = tp;

  // close printer when it's idle more than a few seconds
  pthread_mutex_lock(&clocklock_t);
  if (cptr->CType.AM320.file != NULL) {

    if (tp != oldtp)
      cptr->CType.AM320.cntdown--;
    if (cptr->CType.AM320.cntdown < 1) {
      fclose(cptr->CType.AM320.file);
      cptr->CType.AM320.file = NULL;
    }
  }
  pthread_mutex_unlock(&clocklock_t);

  return 0;
}

/*-------------------------------------------------------------------*/
/* Obtain list of currently mounted files...                         */
/*-------------------------------------------------------------------*/
void am320_getfile(unsigned int port, char *filename) {
  CARDS *cptr;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am320)
      if (cptr->CType.AM320.port == port) {
        if (strlen(cptr->CType.AM320.filename) > 0)
          sprintf(filename, "%-28s", // left justified...
                  cptr->CType.AM320.filename);
        else
          sprintf(filename, "                           ");
      }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* 'Mount' printer file                                              */
/*-------------------------------------------------------------------*/
int am320_mount(unsigned int port, // base port (for id)
                char *filename)    // filename (to be mounted)
{
  CARDS *cptr;
  FILE *fp;
  int itWorked = FALSE;

  am320_unmount(port);
  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am320) {

      if (cptr->CType.AM320.port == port) {
        pthread_mutex_lock(&clocklock_t);
        if (strlen(filename) > 0) {
          if (filename[0] != '|')
            fp = fopen((char *)filename, "a");
          else
            fp = popen((char *)&(filename[1]), "w");
          if (fp != NULL) {
            if (filename[0] != '|')
              fclose(fp);
            else
              pclose(fp);
            strncpy(cptr->CType.AM320.filename, filename, 253);
            itWorked = TRUE;
          }
        }
        pthread_mutex_unlock(&clocklock_t);
      }
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);

  return (itWorked);
}

/*-------------------------------------------------------------------*/
/* 'UnMount' printer file                                            */
/*-------------------------------------------------------------------*/
void am320_unmount(unsigned int port) // base port (for id)
{
  CARDS *cptr;

  cptr = cards;
  do {
    if (cptr->C_Type == C_Type_am320) {

      if (cptr->CType.AM320.port == port) {
        pthread_mutex_lock(&clocklock_t);
        if (strlen(cptr->CType.AM320.filename) > 0) {
          if (cptr->CType.AM320.file != NULL) {
            if (cptr->CType.AM320.filename[0] != '|')
              fclose(cptr->CType.AM320.file);
            else
              pclose(cptr->CType.AM320.file);
          }
          cptr->CType.AM320.file = NULL;
          cptr->CType.AM320.filename[0] = '\0';
        }
        pthread_mutex_unlock(&clocklock_t);
      }
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* Port Handlers                                                     */
/*-------------------------------------------------------------------*/
void am320_Port0(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //                              -------- 'P', 'I' reg ---------------
  cptr = (CARDS *)sa;

  switch (rwflag) {

  case 0: /* read */
    *chr = 0xff;
    break;

  case 1: /* write */
    /* don't let the clock task close file while we try to write!   */
    pthread_mutex_lock(&clocklock_t);
    if (strlen(cptr->CType.AM320.filename) > 0) {
      if (cptr->CType.AM320.file == NULL) {
        if (cptr->CType.AM320.filename[0] != '|')
          cptr->CType.AM320.file = fopen(cptr->CType.AM320.filename, "a");
        else
          cptr->CType.AM320.file = popen(&(cptr->CType.AM320.filename[1]), "w");
      }
      fputc(*chr, cptr->CType.AM320.file);
      cptr->CType.AM320.cntdown = cptr->CType.AM320.delay;
    }
    pthread_mutex_unlock(&clocklock_t);
    break;

  default:
    assert("am320.c - invalid rwflag arg to am320_Port0...");
  }
}

void am320_Port1(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  //                              -------- 'C', 'G' reg ---------------
  cptr = (CARDS *)sa;
  switch (rwflag) {

  case 0: /* read */
    *chr = 0xff;
    break;

  case 1: /* write */
    break;

  default:
    assert("am320.c - invalid rwflag arg to am320_Port1...");
  }
}
