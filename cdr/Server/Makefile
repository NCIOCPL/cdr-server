#--------------------------------------------------------------------------
#
# $Id: Makefile,v 1.11 2000-05-02 21:54:41 ameyer Exp $
#
# Rules for building CDR server.
#
# $Log: not supported by cvs2svn $
# Revision 1.10  2000/05/02 19:26:38  bkline
# Added CdrRegEx, CdrBlob, CdrValidateDoc, and prelimintary CdrGetDoc.
#
# Revision 1.9  2000/04/26 01:27:03  bkline
# Added rules for building programmer documentation.  Added CdrSearch
# and CdrValidateDoc modules.
#
# Revision 1.8  2000/04/22 09:26:36  bkline
# Added rules for user and group commands.
#
# Revision 1.7  2000/04/21 13:52:24  bkline
# Added CdrSearch module.
#
# Revision 1.6  2000/04/17 21:26:23  bkline
# Added CdrCheckAuth command.
#
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
REGEX  =/usr/regex
CDRINC =$(CDR)/include
INC    =-I$(XML4C)/include -I$(CDRINC) -I$(REGEX)/include
CFLAGS =$(INC) -GX -MT -GR -Zi
LFLAGS =/LIBPATH:/usr/xml4c/lib /LIBPATH:$(CDR)/lib /DEBUG
CC     =cl
LN     =link
DOC    =../../InetPub/wwwtest/cdr/
DOCS   =$(DOC)default.htm
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
        $(CDRINC)/CdrSession.h \
        $(CDRINC)/CdrParserInput.h \
        $(CDRINC)/CdrInt.h \
        $(CDRINC)/CdrSearch.h \
        $(CDRINC)/CdrRegEx.h \
        $(CDRINC)/CdrBlob.h \
        $(CDRINC)/CdrGetDoc.h \
        $(CDRINC)/CdrValidateDoc.h
LIBOBJS=CdrString.obj \
        CdrXsd.obj \
        CdrDom.obj \
        CdrDbConnection.obj \
        CdrDbStatement.obj \
        CdrDbResultSet.obj \
        CdrSession.obj \
        CdrParserInput.obj \
        CdrRegEx.obj
OBJS   =CdrCommand.obj \
        CdrLogon.obj \
        CdrLogoff.obj \
        CdrCheckAuth.obj \
        CdrAddGrp.obj \
        CdrGetGrp.obj \
        CdrModGrp.obj \
        CdrDelGrp.obj \
        CdrListGrps.obj \
        CdrAddUsr.obj \
        CdrGetUsr.obj \
        CdrModUsr.obj \
        CdrDelUsr.obj \
        CdrListUsrs.obj \
        CdrValidateDoc.obj \
        CdrSearch.obj \
        CdrSearch_tab.obj \
        CdrGetDoc.obj
LIBS   =cdr.lib xerces-c_1.lib wsock32.lib odbc32.lib user32.lib

# File suffixes examined for rule interpretation
.SUFFIXES: .obj .c .cpp

# Targets
all: init $(CDRSVR) CdrDocs

#--------------------------------------------------------------------------
# Initialization, saving two generations of old errors
#--------------------------------------------------------------------------
init:
	-cp errs.1 errs.2
	-cp errs errs.1
	-rm errs

#--------------------------------------------------------------------------
# Rule for building CDR server.
#--------------------------------------------------------------------------

$(CDRSVR): CdrServer.obj $(OBJS) $(CDRLIB)
	$(LN) $(LFLAGS) /out:$(CDRSVR) CdrServer.obj $(OBJS) $(LIBS) >> errs

#--------------------------------------------------------------------------
# We're not doing dependency checking.
# Any change to any header file recompiles all the source.
#--------------------------------------------------------------------------
$(OBJS): $(HDRS)

$(LIBOBJS): $(HDRS)

CdrServer.obj: CdrServer.cpp $(HDRS)

#--------------------------------------------------------------------------
# For Bison, we're not attempting a generic rule
#--------------------------------------------------------------------------
BISON_SIMPLE=/etc/bison.cdrtest
CdrSearch_tab.obj: CdrSearch_tab.c $(HDRS)
	$(CC) $(CFLAGS) -c -TP CdrSearch_tab.c >> errs

CdrSearch_tab.c: CdrSearch.y $(HDRS) $(CDRINC)/CdrSearch.h
	bison -p CdrSearch CdrSearch.y

#--------------------------------------------------------------------------
# Rule for building CDR library.
#--------------------------------------------------------------------------
$(CDRLIB): $(LIBOBJS)
	lib /OUT:$(CDRLIB) $(LIBOBJS) >> errs


# This one has a different header dependency
CdrString.obj: CdrString.cpp $(CDRINC)/CdrString.h

#--------------------------------------------------------------------------
# Rule for building CDR programmer documentation.
#--------------------------------------------------------------------------
CdrDocs: $(DOCS)

$(DOCS): $(HDRS)
	ccdoc -html $(DOC) -imgurl images/ $(HDRS)
	cp -f $(DOC)ccdoc.index.html $(DOCS)

#--------------------------------------------------------------------------
# Generic rules for object files
#--------------------------------------------------------------------------
.c.obj:
	$(CC) -c $(CFLAGS) $*.c >> errs

.cpp.obj:
	$(CC) -c $(CFLAGS) $*.cpp >> errs

#--------------------------------------------------------------------------
# Cleanup rules.
#--------------------------------------------------------------------------
clean:
	rm -f *.obj *~ CdrSearch_tab.c
