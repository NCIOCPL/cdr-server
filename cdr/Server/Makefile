#--------------------------------------------------------------------------
#
# $Id: Makefile,v 1.6 2000-04-17 21:26:23 bkline Exp $
#
# Rules for building CDR server.
#
# $Log: not supported by cvs2svn $
# Revision 1.5  2000/04/16 21:39:04  bkline
# Extensive cleanup.  Added CdrLogoff command.
#
# Revision 1.4  2000/04/15 14:13:39  bkline
# Added support for DB classes.  Added implementation of CdrLogon command.
#
# Revision 1.3  2000/04/13 17:09:37  bkline
# Added rule for building server.
#
# Revision 1.2  2000/04/11 17:51:30  bkline
# Added rule for building test driver for CdrString class.  Enhanced 'make
# clean' rule.
#
# Revision 1.1  2000/04/11 14:15:32  bkline
# Initial revision
#
#
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Makefile variables
#--------------------------------------------------------------------------

CDR    =/cdr
XML4C  =/usr/xml4c
CDRINC =$(CDR)/include
INC    =-I$(XML4C)/include -I$(CDRINC)
CFLAGS =$(INC) -GX -MD -GR -Zi
LFLAGS =/LIBPATH:/usr/xml4c/lib /LIBPATH:$(CDR)/lib /DEBUG
CC     =cl
LN     =link
CDRLIB =$(CDR)/lib/cdr.lib
CDRBIN =$(CDR)/bin
CDRSVR =$(CDRBIN)/CdrServer.exe
HDRS   =$(CDRINC)/CdrString.h \
        $(CDRINC)/CdrXsd.h \
        $(CDRINC)/CdrException.h \
        $(CDRINC)/CdrDom.h \
        $(CDRINC)/CdrCommand.h \
        $(CDRINC)/CdrDbConnection.h \
        $(CDRINC)/CdrDbStatement.h \
        $(CDRINC)/CdrDbResultSet.h \
        $(CDRINC)/CdrCommand.h \
        $(CDRINC)/CdrSession.h
LIBOBJS=CdrString.obj \
        CdrXsd.obj \
        CdrDom.obj \
        CdrDbConnection.obj \
        CdrDbStatement.obj \
        CdrDbResultSet.obj \
        CdrSession.obj
OBJS   =CdrServer.obj \
        CdrCommand.obj \
        CdrLogon.obj \
        CdrLogoff.obj \
        CdrCheckAuth.obj
LIBS   =cdr.lib xerces-c_1.lib wsock32.lib odbc32.lib

#--------------------------------------------------------------------------
# Rule for building CDR server.
#--------------------------------------------------------------------------

$(CDRSVR): $(OBJS) $(CDRLIB)
	$(LN) $(LFLAGS) /out:$(CDRSVR) $(OBJS) $(LIBS)

#$(CC) $(CFLAGS) -Fe$(CDRSVR) $(OBJS) $(LIBS)

CdrServer.obj: CdrServer.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrServer.cpp

CdrCommand.obj: CdrCommand.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrCommand.cpp

CdrLogon.obj: CdrLogon.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrLogon.cpp

CdrLogoff.obj: CdrLogoff.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrLogoff.cpp

CdrCheckAuth.obj: CdrCheckAuth.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrCheckAuth.cpp

#--------------------------------------------------------------------------
# Rule for building CDR library.
#--------------------------------------------------------------------------
$(CDRLIB): $(LIBOBJS)
	lib /OUT:$(CDRLIB) $(LIBOBJS)

CdrString.obj: CdrString.cpp $(CDRINC)/CdrString.h
	$(CC) $(CFLAGS) -c CdrString.cpp

CdrXsd.obj: CdrXsd.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrXsd.cpp

CdrDom.obj: CdrDom.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrDom.cpp

CdrDbConnection.obj: CdrDbConnection.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrDbConnection.cpp

CdrDbStatement.obj: CdrDbStatement.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrDbStatement.cpp

CdrDbResultSet.obj: CdrDbResultSet.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrDbResultSet.cpp

CdrSession.obj: CdrSession.cpp $(HDRS)
	$(CC) $(CFLAGS) -c CdrSession.cpp

tCdrString.exe: tCdrString.cpp $(CDRLIB)
	$(CC) $(CFLAGS) tCdrString.cpp $(CDRLIB)

clean:
	rm -f *.obj *~
