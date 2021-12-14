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

TARGET	= 	am100
CPUDIR  = 	libemuwd16
CPUSRC  = 	$(CPUDIR)/src
CPULIB  = 	$(CPUDIR)/libwd16.a

VERSION = 	2.0

CC  = 		gcc

INC = 		-I./ 			\
	  		-I$(CPUSRC)

CFLAGS += 	$(INC) 
CFLAGS += 	-Os -DVERSION=$(VERSION) 
CFLAGS += 	-Wno-unused-result 			\
			-Wno-pointer-to-int-cast 	\
			-Wno-format 				\
			-Wno-discarded-qualifiers 	\
			-Wno-int-to-pointer-cast

LFLAGS	 = -lm -lpthread -lncurses -lpanel -lmenu

OBJS     = 	src/main.o 		\
			src/config.o 		\
	   		src/priority.o 		\
	   		src/hwassist.o 		\
	   		src/trace.o 		\
	   		src/clock.o 		\
	   		src/io.o 			\
	   		src/am320.o 		\
	   		src/am600.o 		\
	   		src/am300.o 		\
	   		src/ps3.o 			\
	   		src/terms.o 		\
	   		src/telnet.o 		\
	   		src/front-panel.o 	\
	   		src/dialogs.o 		\
	   		src/memory.o 		\
	   		src/rom.o 			\
	   		src/ram.o

HEADERS  =  src/am100.h \
			src/am-ddb.h

$(TARGET):  $(OBJS) $(CPULIB)
	$(CC) -o $(TARGET) $(OBJS) $(CPULIB) $(LFLAGS)

$(OBJS): %.o: %.c $(HEADERS) makefile
	$(CC) $(CFLAGS) -o $@ -c $<

$(CPULIB):
	(cd $(CPUDIR) && make)

clean:
	rm -f $(EXEFILES) $(EXEFILES).exe src/*.o
	(cd $(CPUDIR) && make clean)
