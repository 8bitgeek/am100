/* io.h          (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_IO_H__
#define __AM100_IO_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------*/
/* in io.c                                                           */
/*-------------------------------------------------------------------*/
void initAMports(void);
void io_stop(void);
void io_reset(void);
void regAMport(int portnum, PortPtr GoodPort, unsigned char *sa);
void getAMIObyte(unsigned char *chr, unsigned long PORT);
void putAMIObyte(unsigned char *chr, unsigned long PORT);
void LEDPort(unsigned char *chr, int rwflag, unsigned char *sa);
void DeadPort(unsigned char *chr, int rwflag, unsigned char *sa);


#ifdef __cplusplus
}
#endif

#endif
