/* main.c       (c) Copyright Mike Noel, 2001-2008                   */
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

void usage() {
  fprintf(stderr, "Syntax:\tam100 {-P -T -f inifile}\n"
                  " where:\t-f inifile = name of initialization file\n"
                  " where:\t-T = startup with tracing on\n"
                  " where:\t-P = detailed POST (power on self test)\n");
  sleep(10);
  exit(99);
}

/*-------------------------------------------------------------------*/
/* This module initializes the am100 emulator.                       */
/*-------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char *inifile = "am100.ini";
  int c, arg_error = 0;

  /* get command line argument(s) (if any) */
  gPOST = 0;       // default to no POST
  gTRACE = 0;      // default to no startup Trace
  gDialog = FALSE; // and while we're at it no Dialog now...
  while ((c = getopt(argc, argv, "f:PT")) != EOF) {
    switch (c) {
    case 'f':
      inifile = optarg;
      break;
    case 'P':
      gPOST++;
      break;
    case 'T':
      gTRACE++;
      break;
    default:
      arg_error++;
    }
  }
  if (optind < argc)
    arg_error++;
  if (arg_error > 0)
    usage();

  if (!config_start(inifile)) {
    // if (panels != NULL) hide_all_panels();
    fprintf(stderr, "configuration failure!\n\r");
    sleep(4);
    endwin();
    exit(99);
  }

  regs.waiting = 0;          // start cpu running
  pthread_join(cpu_t, NULL); // wait for it to die

  config_stop();

  return (0);
}
