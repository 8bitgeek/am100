/* rom.c       (c) Copyright Mike Noel, 2001-2008                    */
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
#include "rom.h"
#include "io.h"
#include "memory.h"

/*-------------------------------------------------------------------*/
/* This module simulates a read-only memory board.  It handles       */
/* initial setup and responds to IO, reset, and phantom signals.     */
/*                                                                   */
/* rom  port=40 pinit=00000000 \             */
/*  phantom=off \                */
/*  size=8 \                 */
/*  base0=e000 base1=e400 base2=e800 base3=ec00 \        */
/*  base4=f000 base5=f400 base6=f800 base7=fc00 \        */
/*  image=bsave8.rom               */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize the board emulator                                     */
/*-------------------------------------------------------------------*/
void rom_Init(unsigned int port, /* port 0 means not bank switched */
              unsigned int pinit, unsigned int phantom, unsigned int size,
              long base[8], unsigned char *image) {
  FILE *fp;
  CARDS *cptr;
  int i;
  /* get card memory */
  cptr = (CARDS *)malloc(sizeof(CARDS));
  if (cptr == NULL)
    assert("rom.c - failed to obtain card memory");

  /* fill in type and parameters */
  cptr->C_Type = C_Type_rom;
  cptr->CType.ROM.port = port;
  cptr->CType.ROM.pinit = pinit;
  cptr->CType.ROM.plast = pinit;
  cptr->CType.ROM.size = size;
  for (i = 0; i < 8; i++)
    cptr->CType.ROM.base[i] = base[i];

  /* get ram to simulate rom */
  cptr->CType.ROM.mem = (uint8_t *)malloc(size * 1024);
  if (cptr->CType.ROM.mem == NULL)
    assert("rom.c - failed to obtain rom memory");

  /* load the image file */
  fp = fopen((char *)image, "r");
  if (fp == NULL)
    assert("rom.c - failed to open image file");
  if (fread(cptr->CType.ROM.mem, size * 1024, 1, fp) == 0)
    if (ferror(fp))
      assert("rom.c - error reading image file");
  fclose(fp);

  /* if port not zero register it */
  if (port != 0)
    regAMport(port, &rom_Port, (unsigned char *)cptr);

  /* call rom_Port directly (even if port = 0) to map initial memory */
  rom_Port((unsigned char *)&pinit, 1, (unsigned char *)cptr);

  /* everything worked so put card on card chain */
  cptr->CARDS_NEXT = am100_state.cards;
  am100_state.cards = cptr;
}

/*-------------------------------------------------------------------*/
/* Port Handler                                                      */
/*-------------------------------------------------------------------*/
void rom_Port(unsigned char *chr, int rwflag, unsigned char *sa) {
  CARDS *cptr;
  int i, j, k, num_blks;

  cptr = (CARDS *)sa;
  switch (rwflag) {

  case 0: /* read, returns the last write value */
    *chr = cptr->CType.ROM.plast;
    break;

  case 1: /* write, save value & turns banked memory on or off */
    cptr->CType.ROM.plast = *chr;

    num_blks = cptr->CType.ROM.size / 2; /* actually should be size */
    for (i = 0, j = *chr; i < 8; i++)    /* divided by 8 (=k) times */
    {                                    /* 4 (cvt k to 265) =2!    */
      k = (j >> (7 - i)) & 1;
      if (k == 0) { /* turning memory off (but not in initial config!) */
        swapOutAMmemory(0, cptr->CType.ROM.port, 0, num_blks,
                        cptr->CType.ROM.base[i]);
      }
    }
    for (i = 0, j = *chr; i < 8; i++) /* need two passes (off,on) */
    {                                 /* 'cause may be more than */
      k = (j >> (7 - i)) & 1;         /* one bank on a board     */
      if (k != 0) {                   /* turning memory on */
        swapInAMmemory(cptr->CType.ROM.mem + (i * num_blks * 256),
                       cptr->CType.ROM.port, 0, num_blks,
                       cptr->CType.ROM.base[i]);
      }
    }
    break;

  default:
    assert("rom.c - invalid rwflag arg to rom_Port...");
  }
}
