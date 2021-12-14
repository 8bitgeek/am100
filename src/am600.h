/* am600.h       (c) Copyright Mike Sharkey, 2021                    */
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
#ifndef __AM100_AM600_H__
#define __AM100_AM600_H__

#include "am100.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
int read_awstape(CARDS *cptr, int which, uint8_t *buf);
int write_awstape(CARDS *cptr, int which, uint8_t *buf, uint16_t blklen);
int write_awsmark(CARDS *cptr, int which);
int fsb_awstape(CARDS *cptr, int which);
int bsb_awstape(CARDS *cptr, int which);
//  fsf_awstape (CARDS  *cptr, int which); // fsf, bsf not used
//  bsf_awstape (CARDS  *cptr, int which);
int rew_awstape(CARDS *cptr, int which);
int run_awstape(CARDS *cptr, int which); // aka "close_awstape"
int erg_awstape(CARDS *cptr, int which);


#ifdef __cplusplus
}
#endif

#endif
