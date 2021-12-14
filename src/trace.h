/* trace.h       (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_TRACE_H__
#define __AM100_TRACE_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------*/
/* in trace.c                                                        */
/*-------------------------------------------------------------------*/
void trace_pre_regs(void);
void trace_Interrupt(int i);
void trace_fmtInvalid(void);
void trace_fmt1(char *opc, int mask);
void trace_fmt2(char *opc, int reg);
void trace_fmt3(char *opc, int arg);
void trace_fmt4_svca(char *opc, int arg);
void trace_fmt4_svcb(char *opc, int arg);
void trace_fmt4_svcc(char *opc, int arg);
void trace_fmt5(char *opc, int dest);
void trace_fmt6(char *opc, int count, int reg);
void trace_fmt7(char *opc, int dmode, int dreg, uint16_t n1word);
void trace_fmt8(char *opc, int sreg, int dreg);
void trace_fmt9(char *opc, int sreg, int dmode, int dreg, uint16_t n1word);
void trace_fmt9_jsr(char *opc, int sreg, int dmode, int dreg, uint16_t n1word);
void trace_fmt9_lea(char *opc, int sreg, int dmode, int dreg, uint16_t n1word);
void trace_fmt9_sob(char *opc, int sreg, int dmode, int dreg);
void trace_fmt10(char *opc, int smode, int sreg, int dmode, int dreg,
                 uint16_t n1word);
void trace_fmt11(char *opc, int sind, int sreg, double s, int dind, int dreg,
                 double d);


#ifdef __cplusplus
}
#endif

#endif
