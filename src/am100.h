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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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

#include <assert.h>

/*-------------------------------------------------------------------*/
/* Endian determination                */
/* Endian determination                */
/* Endian determination                */
/*-------------------------------------------------------------------*/

#define AM_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define AM_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#define AM_BYTE_ORDER __BYTE_ORDER__

/*-------------------------------------------------------------------*/
/* TYPES  TYPES TYPES TYPES TYPES TYPES TYPES      */
/* TYPES  TYPES TYPES TYPES TYPES TYPES TYPES      */
/* TYPES  TYPES TYPES TYPES TYPES TYPES TYPES      */
/*-------------------------------------------------------------------*/

/* Platform-independent storage operand definitions */
typedef uint8_t BYTE;
typedef uint8_t HWORD[2];
typedef uint8_t FWORD[4];
typedef uint8_t DWORD[8];
typedef uint8_t U8;
typedef int8_t S8;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint32_t U32;
typedef int32_t S32;
typedef uint64_t U64;
typedef int64_t S64;

/*-------------------------------------------------------------------*/
/* Structure definition for PS context               */
/*-------------------------------------------------------------------*/
typedef struct _PS { /* Status Word        */

#if AM_BYTE_ORDER == AM_BIG_ENDIAN
  uint16_t pwrfail : 1, /* 1=power is failing        */
      buserror : 1,     /* 1=problem on the bus      */
      parityerr : 1,    /* 1=problem with memory     */
      I2 : 1,           /* 1=Interrupt enabled       */
      haltjmp2 : 1,     /* - hardware jumper set     */
      haltjmp1 : 1,     /* - hardware jumper set     */
      pwrjmp2 : 1,      /* - hardware jumper set     */
      pwrjmp1 : 1,      /* - hardware jumper set     */
      ALU : 4,          /* not relevent to pgmr?     */
      N : 1,            /* 1=MSB is set              */
      Z : 1,            /* 1=zero                    */
      V : 1,            /* 1=overflow                */
      C : 1;            /* 1=carry                   */
#else
  uint16_t C : 1,    /* 1=carry                   */
      V : 1,         /* 1=overflow                */
      Z : 1,         /* 1=zero                    */
      N : 1,         /* 1=MSB is set              */
      ALU : 4,       /* not relevent to pgmr?     */
      pwrjmp1 : 1,   /* - hardware jumper set     */
      pwrjmp2 : 1,   /* - hardware jumper set     */
      haltjmp1 : 1,  /* - hardware jumper set     */
      haltjmp2 : 1,  /* - hardware jumper set     */
      I2 : 1,        /* 1=Interrupt enabled       */
      parityerr : 1, /* 1=problem with memory     */
      buserror : 1,  /* 1=problem on the bus      */
      pwrfail : 1;   /* 1=power is failing        */
#endif

} TPS;

/*-------------------------------------------------------------------*/
/* Structure definition for CPU register context         */
/*-------------------------------------------------------------------*/
typedef struct _REGS { /* Processor registers       */
  U64 instcount;       /* Instruction counter       */
  U16 *gpr;            /* addressing of registers   */
  S16 *spr;            /* addressing of signed regs */
  U16 R0;              /* aka gpr[0]                */
  U16 R1;              /* aka gpr[1]                */
  U16 R2;              /* aka gpr[2]                */
  U16 R3;              /* aka gpr[3]                */
  U16 R4;              /* aka gpr[4]                */
  U16 R5;              /* aka gpr[5]                */
  U16 SP;              /* aka gpr[6]                */
  U16 PC;              /* aka gpr[7]                */
  TPS PS;              /* aka gpr[8]                */
  /*                           */
  int trace; /* XCT trace flag            */
  /*                           */
  int halting;     /* halting flag              */
  int BOOTing;     /* monitor BOOTing flag      */
  int stepping;    /* single stepping flag      */
  int tracing;     /* inst. tracing flag        */
  int utrace;      /* user space trace flag     */
  U16 utR0;        /* utrace JOBCUR initial     */
  U16 utRX;        /* utrace JOBCUR match       */
  U16 utPC;        /* utrace MEMBAS match       */
  int waiting;     /* waiting flag              */
  int intpending;  /* interrupt pending flag    */
  int whichint[9]; /* which int is pending?     */
  /* 0=nv, 1-8 vectored        */
  unsigned char LED; /* Diagnostic LED            */

} REGS;

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
  U16 size;
  U16 base[8];
  U8 *mem;
} card_ram;

