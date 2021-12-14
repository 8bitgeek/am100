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

#include <wd16.h>
#include "am100.h"
#include "config.h"

am100_state_t am100_state;

static void am100_init(void);
static void usage(void);

void am100_init()
{
  memset(&am100_state,0,sizeof(am100_state_t));
  am100_state.wd16_cpu_state = &wd16_cpu_state;

  /** @FIXME - initialize wd16 callbacks */


}

/*-------------------------------------------------------------------*/
/* This module initializes the am100 emulator.                       */
/*-------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
  char *inifile = "am100.ini";
  int c, arg_error = 0;

  am100_init();

  /* get command line argument(s) (if any) */
  while ((c = getopt(argc, argv, "f:PT")) != EOF) {
    switch (c) {
    case 'f':
      inifile = optarg;
      break;
    case 'P':
      am100_state.gPOST++;
      break;
    case 'T':
      am100_state.gTRACE++;
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

  am100_state.wd16_cpu_state->regs.waiting = 0;           // start cpu running
  pthread_join(am100_state.wd16_cpu_state->cpu_t, NULL);   // wait for it to die

  config_stop();

  return (0);
}

static void usage(void ) 
{
  fprintf(stderr, "Syntax:\tam100 {-P -T -f inifile}\n"
                  " where:\t-f inifile = name of initialization file\n"
                  " where:\t-T = startup with tracing on\n"
                  " where:\t-P = detailed POST (power on self test)\n");
  sleep(10);
  exit(99);
}

