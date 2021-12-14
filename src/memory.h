/* memory.h      (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_MEMORY_H__
#define __AM100_MEMORY_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------*/
/* in memory.c                                                       */
/*-------------------------------------------------------------------*/
void initAMmemory(void);
void memory_stop(void);
void memory_reset(void);
void swapInAMmemory(unsigned char *cardaddr, int cardport,
                    unsigned char cardflags, /* all same for numpage */
                    int numpage, uint16_t amaddr);
void swapOutAMmemory(unsigned char *cardaddr, /* not used */
                     int cardport,            /* verify swapout for this card */
                     unsigned char cardflags, /* not used */
                     int numpage, uint16_t amaddr);
void getAMbyte(unsigned char *chr, long address);
void getAMword(unsigned char *chr, long address);
void putAMbyte(unsigned char *chr, long address);
void putAMword(unsigned char *chr, long address);
void moveAMbyte(long to, long from);
void moveAMword(long to, long from);
uint16_t getAMaddrBYmode(int regnum, int mode, int offset);
uint16_t getAMwordBYmode(int regnum, int mode, int offset);
void putAMwordBYmode(int regnum, int mode, int offset, uint16_t theword);
void undAMwordBYmode(int regnum, int mode);
uint8_t getAMbyteBYmode(int regnum, int mode, int offset);
void putAMbyteBYmode(int regnum, int mode, int offset, uint8_t thebyte);
void undAMbyteBYmode(int regnum, int mode);


#ifdef __cplusplus
}
#endif

#endif
