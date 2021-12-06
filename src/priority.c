/* priority.c    (c) Copyright Mike Noel, 2001-2008                  */
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

#ifdef __CYGWIN__
#include <windows.h>
#else
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

void set_pri_low() {

#ifdef __CYGWIN__
  {
    HANDLE threadId;
    int err = 0;
    threadId = GetCurrentThread();
    if (!SetThreadPriority(threadId, THREAD_PRIORITY_LOWEST))
      err = GetLastError();
  }
#else
  setpriority(PRIO_PROCESS, 0, 15);
#endif
}

void set_pri_normal() {

#ifdef __CYGWIN__
  {
    HANDLE threadId;
    int err = 0;
    threadId = GetCurrentThread();
    if (!SetThreadPriority(threadId, THREAD_PRIORITY_BELOW_NORMAL))
      err = GetLastError();
  }
#else
  setpriority(PRIO_PROCESS, 0, 5);
#endif
}

void set_pri_high() {

#ifdef __CYGWIN__
  {
    HANDLE threadId;
    int err = 0;
    threadId = GetCurrentThread();
    if (!SetThreadPriority(threadId, THREAD_PRIORITY_NORMAL))
      err = GetLastError();
  }
#else
  setpriority(PRIO_PROCESS, 0, 1);
#endif
}
