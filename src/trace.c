/* trace.c           (c) Copyright Mike Noel, 2001-2008              */
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

// following macro used to scan memory AFTER EVERY INSTRUCTION ! !
//                 and dump memory to trace when it finds a problem
// call from utcheck, then only happens when user trace running...

#define memmapck                                                               \
  {                                                                            \
    long TESTEND;                                                              \
    U16 END, LINK, R0, SIZE;                                                   \
    getAMword((unsigned char *)&R0, 0x4E);      /*JOBCUR*/                     \
    getAMword((unsigned char *)&LINK, R0 + 12); /*JOBBAS*/                     \
    getAMword((unsigned char *)&SIZE, R0 + 14); /*JOBSIZ*/                     \
    END = LINK + SIZE;                                                         \
    while (true) {                                                             \
      getAMword((unsigned char *)&SIZE, LINK);                                 \
      if (SIZE == 0)                                                           \
        break;                                                                 \
      TESTEND = SIZE + LINK;                                                   \
      if (((SIZE & 1) != 0) || (TESTEND >= END)) {                             \
        fflush(stderr);                                                        \
        fprintf(stderr, "\n\r\n\r<><><>memory map destroyed<><><>\n\r\n\r");   \
        getAMword((unsigned char *)&LINK, R0 + 12);                            \
        getAMword((unsigned char *)&SIZE, R0 + 14);                            \
        fprintf(stderr, "JOBCUR=%04x, JOBBAS=%04x, JOBSIZ=%04x\n\r\n\r", R0,   \
                LINK, SIZE);                                                   \
        while (true) {                                                         \
          getAMword((unsigned char *)&SIZE, LINK);                             \
          fprintf(stderr, "LINK=%04x, SIZE=%04x\n\r", LINK, SIZE);             \
          if (SIZE == 0)                                                       \
            break;                                                             \
          TESTEND = SIZE + LINK;                                               \
          if (((SIZE & 1) != 0) || (TESTEND >= END)) {                         \
            break;                                                             \
          }                                                                    \
          LINK = TESTEND;                                                      \
        }                                                                      \
        getAMword((unsigned char *)&LINK, R0 + 12);                            \
        getAMword((unsigned char *)&SIZE, R0 + 14);                            \
        config_memdump(LINK, SIZE);                                            \
        regs.halting = true;                                                   \
        break;                                                                 \
      }                                                                        \
      LINK = TESTEND;                                                          \
    };                                                                         \
  }

// macro to suppress trace except when user trace turned on...
// last statement could be 'memmapck;' if above is to be run...

#define utcheck                                                                \
  if (regs.utrace) {                                                           \
    if (regs.PC < regs.utPC)                                                   \
      return;                                                                  \
    getAMword((unsigned char *)&regs.utRX, 0x4E);                              \
    if (regs.utRX != regs.utR0)                                                \
      return;                                                                  \
    /* memmapck; */                                                            \
  }

// define MAXWHERE to cause trace to 'wrap'.  so
// 30000000 means wrap trace at 30mb.

#define MAXWHERE 30000000

void trace_pre_regs() {

#ifdef MAXWHERE
  long where;
  where = ftell(stderr);
  if (where > MAXWHERE) {

    fflush(stderr);
    fseek(stderr, 0, SEEK_SET);
    // regs.halting=1;                  // crash out
    // now...
  }
#endif

  fflush(stderr);
  // fprintf (stderr, "\n\r%06lld\t", line_clock_ticks);
  fprintf(stderr, "\n\r\t");
  fprintf(stderr, "R0=%04x  R1=%04x  R2=%04x  R3=%04x", regs.R0, regs.R1,
          regs.R2, regs.R3);
  fprintf(stderr, "  R4=%04x  R5=%04x  SP=%04x\n\r", regs.R4, regs.R5, regs.SP);
  fprintf(stderr, "\tPS.I2=%d  PS.N=%d   PS.Z=%d   PS.V=%d   PS.C=%d   %09lld",
          regs.PS.I2, regs.PS.N, regs.PS.Z, regs.PS.V, regs.PS.C,
          regs.instcount);
}

void trace_Interrupt(int i) {
  utcheck;
  fprintf(stderr, "\n\r-- Interrupt %d --", i);
}

void trace_fmtInvalid() {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s", opPC, op, "Invalid Op");
}

