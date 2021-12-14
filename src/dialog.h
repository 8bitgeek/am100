/* cpu-fmt9.h    (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_DIALOG_H__
#define __AM100_DIALOG_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif
/*-------------------------------------------------------------------*/
/* in dialogs.c                                                      */
/*-------------------------------------------------------------------*/
int Dialog_ConfirmQuit(char *msg);
void Dialog_ALTHelp(void);
void Dialog_Mount(void);
int Dialog_YN(char *msg1, char *msg2);
void Dialog_OK(char *msg);
void wdbox(WINDOW *win, int nlines, int ncols, int begin_y, int begin_x,
           char *text, int attrs);
void webox(WINDOW *win, int nlines, int ncols, int begin_y, int begin_x,
           int attrs);


#ifdef __cplusplus
}
#endif

#endif
