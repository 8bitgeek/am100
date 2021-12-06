/* memory.c       (c) Copyright Mike Noel, 2001-2008                 */
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

/*-------------------------------------------------------------------*/
/* This module does all memory access and control.  Since the AM100  */
/* uses memory mapped I/O this module is also the only place from    */
/* which the routines in the io.c module are called.                 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Initialize Memory control blocks                                  */
/*-------------------------------------------------------------------*/
void initAMmemory() {
  int i;

  for (i = 0; i < 256; i++) /* turn off all memory */
  {
    am_membase[i] = NULL;
    am_memowns[i] = 0;
    am_memflags[i] = 0;
  }
}

/*-------------------------------------------------------------------*/
/* shutdown memory                                                   */
/*-------------------------------------------------------------------*/
void memory_stop() {}

/*-------------------------------------------------------------------*/
/* put memory back into initial configuration                        */
/*-------------------------------------------------------------------*/
void memory_reset() {
  CARDS *cptr;
  unsigned char pinit, zero = 0;

  cptr = cards;
  do { // turn all memory off
    if (cptr->C_Type == C_Type_ram) {
      ram_Port(&zero, 1, (unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_rom) {
      rom_Port(&zero, 1, (unsigned char *)cptr);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);

  cptr = cards;
  do { // turn some memory back on
    if (cptr->C_Type == C_Type_ram) {
      pinit = cptr->CType.RAM.pinit;
      ram_Port(&pinit, 1, (unsigned char *)cptr);
    }
    if (cptr->C_Type == C_Type_rom) {
      pinit = cptr->CType.ROM.pinit;
      rom_Port(&pinit, 1, (unsigned char *)cptr);
    }
    cptr = cptr->CARDS_NEXT;
  } while (cptr != NULL);
}

/*-------------------------------------------------------------------*/
/* swap memory into AM 64k address space                             */
/* --- called by memory card init and I/O routines.                  */
/*-------------------------------------------------------------------*/
void swapInAMmemory(unsigned char *cardaddr, int cardport,
                    unsigned char cardflags, /* all same for numpage */
                    int numpage, U16 amaddr) {
  int i, j;
  unsigned char *cb;

  for (i = 0; i < numpage; i++) {
    j = i + (amaddr / 256);
    cb = cardaddr + (i * 256);
    am_membase[j] = cb;
    am_memowns[j] = cardport;
    am_memflags[j] = cardflags;
  }
}

/*-------------------------------------------------------------------*/
/* swap memory out of AM 64k address space                           */
/* --- called by memory card init and I/O routines.                  */
/*-------------------------------------------------------------------*/
void swapOutAMmemory(unsigned char *cardaddr, /* not used */
                     int cardport,            /* verify swapout for this card */
                     unsigned char cardflags, /* not used */
                     int numpage, U16 amaddr) {
  int i, j;

  for (i = 0; i < numpage; i++) {
    j = i + (amaddr / 256);
    if (am_memowns[j] == cardport) {
      am_membase[j] = NULL;
      am_memowns[j] = 0;
      am_memflags[j] = 0;
    }
  }
}

/*-------------------------------------------------------------------*/
/* Read Byte from Memory (or I/O port)                               */
/*-------------------------------------------------------------------*/
void getAMbyte(unsigned char *chr, long address) {
  unsigned char *cp;
  unsigned long ADDR, PAGE, PORT;

  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;

  switch (PAGE) {
  case 255: /* it is doing I/O */
    getAMIObyte(chr, PORT);
    break;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      *chr = 255;
    else /* memory exists */
    {
      cp += PORT;
      *chr = *cp; /* move data from caller to memory */
    }
  }
}

/*-------------------------------------------------------------------*/
/* Read Word from Memory (or I/O port)                               */
/*                                                                   */
/* "For word operations, the AM-100 uses only the low-order half     */
/* of the word during a write operation. During a read, the I/O      */
/* byte is replicated in both halves of the resulting word."         */
/*                                                                   */
/* "The AM-100T, on the other hand, ..."                             */
/*                                                                   */
/* This emulator does it the AM-100 (not T) way...                   */
/*-------------------------------------------------------------------*/
void getAMword(unsigned char *chr, long address) {
  unsigned char *cp;
  unsigned long ADDR, PAGE, PORT;

  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;
  if (PAGE != 255) {
    address &= 0xfffe;
    ADDR = address & 0x0ffff;
    PAGE = ADDR / 256;
    PORT = ADDR % 256;
  }

#if AM_BYTE_ORDER == AM_BIG_ENDIAN
  chr++;
#endif

  switch (PAGE) {
  case 255: /* it is doing I/O */
    getAMIObyte(chr, PORT);
    *(chr + 1) = *chr;
    return;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      *chr = 255;
    else /* memory exists */
    {
      cp += PORT;
      *chr = *cp; /* move data from caller to memory */
    }
  }

#if AM_BYTE_ORDER == AM_BIG_ENDIAN
  chr--;
#else
  chr++;
#endif

  address++;
  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;

  switch (PAGE) {
  case 255: /* it is doing I/O */
    getAMIObyte(chr, PORT);
    break;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      *chr = 255;
    else /* memory exists */
    {
      cp += PORT;
      *chr = *cp; /* move data from caller to memory */
    }
  }
}

/*-------------------------------------------------------------------*/
/* Write Byte to Memory (or I/O port)                                */
/*-------------------------------------------------------------------*/
void putAMbyte(unsigned char *chr, long address) {
  unsigned char *cp;
  unsigned long ADDR, PAGE, PORT;

  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;

  switch (PAGE) {
  case 255: /* it is doing I/O */
    putAMIObyte(chr, PORT);
    break;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      return;
    else /* memory exists */
    {
      cp += PORT;
      if (am_memflags[PAGE] && am$mem$flag_writable)
        *cp = *chr; /* move data from memory to caller  */
    }
  }
}

/*-------------------------------------------------------------------*/
/* Write Word to Memory (or I/O port)                                */
/* "For word operations, the AM-100 uses only the low-order half     */
/* of the word during a write operation. During a read, the I/O      */
/* byte is replicated in both halves of the resulting word."         */
/*                                                                   */
/* "The AM-100T, on the other hand, ..."                             */
/*                                                                   */
/* This emulator does it the AM-100 (not T) way...                   */
/*-------------------------------------------------------------------*/
void putAMword(unsigned char *chr, long address) {
  unsigned char *cp;
  unsigned long ADDR, PAGE, PORT;

  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;
  if (PAGE != 255) {
    address &= 0xfffe;
    ADDR = address & 0x0ffff;
    PAGE = ADDR / 256;
    PORT = ADDR % 256;
  }

#if AM_BYTE_ORDER == AM_BIG_ENDIAN
  chr++;
#endif

  switch (PAGE) {
  case 255: /* it is doing I/O */
    putAMIObyte(chr, PORT);
    return;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      return;
    else /* memory exists */
    {
      cp += PORT;
      if (am_memflags[PAGE] && am$mem$flag_writable)
        *cp = *chr; /* move data from memory to caller  */
    }
  }

#if AM_BYTE_ORDER == AM_BIG_ENDIAN
  chr--;
#else
  chr++;
#endif

  address++;
  ADDR = address & 0x0ffff;
  PAGE = ADDR / 256;
  PORT = ADDR % 256;

  switch (PAGE) {
  case 255: /* it is doing I/O */
    putAMIObyte(chr, PORT);
    break;
  default:
    cp = am_membase[PAGE];
    if (cp == NULL) /* then no memory there! */
      return;
    else /* memory exists */
    {
      cp += PORT;
      if (am_memflags[PAGE] && am$mem$flag_writable)
        *cp = *chr; /* move data from memory to caller  */
    }
  }
}

/*-------------------------------------------------------------------*/
/* given register number, addressing mode, and offset returns word   */
/*-------------------------------------------------------------------*/
U16 getAMwordBYmode(int regnum, int mode, int offset) {
  U16 tmp;
  U16 tmp2;

  switch (mode) {
  case 0:
    return (regs.gpr[regnum]);
  case 1:
    getAMword((unsigned char *)&tmp, regs.gpr[regnum]);
    return (tmp);
  case 2:
    getAMword((unsigned char *)&tmp, regs.gpr[regnum]);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    return (tmp);
  case 3:
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    getAMword((unsigned char *)&tmp, tmp2);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    return (tmp);
  case 4:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp, regs.gpr[regnum]);
    return (tmp);
  case 5:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    getAMword((unsigned char *)&tmp, tmp2);
    return (tmp);
  case 6:
    getAMword((unsigned char *)&tmp, offset + regs.gpr[regnum]);
    return (tmp);
  case 7:
    getAMword((unsigned char *)&tmp2, offset + regs.gpr[regnum]);
    getAMword((unsigned char *)&tmp, tmp2);
    return (tmp);
  default:
    assert("memory.c - invalid mode in GetAMwordBYmode");
  }
  return 0;
}

/*-------------------------------------------------------------------*/
/* given register number, addressing mode, and offset return address */
/*-------------------------------------------------------------------*/
U16 getAMaddrBYmode(int regnum, int mode, int offset) {
  U16 tmp;

  switch (mode) {
  case 0:
    return (0);
  case 1:
    tmp = regs.gpr[regnum];
    return (tmp);
  case 2:
    tmp = regs.gpr[regnum];
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    return (tmp);
  case 3:
    getAMword((unsigned char *)&tmp, regs.gpr[regnum]);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    return (tmp);
  case 4:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    tmp = regs.gpr[regnum];
    return (tmp);
  case 5:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp, regs.gpr[regnum]);
    return (tmp);
  case 6:
    tmp = offset + regs.gpr[regnum];
    return (tmp);
  case 7:
    getAMword((unsigned char *)&tmp, offset + regs.gpr[regnum]);
    return (tmp);
  default:
    assert("memory.c - invalid mode in GetAMaddrBYmode");
  }
  return 0;
}

/*-------------------------------------------------------------------*/
/* given register number, addressing mode, and offset stores word    */
/*-------------------------------------------------------------------*/
void putAMwordBYmode(int regnum, int mode, int offset, U16 theword) {
  U16 tmp2;

  switch (mode) {
  case 0:
    regs.gpr[regnum] = theword;
    break;
  case 1:
    putAMword((unsigned char *)&theword, regs.gpr[regnum]);
    break;
  case 2:
    putAMword((unsigned char *)&theword, regs.gpr[regnum]);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    break;
  case 3:
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    putAMword((unsigned char *)&theword, tmp2);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    break;
  case 4:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    putAMword((unsigned char *)&theword, regs.gpr[regnum]);
    break;
  case 5:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    putAMword((unsigned char *)&theword, tmp2);
    break;
  case 6:
    putAMword((unsigned char *)&theword, offset + regs.gpr[regnum]);
    break;
  case 7:
    getAMword((unsigned char *)&tmp2, offset + regs.gpr[regnum]);
    putAMword((unsigned char *)&theword, tmp2);
    break;
  default:
    assert("memory.c - invalid mode in PutAMwordBYmode");
  }
}

/*-------------------------------------------------------------------*/
/* undoes pre and post increments                                    */
/*-------------------------------------------------------------------*/
void undAMwordBYmode(int regnum, int mode) {
  switch (mode) {
  case 2:
  case 3:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    break;
  case 4:
  case 5:
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    //    default:
  }
}

/*-------------------------------------------------------------------*/
/* given register number, addressing mode, and offset returns byte   */
/*-------------------------------------------------------------------*/
U8 getAMbyteBYmode(int regnum, int mode, int offset) {
  U8 tmp;
  U16 tmp2;

  switch (mode) {
  case 0:
    return ((regs.gpr[regnum] & 0xff));
  case 1:
    getAMbyte((unsigned char *)&tmp, regs.gpr[regnum]);
    return (tmp);
  case 2:
    getAMbyte((unsigned char *)&tmp, regs.gpr[regnum]);
    regs.gpr[regnum]++;
    if (regnum > 5)
      regs.gpr[regnum]++;
    return (tmp);
  case 3:
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    getAMbyte((unsigned char *)&tmp, tmp2);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    return (tmp);
  case 4:
    regs.gpr[regnum]--;
    if (regnum > 5)
      regs.gpr[regnum]--;
    getAMbyte((unsigned char *)&tmp, regs.gpr[regnum]);
    return (tmp);
  case 5:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    getAMbyte((unsigned char *)&tmp, tmp2);
    return (tmp);
  case 6:
    getAMbyte((unsigned char *)&tmp, offset + regs.gpr[regnum]);
    return (tmp);
  case 7:
    getAMword((unsigned char *)&tmp2, offset + regs.gpr[regnum]);
    getAMbyte((unsigned char *)&tmp, tmp2);
    return (tmp);
  default:
    assert("memory.c - invalid mode in GetAMbyteBYmode");
  }
  return 0;
}

/*-------------------------------------------------------------------*/
/* given register number, addressing mode, and offset stores byte    */
/*-------------------------------------------------------------------*/
void putAMbyteBYmode(int regnum, int mode, int offset, U8 thebyte) {
  U16 tmp2;

  switch (mode) {
  case 0:
    regs.gpr[regnum] = (regs.gpr[regnum] & 0xff00) | thebyte;
    break;
  case 1:
    putAMbyte((unsigned char *)&thebyte, regs.gpr[regnum]);
    break;
  case 2:
    putAMbyte((unsigned char *)&thebyte, regs.gpr[regnum]);
    regs.gpr[regnum]++;
    if (regnum > 5)
      regs.gpr[regnum]++;
    break;
  case 3:
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    putAMbyte((unsigned char *)&thebyte, tmp2);
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    break;
  case 4:
    regs.gpr[regnum]--;
    if (regnum > 5)
      regs.gpr[regnum]--;
    putAMbyte((unsigned char *)&thebyte, regs.gpr[regnum]);
    break;
  case 5:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    getAMword((unsigned char *)&tmp2, regs.gpr[regnum]);
    putAMbyte((unsigned char *)&thebyte, tmp2);
    break;
  case 6:
    putAMbyte((unsigned char *)&thebyte, offset + regs.gpr[regnum]);
    break;
  case 7:
    getAMword((unsigned char *)&tmp2, offset + regs.gpr[regnum]);
    putAMbyte((unsigned char *)&thebyte, tmp2);
    break;
  default:
    assert("memory.c - invalid mode in PutAMbyteBYmode");
  }
}

/*-------------------------------------------------------------------*/
/* undoes pre and post increments                                    */
/*-------------------------------------------------------------------*/
void undAMbyteBYmode(int regnum, int mode) {
  switch (mode) {
  case 2:
    regs.gpr[regnum]--;
    if (regnum > 5)
      regs.gpr[regnum]--;
    break;
  case 3:
    regs.gpr[regnum]--;
    regs.gpr[regnum]--;
    break;
  case 4:
    regs.gpr[regnum]++;
    if (regnum > 5)
      regs.gpr[regnum]++;
    break;
  case 5:
    regs.gpr[regnum]++;
    regs.gpr[regnum]++;
    break;
    //    default:
  }
}

/*-------------------------------------------------------------------*/
/* Move Byte in Memory                                               */
/*-------------------------------------------------------------------*/
void moveAMbyte(long to, long from) {
  unsigned char temp;

  getAMbyte(&temp, from);
  putAMbyte(&temp, to);
}

/*-------------------------------------------------------------------*/
/* Move Word in Memory                                               */
/*-------------------------------------------------------------------*/
void moveAMword(long to, long from) {
  unsigned char temp[2];

  getAMword(&temp[1], from);
  putAMword(&temp[1], to);
}