void trace_fmt1(char *opc, int mask) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s", opPC, op, opc);
  if (op == 11)
    fprintf(stderr, " %04x", mask);
}

void trace_fmt2(char *opc, int reg) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s R%1d", opPC, op, opc, reg);
}

void trace_fmt3(char *opc, int arg) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s %02x", opPC, op, opc, arg);
}

void trace_fmt4_svca(char *opc,
                     int arg) { /* routine to format/print type 4 op code */
  char *cp, chr[1000], *ttyicp, ttyibuf[1000];
  U16 arg2, savePC;

  utcheck;
  trace_pre_regs();

  savePC = regs.PC;

  cp = &chr[0];
  *cp = 0;
  if (arg == 0)
    sprintf(cp, "%s", "LNKCND");
  if (arg == 1)
    sprintf(cp, "%s", "LNKSVC");
  if (arg == 2)
    sprintf(cp, "%s", "TIN");
  if (arg == 3)
    sprintf(cp, "%s", "TOUT");
  if (arg == 4)
    sprintf(cp, "%s", "KBD");
  if (arg == 5)
    sprintf(cp, "%s", "TTY");
  if (arg == 6) {
    ttyicp = &ttyibuf[0];
    *ttyicp = 0;
    arg2 = 0;
    do {
      getAMbyte((unsigned char *)&arg2, regs.PC);
      regs.PC++;
      *ttyicp++ = arg2;
    } while (arg2 != 0);
    if ((regs.PC & 1) == 1)
      regs.PC++;
    ttyicp--;
    ttyicp--;
    arg2 = *ttyicp;
    ttyicp = &ttyibuf[0];
    if (arg2 == ' ') {
      ttyibuf[strlen(ttyicp) - 1] = 0;
      sprintf(cp, "TYPESP <%s>", ttyicp);
    } else if (arg2 == 13) {
      ttyibuf[strlen(ttyicp) - 1] = 0;
      sprintf(cp, "TYPECR <%s>", ttyicp);
    } else
      sprintf(cp, "TYPE <%s>", ttyicp);
  }
  if (arg == 7)
    sprintf(cp, "%s", "TAB");
  if (arg == 8)
    sprintf(cp, "%s", "CRLF");
  if (arg == 9)
    sprintf(cp, "%s", "EXIT");
  if (arg == 10)
    sprintf(cp, "%s", "IODQ");
  if (arg == 11)
    sprintf(cp, "%s", "PACK");
  if (arg == 12)
    sprintf(cp, "%s", "UNPACK");
  if (arg == 13) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "OCVT", arg2);
  }
  if (arg == 14) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "DCVT", arg2);
  }
  if (arg == 15)
    sprintf(cp, "%s", "LIN");
  if (arg == 16)
    sprintf(cp, "%s", "BYP");
  if (arg == 17)
    sprintf(cp, "%s", "ALF");
  if (arg == 18)
    sprintf(cp, "%s", "NUM");
  if (arg == 19)
    sprintf(cp, "%s", "TRM");
  if (arg == 20)
    sprintf(cp, "%s", "GTOCT");
  if (arg == 21)
    sprintf(cp, "%s", "GTDEC");
  if (arg == 22)
    sprintf(cp, "%s", "GTPPN");
  if (arg == 23)
    sprintf(cp, "%s", "TRMICP");
  if (arg == 24)
    sprintf(cp, "%s", "TRMOCP");
  if (arg == 25)
    sprintf(cp, "%s", "TRMBFQ");
  if (arg == 26)
    sprintf(cp, "%s", "QGET");
  if (arg == 27)
    sprintf(cp, "%s", "QRET");
  if (arg == 28)
    sprintf(cp, "%s", "QADD");
  if (arg == 29)
    sprintf(cp, "%s", "QINS");
  if (arg == 30) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "JWAIT", arg2);
  }
  if (arg == 31) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "JRUN", arg2);
  }
  if (arg == 32) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "CTRLC", arg2 + opPC + 4);
  }
  if (arg == 33)
    sprintf(cp, "%s", "TBUF");
  if (arg == 34)
    sprintf(cp, "%s", "HTIM");
  if (arg == 35)
    sprintf(cp, "%s", "SCAN");
  if (arg == 36)
    sprintf(cp, "%s", "TCRT");
  if (arg == 37)
    sprintf(cp, "%s", "BNKSWP");
  if (arg == 38)
    sprintf(cp, "%s", "JLOCK");
  if (arg == 39)
    sprintf(cp, "%s", "JUNLK");
  if (arg == 40) {
    sprintf(cp, "%s", "JWAITC");
  }
  if (arg == 41)
    sprintf(cp, "%s", "WAKE");
  if (arg == 42)
    sprintf(cp, "%s", "RLSE");
  if (arg == 43)
    sprintf(cp, "%s", "RQST");
  if (arg == 44) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "DMADDR", arg2);
  }
  if (arg == 45) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "OUT", arg2);
  }
  if (arg == 46) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "%s %04x", "JWAITU", arg2);
  }

  if (arg == 77) {
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    if (arg2 == 0)
      sprintf(cp, "%s", "UCS");
    if (arg2 == 1)
      sprintf(cp, "%s", "LCS");
    if (arg2 == 2)
      sprintf(cp, "%s", "AMOS");
  }

  if (strlen(cp) > 0)
    fprintf(stderr, "\n\r%04x %04x %s\t\t\tSVC", opPC, op, cp);
  else
    fprintf(stderr, "\n\r%04x %04x %s\t%02x\t\t\tSVC", opPC, op, opc, arg);

  regs.PC = savePC;
}

