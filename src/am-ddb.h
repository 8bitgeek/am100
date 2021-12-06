/* am-ddb.h           (c) Copyright Mike Noel, 2001-2008             */
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

//      DDB offsets ( '<---' means used by vdkdrv )

#define DB$ERR 0  // error code
#define DB$FLG 1  // flags                <---
#define DB$BAD 2  // buffer address       <---
#define DB$RSZ 4  // record size          <---
#define DB$BFI 6  // buffer index
#define DB$RNM 8  // record number        <---
#define DB$QCL 10 // queue chain link
#define DB$JCB 12 // JCB address
#define DB$PRI 14 // job priority
#define DB$DVC 16 // device code
#define DB$DRI 18 // drive
#define DB$CLL 19 // call level
#define DB$FIL 20 // filename
#define DB$EXT 24 // extension
#define DB$PPN 26 // PPN
#define DB$OCD 28 // open code
#define DB$WRK 30 // driver work area (5 words)
#define DB$NRC 30 // number of records in file
#define DB$ADB 32 // active bytes in last record
#define DB$FRN 34 // number of first record
#define SIZ$DB 40 // size of DDB
