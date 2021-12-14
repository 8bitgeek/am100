#/* makefile           (c) Copyright Mike Noel, 2001-2006             */
#/* ----------------------------------------------------------------- */
#/*                                                                   */
#/* This software is an emulator for the Alpha-Micro AM-100 computer. */
#/* It is copyright by Michael Noel and licensed for non-commercial   */
#/* hobbyist use under terms of the "Q public license", an open       */
#/* source certified license.  A copy of that license may be found    */
#/* here:       http://www.otterway.com/am100/license.html            */
#/*                                                                   */
#/* There exist known serious discrepancies between this software's   */
#/* internal functioning and that of a real AM-100, as well as        */
#/* between it and the WD-1600 manual describing the functionality of */
#/* a real AM-100, and even between it and the comments in the code   */
#/* describing what it is intended to do! Use it at your own risk!    */
#/*                                                                   */
#/* Reliability aside, it isn't the intent of the copyright holder to */
#/* use this software to compete with current or future Alpha-Micro   */
#/* products, and no such competing application of the software will  */
#/* be supported.                                                     */
#/*                                                                   */
#/* Alpha-Micro and other software that may be run on this emulator   */
#/* are not covered by the above copyright or license and must be     */
#/* legally obtained from an authorized source.                       */
#/*                                                                   */
#/* ----------------------------------------------------------------- */

TARGET	= am100
CPUDIR  = ../../libemuwd16
CPUSRC  = $(CPUDIR)/src
CPULIB  = $(CPUDIR)/libwd16.a

VERSION  = 1.1

C        = gcc

# CFLAGS   = -O1 -Wall -DVERSION=$(VERSION) -ggdb


INC = -I ./ \
	  -I $(CPUSRC) 

CFLAGS   = -Os -DVERSION=$(VERSION) -Wno-unused-result 			\
									-Wno-pointer-to-int-cast 	\
									-Wno-format 				\
									-Wno-discarded-qualifiers 	\
									-Wno-int-to-pointer-cast

#	   -march=pentium -malign-double -mwide-multiply

LFLAGS	 = -lm -lpthread -lncurses -lpanel -lmenu

EXEFILES = $(TARGET) tapfil filtap tapdir

TARFILES = makefile *.c *.h *.ini

OBJS     = main.o config.o \
	   priority.o hwassist.o trace.o\
	   clock.o io.o am320.o am600.o\
	   am300.o ps3.o terms.o telnet.o\
	   front-panel.o dialogs.o \
	   memory.o rom.o ram.o

HEADERS  = $(TARGET).h am-ddb.h

all:	   $(EXEFILES)

$(TARGET):  $(OBJS) $(CPULIB)
	$(CC) -o $(TARGET) $(OBJS) $(CPULIB) $(LFLAGS)

$(OBJS): %.o: %.c $(HEADERS) makefile
	$(CC) $(CFLAGS) -o $@ -c $<

$(CPULIB):
	(cd $(CPUDIR) && make)

tapfil:  tapfil.c tapfil.h makefile
	$(CC) -o tapfil tapfil.c -ggdb
tapdir:  tapdir.c tapfil.h makefile
	$(CC) -o tapdir tapdir.c -ggdb
filtap:  filtap.c tapfil.h makefile
	$(CC) -o filtap filtap.c -ggdb


clean:
	rm -f $(EXEFILES) $(EXEFILES).exe *.o
	(cd $(CPUDIR) && make clean)

tar:
	tar cvzf $(TARGET)-$(VERSION).tar.gz --exclude \*.o $(TARFILES)