void trace_fmt4_svcb(char *opc,
                     int arg) { /* routine to format/print type 4 op code */
  char *cp, chr[60], *dp, dhr[10], *ep, ehr[10];
  U16 arg2, aaaa, bbbbbb, cccccc, dddd, mmm, rrr;
  U16 savePC;

  utcheck;
  trace_pre_regs();

  savePC = regs.PC;

  dp = &dhr[0];
  *dp = 0;
  ep = &ehr[0];
  *ep = 0;

  // ====================== begin svcb decode routine, like monitor ===
  // get first word of agruments (PSI)
  getAMword((unsigned char *)&arg2, regs.PC);
  regs.PC += 2;
  // decode PSI
  aaaa = arg2 >> 12;
  bbbbbb = (arg2 >> 6) & 63;
  cccccc = arg2 & 63;
  mmm = (bbbbbb >> 3) & 7;
  rrr = bbbbbb & 7;
  // obtain real args and increment PC as needed
  if ((mmm & 1) == 1) {
    *ep++ = '@';
    *ep-- = 0;
  }
  switch (mmm) {
  case 0:
  case 1:
    if (rrr < 6)
      sprintf(dp, "%sR%d", ep, rrr);
    else if (rrr == 6)
      sprintf(dp, "%sSP", ep);
    else if (rrr > 6)
      sprintf(dp, "%sPC", ep);
    break;
  case 2:
  case 3:
    if (rrr == 7)
      regs.PC += 2;
    if (rrr < 6)
      sprintf(dp, "%s(R%d)+", ep, rrr);
    else if (rrr == 6)
      sprintf(dp, "%s(SP)+", ep);
    else if (rrr > 6)
      sprintf(dp, "%s(PC)+", ep);
    break;
  case 4:
  case 5:
    if (rrr == 7)
      regs.PC -= 2;
    if (rrr < 6)
      sprintf(dp, "%s-(R%d)", ep, rrr);
    else if (rrr == 6)
      sprintf(dp, "%s-(SP)", ep);
    else if (rrr > 6)
      sprintf(dp, "%s-(PC)", ep);
    break;
  case 6:
  case 7:
    getAMword((unsigned char *)&dddd, regs.PC);
    regs.PC += 2;
    if (rrr < 6)
      sprintf(dp, "%s%04x(R%d)", ep, dddd, rrr);
    else if (rrr == 6)
      sprintf(dp, "%s%04x(SP)", ep, dddd);
    else if (rrr > 6)
      sprintf(dp, "%s%04x(PC)", ep, dddd);
  } /* end switch(mmm) */
    // ====================== end of svcb decode routine ================

  cp = &chr[0];
  *cp = 0;
  if (arg == 0) { // file services
    switch (aaaa) {
    case 0:
      sprintf(cp, "INIT\t%s", dp);
      break;
    case 1:
      sprintf(cp, "OPEN\t%s", dp); // ?????????
      break;
    case 2:
      break;
    case 3:
      sprintf(cp, "READ\t%s", dp);
      break;
    case 4:
      sprintf(cp, "WRITE\t%s", dp);
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      sprintf(cp, "WAIT\t%s", dp);
      break;
    case 8:
      break;
    case 9:
      break;
    case 10:
      sprintf(cp, "ASSIGN\t%s", dp);
      break;
    case 11:
      sprintf(cp, "DEASGN\t%s", dp);
      break;
    case 12:
      sprintf(cp, "DSKXXX\t%s", dp); // ?????????
      break;
    case 13:
      sprintf(cp, "LOKCAL  %s", dp); // ?????????
      break;
    case 14:
      break;
    case 15:
      break;
    case 16:
      break;
    case 17:
      break;
    case 18:
      break;
    case 19:
      break;
      //                  default:
    } /* end switch(aaaa) */
  }
  if (arg == 1) {                 // FETCH/SRCH
    sprintf(cp, "FETCH\t%s", dp); // ????????????
  }
  if (arg == 2) {                  // memory services
    sprintf(cp, "XXXMEM\t%s", dp); // ????????????
  }
  if (arg == 3) {                  // pseudo terminal services
    sprintf(cp, "PTYTTY\t%s", dp); // ????????????
  }
  if (arg == 4) {                  // JOBxxx services
    sprintf(cp, "JOBXXX\t%s", dp); // ????????????
  }
  if (arg == 5) {                  // USRxxx services
    sprintf(cp, "USRXXX\t%s", dp); // ????????????
  }
  if (arg == 6) { // SLEEP
    sprintf(cp, "SLEEP\t%s", dp);
  }
  if (arg == 7) { // TTYL
    sprintf(cp, "TTYL\t%s", dp);
  }
  if (arg == 8) { // FILNAM
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "FILNAM\t%s", dp);
  }
  if (arg == 9) { // FSPEC
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2;
    sprintf(cp, "FSPEC\t%s", dp);
  }
  if (arg == 10) { // PFILE
    sprintf(cp, "PFILE\t%s", dp);
  }
  if (arg == 11) { // PRNAM
    sprintf(cp, "PRNAM\t%s", dp);
  }
  if (arg == 12) { // PRPPN
    sprintf(cp, "PRPPN\t%s", dp);
  }
  if (arg == 14) { // PCALL
    sprintf(cp, "PCALL\t%s", dp);
  }
  if (arg == 15) { // ERRMSG
    getAMword((unsigned char *)&arg2, regs.PC);
    regs.PC += 2; // ????????????
    sprintf(cp, "ERRMSG\t%s", dp);
  }

  if (strlen(cp) > 0)
    fprintf(stderr, "\n\r%04x %04x %s\t\t\tSVC", opPC, op, cp);
  else
    fprintf(stderr, "\n\r%04x %04x %s\t%02x\t\t\tSVC", opPC, op, opc, arg);

  regs.PC = savePC;
}