typedef struct _card_rom { // generic ROM (bytesaver?)
  int port;
  int pinit;
  int plast;
  U16 size;
  U16 base[8];
  U8 *mem;
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
  U16 C_Type;

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
  BYTE flags1;   /* Flags byte 1              */
  BYTE flags2;   /* Flags byte 2              */
} AWSTAPE_BLKHDR;

/* Definitions for AWSTAPE_BLKHDR flags byte 1 */
#define AWSTAPE_FLAG1_NEWREC 0x80   /* Start of new record       */
#define AWSTAPE_FLAG1_TAPEMARK 0x40 /* Tape mark                 */
#define AWSTAPE_FLAG1_ENDREC 0x20   /* End of record             */

/*-------------------------------------------------------------------*/
/* GLOBALS  GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS      */
/* GLOBALS  GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS      */
/* GLOBALS  GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS GLOBALS      */
/*-------------------------------------------------------------------*/

char instruction_type[65536]; /* maps all words to inst format type */

unsigned char *am_membase[256]; /* pointers to 256 byte chuck memory */
unsigned char am_memowns[256];  /* port associated with owning card  */
unsigned char am_memflags[256]; /* flags for memory chunks */
#define am$mem$flag_writable 1

PortPtr am_ports[256];           /* pointers to port handlers */
unsigned char *am_portbase[256]; /* pointers to per port save areas */

PANEL_DATA *panels; /* base of panel data chain */
CARDS *cards;       /* base of card chain */
REGS regs;

U16 oldPCs[256];            /* table of prior PC's */
unsigned oldPCindex;        /* pointer to next entry in prior PC's table */
U16 op, opPC;               /* current opcode (base) and its location */
long long line_clock_ticks; /* incremented every 1/clkfrq seconds */

int gPOST;   /* flag for extended startup diagnostics */
int gTRACE;  /* flag for trace at startup */
int gDialog; /* flag to Dialog active */

char cpu4_svcctxt[16]; /* my SVCCs starts LO and go up, or */
                       /*          starts HI and go down.. */

long gCHRSLP; /* num usecs to sleep waiting for next  */
              /* char in multi-char sequence    */

pthread_attr_t attR_t;                                /* attribute */
pthread_mutex_t intlock_t;                            /* interrupt lock */
pthread_mutex_t bfrlock_t;                            /* buffer ctl lock */
pthread_mutex_t clocklock_t;                          /* clock tasks lock */
pthread_t background_t, lineclock_t, cpu_t, telnet_t; /* threads */

/*-------------------------------------------------------------------*/
/* Macros Macros  Macros  Macros  Macros  Macros  Macros       */
/* Macros Macros  Macros  Macros  Macros  Macros  Macros       */
/* Macros Macros  Macros  Macros  Macros  Macros  Macros       */
/*-------------------------------------------------------------------*/

#undef _TRACE_INST_

#define true 1
#define false 0

#define STRINGMAC(x) #x
#define MSTRING(x) STRINGMAC(x)

/*-------------------------------------------------------------------*/
/* Function prototypes  Function prototypes Function prototypes  */
/* Function prototypes  Function prototypes Function prototypes  */
/* Function prototypes  Function prototypes Function prototypes  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* in front-panel.c                                                  */
/*-------------------------------------------------------------------*/
void FP_init(void);
void FP_refresh(void);
void FP_led(void);
void FP_key(int c);
void FP_stop(void);

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

/*-------------------------------------------------------------------*/
/* in config.c                                                       */
/*-------------------------------------------------------------------*/
int config_start(char *config_file_name);
void config_stop(void);
void config_reset(void);
void config_fileload(unsigned char *filename, U16 where, U16 *fsize);
void config_filesave(unsigned char *filename, U16 where, U16 *fsize);
void config_memdump(U16 where, U16 fsize);
void config_stop(void);

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

