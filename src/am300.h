/* am300.h       (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_AM300_H__
#define __AM100_AM300_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------*/
/* in am300.c                                                        */
/*-------------------------------------------------------------------*/
void am300_Init(unsigned int port, unsigned int intlvl, unsigned int fnum1,
                unsigned int fnum2, unsigned int fnum3, unsigned int fnum4,
                unsigned int fnum5, unsigned int fnum6);
void am300_stop(void);
void am300_reset(unsigned char *sa);
int am300_poll(unsigned char *sa);
void am300_Port0(unsigned char *chr, int rwflag, unsigned char *sa);
void am300_Port1(unsigned char *chr, int rwflag, unsigned char *sa);
void am300_Port2(unsigned char *chr, int rwflag, unsigned char *sa);
void am300_Port3(unsigned char *chr, int rwflag, unsigned char *sa);
void am300_Port4(unsigned char *chr, int rwflag, unsigned char *sa);



#ifdef __cplusplus
}
#endif

#endif