void trace_fmt4_svcc(char *opc,
                     int arg) { /* routine to format/print type 4 op code */
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s\t%02x", opPC, op, opc, arg);
}

void trace_fmt5(char *opc, int dest) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s %d (%04x)", opPC, op, opc, (2 * dest),
          (regs.PC + (2 * dest)));
}

void trace_fmt6(char *opc, int count, int reg) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s %04d,R%d", opPC, op, opc, count, reg);
}

void trace_fmt7(char *opc, int dmode, int dreg,
                U16 n1word) { /* routine to format/print type 7 op code */
  char *cp, chr[40], *dr, dchr[4];

  utcheck;
  trace_pre_regs();

  dr = &dchr[0];
  *dr = 0;
  if (dreg < 6)
    sprintf(dr, "R%d", dreg);
  if (dreg == 6)
    sprintf(dr, "SP");
  if (dreg == 7)
    sprintf(dr, "PC");

  cp = &chr[0];
  *cp = 0;
  if (dmode == 0)
    sprintf(cp, "%s", dr);
  if (dmode == 1)
    sprintf(cp, "@%s", dr);
  if (dmode == 2)
    sprintf(cp, "(%s)+", dr);
  if (dmode == 3)
    sprintf(cp, "@(%s)+", dr);
  if (dmode == 4)
    sprintf(cp, "-(%s)", dr);
  if (dmode == 5)
    sprintf(cp, "@-(%s)", dr);
  if (dmode == 6)
    sprintf(cp, "%04x(%s)", n1word, dr);
  if (dmode == 7)
    sprintf(cp, "@%04x(%s)", n1word, dr);

  cp = &chr[0];
  fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt8(char *opc, int sreg, int dreg) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s\tR%d,R%d", opPC, op, opc, sreg, dreg);
}

