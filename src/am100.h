/* am100.h       (c) Copyright Mike Noel, 2001-2008                  */
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
#ifndef __AM100_H__
#define __AM100_H__

#include <wd16.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>

#include <menu.h>
#include <ncurses.h>
#include <panel.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------------------*/
/* Card configuration block              */
/*-------------------------------------------------------------------*/

typedef struct _PANEL_DATA {
  struct _PANEL_DATA *PANEL_DATA_NEXT;
  PANEL *thePanel;
  long long telnet_lct;       // telnet keep alive line clock ticker
  unsigned char telnet_ip[4]; // telnet IP address for each port
  int teloutbufcnt;           // telnet output buffer count
  char teloutbuf[32];         // telnet output buffer
  int hide, fnum, theFG, theBG, sd;
  int theoutcnt;
  char theoutbuf[10];
} PANEL_DATA;

typedef struct _card_am300 { // alphamicro am-300 6 port serial
  int port;
  int intlvl;
  int lastout[5];              // last value written to port
  int IF[6];                   // interrupt flags for each channel
  unsigned char inbuf[128][6]; // input buffer for each channel
  int inptr[6], inptr2[6], incnt[6];
  unsigned char outbuf[128][6]; // output buffer for each channel
  int outptr[6], outptr2[6], outcnt[6];
  PANEL *thePanel[6]; // panels for each channel
  PANEL_DATA panel_data[6];
} card_am300;

typedef struct _card_am320 { // alphamicro am-320 line printer
  int port;
  int delay, tp, cntdown;
  FILE *file;
  char filename[255];
} card_am320;

#define MAXTAPES 8

typedef struct _card_am600 { // alphamicro am-600 mag tape
  int port;
  int mtlvl;
  int LSTr3;
  int CNTr4;
  int DMAr4;
  int CNTr5;
  int DMAr5;
  int which;
  FILE *file[MAXTAPES];
  char filename[MAXTAPES][255];
  int filerw[MAXTAPES];
  int filefirstwrite[MAXTAPES];
  long filepos[MAXTAPES];
  long nxtblkpos[MAXTAPES];
  long prvblkpos[MAXTAPES];
  int filecnt[MAXTAPES];
  int curfilen[MAXTAPES];
} card_am600;

typedef struct _card_ram { // generic RAM
  int port;
  int pinit;
  int plast;
  uint16_t size;
  uint16_t base[8];
  uint8_t *mem;
} card_ram;

typedef struct _card_rom { // generic ROM (bytesaver?)
  int port;
  int pinit;
  int plast;
  uint16_t size;
  uint16_t base[8];
  uint8_t *mem;
} card_rom;

typedef struct _card_ps3 { // Proc Tech 3P+S
  int port;
  unsigned char inbuf[128];
  int inptr, inptr2, incnt;
  unsigned char outbuf[128];
  int outptr, outptr2, outcnt;
  PANEL *thePanel;
  PANEL_DATA panel_data;
} card_ps3;

typedef struct _card_clock { // not really a card, but treated as one...
  int clkfrq;
} card_clock;

#define C_Type_am100 0x8000
#define C_Type_am200 0x4000
#define C_Type_am300 0x2000
#define C_Type_am320 0x1000
#define C_Type_am600 0x0800
#define C_Type_PS3 0x0400
#define C_Type_ram 0x0200
#define C_Type_rom 0x0100
#define C_Type_clock 0x0080

typedef struct _CARD {
  struct _CARD *CARDS_NEXT;
  uint16_t C_Type;

  union {
    card_am300 AM300;
    card_am320 AM320;
    card_am600 AM600;
    card_ps3 PS3;
    card_ram RAM;
    card_rom ROM;
    card_clock clock;
  } CType;

} CARDS;

/*-------------------------------------------------------------------*/
/* pointer to port handler routines                                  */
/*-------------------------------------------------------------------*/
typedef void (*PortPtr)();

/*-------------------------------------------------------------------*/
/* AWS (tape) header                                                 */
/*-------------------------------------------------------------------*/
typedef struct _AWSTAPE_BLKHDR {
  HWORD curblkl; /* Length of this block      */
  HWORD prvblkl; /* Length of previous block  */
  uint8_t flags1;   /* Flags byte 1              */
  uint8_t flags2;   /* Flags byte 2              */
} AWSTAPE_BLKHDR;

/* Definitions for AWSTAPE_BLKHDR flags byte 1 */
#define AWSTAPE_FLAG1_NEWREC 0x80   /* Start of new record       */
#define AWSTAPE_FLAG1_TAPEMARK 0x40 /* Tape mark                 */
#define AWSTAPE_FLAG1_ENDREC 0x20   /* End of record             */

#undef _TRACE_INST_

#define STRINGMAC(x) #x
#define MSTRING(x) STRINGMAC(x)

/*-------------------------------------------------------------------*/
/* AM100 State                                                       */
/*-------------------------------------------------------------------*/
typedef struct _am100_state_t
{
  
  unsigned char *am_membase[256]; /* pointers to 256 byte chuck memory */
  unsigned char am_memowns[256];  /* port associated with owning card  */
  unsigned char am_memflags[256]; /* flags for memory chunks */
  #define am$mem$flag_writable 1

  PortPtr am_ports[256];           /* pointers to port handlers */
  unsigned char *am_portbase[256]; /* pointers to per port save areas */

  PANEL_DATA *panels; /* base of panel data chain */
  CARDS *cards;       /* base of card chain */
  long long line_clock_ticks; /* incremented every 1/clkfrq seconds */

  int gPOST;   /* flag for extended startup diagnostics */
  int gTRACE;  /* flag for trace at startup */
  int gDialog; /* flag to Dialog active */

  char cpu4_svcctxt[16]; /* my SVCCs starts LO and go up, or */
                         /*          starts HI and go down.. */

  long gCHRSLP; /* num usecs to sleep waiting for next  */
                /* char in multi-char sequence    */

  pthread_attr_t attR_t;                                /* attribute */
  pthread_mutex_t bfrlock_t;                            /* buffer ctl lock */
  pthread_mutex_t clocklock_t;                          /* clock tasks lock */
  pthread_t background_t, lineclock_t, telnet_t;        /* threads */

  wd16_cpu_state_t* wd16_cpu_state;                     /* wd16 cpu state */

} am100_state_t;

extern am100_state_t am100_state;

#ifdef __cplusplus
}
#endif

#endif
