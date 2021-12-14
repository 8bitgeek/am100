/* ram.c       (c) Copyright Mike Noel, 2001-2008                    */
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
#include "ram.h"
#include "io.h"
#include "memory.h"

/*-------------------------------------------------------------------*/
/* This module simulates a read-write memory board.  It handles      */
/* initial setup and responds to IO, reset, and phantom signals.     */
/*                                                                   */
/* ram  port=41 pinit=00000000 \             */
/*  phantom=off \                */
/*  size=32 \                */
/*  base0=8000 base1=9000 base2=a000 base3=b000 \        */
/*  base4=c000 base5=d000 base6=e000 base7=f000        */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void ram_Init(unsigned int port, /* port 0 means not bank switched */
              unsigned int pinit, unsigned int phantom, unsigned int size,
              long base[8]) {
  CARDS *cptr;
  int i;
  unsigned char p;
  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("ram.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_ram;
  cptr->CType.RAM.port = port;
  cptr->CType.RAM.pinit = pinit;
  cptr->CType.RAM.plast = pinit;
  cptr->CType.RAM.size = size;
  for (i = 0; i < 8; i++)
    cptr->CType.RAM.base[i] = base[i];

  /* get ram to simulate */
  cptr->CType.RAM.mem = (uint8_t *)malloc(size * 1024);
  if (cptr->CType.RAM.mem == NULL)
    assert("ram.c - failed to obtain ram memory");

  /* if port not zero register it */
  if (port != 0)
    regAMport(port, &ram_Port, (unsigned char *)cptr);

  /* call ram_Port directly (even if port = 0) to map initial memory */
  // m_Port ((unsigned char *) &pinit, 1, (unsigned char *) cptr);
  p = pinit;
  ram_Port(&p, 1, (unsigned char *)cptr);

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = am100_state.cards;
  am100_state.cards = cptr;
}

/*-------------------------------------------------------------------*/
/* Port Handler                                                      */
/*-------------------------------------------------------------------*/
void ram_Port(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int i, j, k, num_blks;

  cptr = (CARDS *)sa;
  switch (rwflag) {

  case 0: /* read, returns the last write value */
    *chr = cptr->CType.RAM.plast;
    break;

  case 1: /* write, save value & turns banked memory on or off */
    cptr->CType.RAM.plast = *chr;

    num_blks = cptr->CType.RAM.size / 2; /* actually should be size */
    for (i = 0, j = *chr; i < 8; i++)    /* divided by 8 (=k) times */
    {                                    /* 4 (cvt k to 265) =2!    */
      k = (j >> (7 - i)) & 1;
      if (k == 0) { /* turning memory off (but not in initial config!) */
        swapOutAMmemory(0, cptr->CType.RAM.port, 0, num_blks,
                        cptr->CType.RAM.base[i]);
      }
    }
    for (i = 0, j = *chr; i < 8; i++) /* need two passes (off,on) */
    {                                 /* 'cause may be more than */
      k = (j >> (7 - i)) & 1;         /* one bank on a board     */
      if (k != 0) {                   /* turning memory on */
        swapInAMmemory(cptr->CType.RAM.mem + (i * num_blks * 256),
                       cptr->CType.RAM.port, am$mem$flag_writable, num_blks,
                       cptr->CType.RAM.base[i]);
      }
    }
    break;

  default:
    assert("ram.c - invalid rwflag arg to ram_Port...");
  }
}