/*-------------------------------------------------------------------*/
/* in clock.c                                                        */
/*-------------------------------------------------------------------*/
void clock_Init(unsigned int clkfrq);
void clock_thread(void);
void background_thread(void);
void clock_stop(void);
void clock_reset(void);

/*-------------------------------------------------------------------*/
/* in telnet.c                                                       */
/*-------------------------------------------------------------------*/
void telnet_Init1(unsigned int port);
void telnet_Init2(void);
void telnet_thread(void);
void telnet_stop(void);
void telnet_reset(void);
int telnet_poll(void);
int telnet_InChars(PANEL *panel);
void telnet_OutChars(PANEL *panel, char *chr);

/*-------------------------------------------------------------------*/
/* in terms.c                                                        */
/*-------------------------------------------------------------------*/
int get_panelnumber(void);
void hide_all_panels(void);
void show_panel_by_fnum(int fnum);
char *getns(char *str, int nn, WINDOW *win);
int getxh(int wait);
int InChars(void);
void OutChars(PANEL *panel, char *chr);

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

/*-------------------------------------------------------------------*/
/* in am320.c                                                        */
/*-------------------------------------------------------------------*/
void am320_Init(unsigned int port, unsigned int intlvl, char *filename);
void am320_stop(void);
void am320_reset(unsigned char *sa);
int am320_poll(unsigned char *sa);
void am320_getfile(unsigned int port, char *filename);
int am320_mount(unsigned int port, char *filename);
void am320_unmount(unsigned int port);
void am320_Port0(unsigned char *chr, int rwflag, unsigned char *sa);
void am320_Port1(unsigned char *chr, int rwflag, unsigned char *sa);

/*-------------------------------------------------------------------*/
/* in am600.c                                                        */
/*-------------------------------------------------------------------*/
void am600_Init(unsigned int port, unsigned int intlvl, char *filename,
                int filerw);
void am600_stop(void);
void am600_reset(unsigned char *sa);
void am600_getfiles(unsigned int port, char *fn[]);
int am600_mount(unsigned int port, int unit, char *filename, int filerw);
void am600_unmount(unsigned int port, int unit);
void am600_Port0(unsigned char *chr, int rwflag, unsigned char *sa);
void am600_Port1(unsigned char *chr, int rwflag, unsigned char *sa);
// id am600_Port2(unsigned char *chr, int rwflag, unsigned char *sa);
void am600_Port3(unsigned char *chr, int rwflag, unsigned char *sa);
void am600_Port4(unsigned char *chr, int rwflag, unsigned char *sa);
void am600_Port5(unsigned char *chr, int rwflag, unsigned char *sa);
// id am600_Port6(unsigned char *chr, int rwflag, unsigned char *sa);

int open_awstape(CARDS *cptr, int which);
int readhdr_awstape(CARDS *cptr, int which, long blkpos, AWSTAPE_BLKHDR *buf);
int read_awstape(CARDS *cptr, int which, BYTE *buf);
int write_awstape(CARDS *cptr, int which, BYTE *buf, U16 blklen);
int write_awsmark(CARDS *cptr, int which);
int fsb_awstape(CARDS *cptr, int which);
int bsb_awstape(CARDS *cptr, int which);
//  fsf_awstape (CARDS  *cptr, int which); // fsf, bsf not used
//  bsf_awstape (CARDS  *cptr, int which);
int rew_awstape(CARDS *cptr, int which);
int run_awstape(CARDS *cptr, int which); // aka "close_awstape"
int erg_awstape(CARDS *cptr, int which);

/*-------------------------------------------------------------------*/
/* in ps3.c                                                          */
/*-------------------------------------------------------------------*/
void PS3_Init(unsigned int port, unsigned int fnum);
void PS3_stop(void);
void PS3_reset(unsigned char *sa);
int PS3_poll(unsigned char *sa);
void PS3_Port0(unsigned char *chr, int rwflag, unsigned char *sa);
void PS3_Port1(unsigned char *chr, int rwflag, unsigned char *sa);