void trace_fmt9(char *opc, int sreg, int dmode, int dreg,
                U16 n1word) { /* routine to format/print type 9 op code */
  char *cp, chr[40], *dr, dchr[4], *sr, schr[4];

  utcheck;
  trace_pre_regs();

  sr = &schr[0];
  *sr = 0;
  if (sreg < 6)
    sprintf(sr, "R%d", sreg);
  if (sreg == 6)
    sprintf(sr, "SP");
  if (sreg == 7)
    sprintf(sr, "PC");

  dr = &dchr[0];
  *dr = 0;
  if (dreg < 6)
    sprintf(dr, "R%d", dreg);
  if (dreg == 6)
    sprintf(dr, "SP");
  if (dreg == 7)
    sprintf(dr, "PC");

  cp = &chr[0];
  *cp = 0;
  if (dmode == 0)
    sprintf(cp, "%s,%s", sr, dr);
  if (dmode == 1)
    sprintf(cp, "%s,@%s", sr, dr);
  if (dmode == 2)
    sprintf(cp, "%s,(%s)+", sr, dr);
  if (dmode == 3)
    sprintf(cp, "%s,@(%s)+", sr, dr);
  if (dmode == 4)
    sprintf(cp, "%s,-(%s)", sr, dr);
  if (dmode == 5)
    sprintf(cp, "%s,@-(%s)", sr, dr);
  if (dmode == 6)
    sprintf(cp, "%s,%04x(%s)", sr, n1word, dr);
  if (dmode == 7)
    sprintf(cp, "%s,@%04x(%s)", sr, n1word, dr);

  cp = &chr[0];
  fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt9_jsr(char *opc, int sreg, int dmode, int dreg,
                    U16 n1word) { /* routine to format/print type 9 op code */
  char *cp, chr[40], *dr, dchr[4], *sr, schr[4];

  utcheck;
  trace_pre_regs();

  sr = &schr[0];
  *sr = 0;
  if (sreg < 6)
    sprintf(sr, "R%d", sreg);
  if (sreg == 6)
    sprintf(sr, "SP");
  if (sreg == 7)
    sprintf(sr, "PC");

  dr = &dchr[0];
  *dr = 0;
  if (dreg < 6)
    sprintf(dr, "R%d", dreg);
  if (dreg == 6)
    sprintf(dr, "SP");
  if (dreg == 7)
    sprintf(dr, "PC");

  cp = &chr[0];
  *cp = 0;
  if (dmode == 0)
    sprintf(cp, "%s,%s", sr, dr);
  if (dmode == 1)
    sprintf(cp, "%s,@%s", sr, dr);
  if (dmode == 2)
    sprintf(cp, "%s,(%s)+", sr, dr);
  if (dmode == 3)
    sprintf(cp, "%s,@(%s)+", sr, dr);
  if (dmode == 4)
    sprintf(cp, "%s,-(%s)", sr, dr);
  if (dmode == 5)
    sprintf(cp, "%s,@-(%s)", sr, dr);
  if (dmode == 6)
    sprintf(cp, "%s,%04x(%s)", sr, n1word, dr);
  if (dmode == 7)
    sprintf(cp, "%s,@%04x(%s)", sr, n1word, dr);

  cp = &chr[0];
  if (sreg == 7)
    fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, "CALL", &cp[3]);
  else
    fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt9_lea(char *opc, int sreg, int dmode, int dreg,
                    U16 n1word) { /* routine to format/print type 9 op code */
  char *cp, chr[40], *dr, dchr[4], *sr, schr[4];

  utcheck;
  trace_pre_regs();

  sr = &schr[0];
  *sr = 0;
  if (sreg < 6)
    sprintf(sr, "R%d", sreg);
  if (sreg == 6)
    sprintf(sr, "SP");
  if (sreg == 7)
    sprintf(sr, "PC");

  dr = &dchr[0];
  *dr = 0;
  if (dreg < 6)
    sprintf(dr, "R%d", dreg);
  if (dreg == 6)
    sprintf(dr, "SP");
  if (dreg == 7)
    sprintf(dr, "PC");

  cp = &chr[0];
  *cp = 0;
  if (dmode == 0)
    sprintf(cp, "%s,%s", sr, dr);
  if (dmode == 1)
    sprintf(cp, "%s,@%s", sr, dr);
  if (dmode == 2)
    sprintf(cp, "%s,(%s)+", sr, dr);
  if (dmode == 3)
    sprintf(cp, "%s,@(%s)+", sr, dr);
  if (dmode == 4)
    sprintf(cp, "%s,-(%s)", sr, dr);
  if (dmode == 5)
    sprintf(cp, "%s,@-(%s)", sr, dr);
  if (dmode == 6)
    sprintf(cp, "%s,%04x(%s)", sr, n1word, dr);
  if (dmode == 7)
    sprintf(cp, "%s,@%04x(%s)", sr, n1word, dr);

  cp = &chr[0];
  if (sreg == 7)
    fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, "JMP", &cp[3]);
  else
    fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt9_sob(char *opc, int sreg, int dmode,
                    int dreg) { /* routine to format/print type 9 op code */
  char *cp, chr[40], *sr, schr[4];
  int dest, desta;

  utcheck;
  trace_pre_regs();

  sr = &schr[0];
  *sr = 0;
  if (sreg < 6)
    sprintf(sr, "R%d", sreg);
  if (sreg == 6)
    sprintf(sr, "SP");
  if (sreg == 7)
    sprintf(sr, "PC");

  dest = ((dmode << 3) + dreg) << 1;
  desta = regs.PC - dest;

  cp = &chr[0];
  *cp = 0;
  sprintf(cp, "%s,-%d  (%04x)", sr, dest, desta);
  cp = &chr[0];
  fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt10(char *opc, int smode, int sreg, int dmode, int dreg,
                 U16 n1word) { /* routine to format/print type 10 op code */
  int lstr;
  char *cp, chr[40], *sr, schr[4], *dr, dchr[4];
  U16 n2word;

  utcheck;
  trace_pre_regs();

  if (dmode > 5) {
    n2word = getAMwordBYmode(sreg, smode, n1word);
    getAMword((unsigned char *)&n2word, regs.PC);
    undAMwordBYmode(sreg, smode);
  }

  sr = &schr[0];
  *sr = 0;
  if (sreg < 6)
    sprintf(sr, "R%d", sreg);
  if (sreg == 6)
    sprintf(sr, "SP");
  if (sreg == 7)
    sprintf(sr, "PC");

  dr = &dchr[0];
  *dr = 0;
  if (dreg < 6)
    sprintf(dr, "R%d", dreg);
  if (dreg == 6)
    sprintf(dr, "SP");
  if (dreg == 7)
    sprintf(dr, "PC");

  cp = &chr[0];
  *cp = 0;
  if (smode == 0)
    sprintf(cp, "%s", sr);
  if (smode == 1)
    sprintf(cp, "@%s", sr);
  if (smode == 2)
    sprintf(cp, "(%s)+", sr);
  if (smode == 3)
    sprintf(cp, "@(%s)+", sr);
  if (smode == 4)
    sprintf(cp, "-(%s)", sr);
  if (smode == 5)
    sprintf(cp, "@-(%s)", sr);
  if (smode == 6)
    sprintf(cp, "%04x(%s)", n1word, sr);
  if (smode == 7)
    sprintf(cp, "@%04x(%s)", n1word, sr);

  lstr = strlen(cp);
  cp = &chr[lstr];
  if (dmode == 0)
    sprintf(cp, ",%s", dr);
  if (dmode == 1)
    sprintf(cp, ",@%s", dr);
  if (dmode == 2)
    sprintf(cp, ",(%s)+", dr);
  if (dmode == 3)
    sprintf(cp, ",@(%s)+", dr);
  if (dmode == 4)
    sprintf(cp, ",-(%s)", dr);
  if (dmode == 5)
    sprintf(cp, ",@-(%s)", dr);
  if (dmode == 6)
    sprintf(cp, ",%04x(%s)", n2word, dr);
  if (dmode == 7)
    sprintf(cp, ",@%04x(%s)", n2word, dr);

  cp = &chr[0];
  fprintf(stderr, "\n\r%04x %04x %s  %s", opPC, op, opc, cp);
}

void trace_fmt11(char *opc, int sind, int sreg, double s, int dind, int dreg,
                 double d) {
  utcheck;
  trace_pre_regs();
  fprintf(stderr, "\n\r%04x %04x %s\t%d,\tR%d,\t%f, \t%d,\tR%d, \t%f", opPC, op,
          opc, sind, sreg, s, dind, dreg, d);
}
