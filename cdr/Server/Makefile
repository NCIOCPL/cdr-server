#--------------------------------------------------------------------------
#
# $Id: Makefile,v 1.2 2000-04-11 17:51:30 bkline Exp $
#
# Rules for building CDR library.
#
# $Log: not supported by cvs2svn $
# Revision 1.1  2000/04/11 14:15:32  bkline
# Initial revision
#
#
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Makefile variables
#--------------------------------------------------------------------------
XML4C =\usr\xml4c
CDRINC=\cdr\include
INC   =-I$(XML4C)\include -I$(CDRINC)
CFLAGS=$(INC) -GX -MD -GR
CC    =cl
CDRLIB=\cdr\lib\cdr.lib
HDRS  =$(CDRINC)\CdrString.h \
       $(CDRINC)\CdrXsd.h \
       $(CDRINC)\CdrException.h \
       $(CDRINC)\CdrDom.h

#--------------------------------------------------------------------------
# Rule for building CDR library.
#--------------------------------------------------------------------------
$(CDRLIB): CdrString.obj CdrXsd.obj CdrDom.obj
	lib /OUT:$(CDRLIB) CdrString.obj CdrXsd.obj CdrDom.obj

CdrString.obj: CdrString.cpp $(CDRINC)\CdrString.h
	$(CC) $(CFLAGS) -c CdrString.cpp

CdrXsd.obj: CdrXsd.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrXsd.cpp

CdrDom.obj: CdrDom.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrDom.cpp

tCdrString.exe: tCdrString.cpp $(CDRLIB)
	$(CC) $(CFLAGS) tCdrString.cpp $(CDRLIB)

clean:
	rm -f *.obj *~