/*-------------------------------------------------------------------*/
/* in memory.c                                                       */
/*-------------------------------------------------------------------*/
void initAMmemory(void);
void memory_stop(void);
void memory_reset(void);
void swapInAMmemory(unsigned char *cardaddr, int cardport,
                    unsigned char cardflags, /* all same for numpage */
                    int numpage, U16 amaddr);
void swapOutAMmemory(unsigned char *cardaddr, /* not used */
                     int cardport,            /* verify swapout for this card */
                     unsigned char cardflags, /* not used */
                     int numpage, U16 amaddr);
void getAMbyte(unsigned char *chr, long address);
void getAMword(unsigned char *chr, long address);
void putAMbyte(unsigned char *chr, long address);
void putAMword(unsigned char *chr, long address);
void moveAMbyte(long to, long from);
void moveAMword(long to, long from);
U16 getAMaddrBYmode(int regnum, int mode, int offset);
U16 getAMwordBYmode(int regnum, int mode, int offset);
void putAMwordBYmode(int regnum, int mode, int offset, U16 theword);
void undAMwordBYmode(int regnum, int mode);
U8 getAMbyteBYmode(int regnum, int mode, int offset);
void putAMbyteBYmode(int regnum, int mode, int offset, U8 thebyte);
void undAMbyteBYmode(int regnum, int mode);

/*-------------------------------------------------------------------*/
/* in rom.c                                                          */
/*-------------------------------------------------------------------*/
void rom_Init(unsigned int port, unsigned int pinit, unsigned int phantom,
              unsigned int size, long base[8], unsigned char *image);
void rom_Port(unsigned char *chr, int rwflag, unsigned char *sa);

/*-------------------------------------------------------------------*/
/* in ram.c                                                          */
/*-------------------------------------------------------------------*/
void ram_Init(unsigned int port, unsigned int pinit, unsigned int phantom,
              unsigned int size, long base[8]);
void ram_Port(unsigned char *chr, int rwflag, unsigned char *sa);

/*-------------------------------------------------------------------*/
/* in cpu-fmt?.c                                                     */
/*-------------------------------------------------------------------*/
void do_fmt_1(void);
void do_fmt_2(void);
void do_fmt_3(void);
void do_fmt_4(void);
int svca_assist(int arg);
int svcb_assist(int arg);
int svcc_assist(int arg);
void do_fmt_5(void);
void do_fmt_6(void);
void do_fmt_7(void);
void do_fmt_8(void);
void do_fmt_9(void);
void do_fmt_10(void);
void do_fmt_11(void);

void build_decode(void);
void do_fmt_invalid(void);
void execute_instruction(void);
void perform_interrupt(void);
void cpu_thread(void);
void cpu_stop(void);

/*-------------------------------------------------------------------*/
/* in priority.c                                                     */
/*-------------------------------------------------------------------*/
void set_pri_low(void);
void set_pri_normal(void);
void set_pri_high(void);

/*-------------------------------------------------------------------*/
/* in hwassist.c                                                     */
/*-------------------------------------------------------------------*/
void vdkdvr_start(void);
void vdkdvr_stop(void);
void vdkdvr_reset(void);
void vdkdvr_getfiles(char *fn[]);
int vdkdvr_mount(int unit, char *filename);
void vdkdvr_unmount(int unit);
void vdkdvr(void);
int vdkdvr_boot(long drv, char *monName, char *iniName, char *dvrName);

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
void trace_fmt7(char *opc, int dmode, int dreg, U16 n1word);
void trace_fmt8(char *opc, int sreg, int dreg);
void trace_fmt9(char *opc, int sreg, int dmode, int dreg, U16 n1word);
void trace_fmt9_jsr(char *opc, int sreg, int dmode, int dreg, U16 n1word);
void trace_fmt9_lea(char *opc, int sreg, int dmode, int dreg, U16 n1word);
void trace_fmt9_sob(char *opc, int sreg, int dmode, int dreg);
void trace_fmt10(char *opc, int smode, int sreg, int dmode, int dreg,
                 U16 n1word);
void trace_fmt11(char *opc, int sind, int sreg, double s, int dind, int dreg,
                 double d);
