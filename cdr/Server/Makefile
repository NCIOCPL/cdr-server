#--------------------------------------------------------------------------
#
# $Id: Makefile,v 1.30 2001-01-17 21:46:52 bkline Exp $
#
# Rules for building CDR server.
#
# $Log: not supported by cvs2svn $
# Revision 1.29  2001/01/08 16:57:08  mruben
# removed libraries no longer needed by sablotron
#
# Revision 1.28  2000/12/28 13:29:54  bkline
# Added CdrGetSchema.obj.
#
# Revision 1.27  2000/10/24 13:13:16  mruben
# added support for CdrReport
#
# Revision 1.26  2000/10/04 18:37:44  bkline
# Added -nologo flag to CFLAGS; added link and catchexp modules.
#
# Revision 1.25  2000/08/10 20:26:56  bkline
# Added commands to stop/start Cdr service as part of install target rule.
#
# Revision 1.24  2000/08/10 19:47:14  bkline
# Added dependency for 'install' rule.
#
# Revision 1.23  2000/08/10 19:37:56  bkline
# Default rule now builds server in /cdr/src; new 'install' rule added.
#
# Revision 1.22  2000/07/10 19:51:55  bkline
# Added rules for service.
#
# Revision 1.21  2000/06/09 04:02:33  ameyer
# Added CdrLog.h, CdrLog.cpp
#
# Revision 1.20  2000/05/21 00:51:33  bkline
# Added CdrShutdown.
#
# Revision 1.19  2000/05/19 00:14:25  bkline
# Cosmetic edit.
#
# Revision 1.18  2000/05/17 13:54:46  ameyer
# Added CdrDoc
#
# Revision 1.17  2000/05/10 21:51:31  mruben
# fixed command line arguments for Sablotron libraries and includes
#
# Revision 1.16  2000/05/09 19:54:56  mruben
# added CdrDOMtoString
#
# Revision 1.15  2000/05/09 19:27:45  mruben
# modified for CdrFilter
#
# Revision 1.14  2000/05/04 16:49:10  bkline
# Added empty .SUFFIXES directive to clear out builtin suffix list.
#
# Revision 1.13  2000/05/04 12:46:32  bkline
# Sorted HDRS lines; eliminated duplicate line.
#
# Revision 1.12  2000/05/03 15:27:13  bkline
# Added CdrDbPreparedStatement module; fixed rm invocation; added warning
# option for compiler.
#
# Revision 1.11  2000/05/02 21:54:41  ameyer
# Changed to generic rules.
# Added error redirection to a file.
# Removed main program from OBJS.
# Made CdrDocs part of the all target.
#
# Revision 1.10  2000/05/02 19:26:38  bkline
# Added CdrRegEx, CdrBlob, CdrValidateDoc, and preliminary CdrGetDoc.
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
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Makefile variables
#--------------------------------------------------------------------------

CDR    =/cdr
XML4C  =/usr/xml4c
SABLOT =/usr/sablot
REGEX  =/usr/regex
CDRINC =$(CDR)/include
INC    =-I$(XML4C)/include -I$(SABLOT)/include -I$(CDRINC) -I$(REGEX)/include
CFLAGS =$(INC) -W3 -GX -MT -GR -Zi -nologo
LFLAGS =/LIBPATH:/usr/xml4c/lib /LIBPATH:$(SABLOT)/lib \
	/LIBPATH:$(CDR)/lib /DEBUG
CC     =cl
LN     =link
DOC    =../../InetPub/wwwtest/cdr/
DOCS   =$(DOC)default.htm
CDRLIB =$(CDR)/lib/cdr.lib
CDRBIN =$(CDR)/bin
CDRSVR =$(CDRBIN)/CdrServer.exe
SERVICE=$(CDRBIN)/CdrService.exe
SVC_CFLAGS =-DCRTAPI1=_cdecl -DCRTAPI2=_cdecl -D_X86_=1 -D_WINNT \
            -D_WIN32_WINNT=0x0400 -DWINVER=0x0400 -DWIN32  -D_WIN32 -D_MT -MT
HDRS   =$(CDRINC)/CdrBlob.h \
        $(CDRINC)/CdrCommand.h \
        $(CDRINC)/CdrDbConnection.h \
        $(CDRINC)/CdrDoc.h \
        $(CDRINC)/CdrDbPreparedStatement.h \
        $(CDRINC)/CdrDbResultSet.h \
        $(CDRINC)/CdrDbStatement.h \
        $(CDRINC)/CdrDom.h \
        $(CDRINC)/CdrException.h \
        $(CDRINC)/CdrGetDoc.h \
        $(CDRINC)/CdrInt.h \
        $(CDRINC)/CdrLink.h \
        $(CDRINC)/CdrLog.h \
        $(CDRINC)/CdrParserInput.h \
        $(CDRINC)/CdrRegEx.h \
        $(CDRINC)/CdrReport.h \
        $(CDRINC)/CdrSearch.h \
        $(CDRINC)/CdrSession.h \
        $(CDRINC)/CdrString.h \
        $(CDRINC)/CdrValidateDoc.h \
        $(CDRINC)/CdrVersion.h \
        $(CDRINC)/CdrXsd.h \
        $(CDRINC)/catchexp.h
LIBOBJS=CdrString.obj \
        CdrXsd.obj \
        CdrDom.obj \
        CdrDbConnection.obj \
        CdrDbStatement.obj \
        CdrDbPreparedStatement.obj \
        CdrDbResultSet.obj \
        CdrLog.obj \
        CdrSession.obj \
        CdrParserInput.obj \
        CdrRegEx.obj \
        catchexp.obj
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
        CdrDoc.obj \
        CdrValidateDoc.obj \
        CdrLink.obj \
        CdrSearch.obj \
        CdrSearch_tab.obj \
        CdrGetDoc.obj \
        CdrFilter.obj \
        CdrVersion.obj \
        CdrReport.obj \
        CdrDocTypes.obj \
        CdrShutdown.obj \
        CdrDOMtoString.obj

LIBS   =cdr.lib xerces-c_1.lib sablot.lib \
        wsock32.lib odbc32.lib user32.lib 

#--------------------------------------------------------------------------
# File suffixes examined for rule interpretation
#--------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .obj .c .cpp

#--------------------------------------------------------------------------
# Targets
#--------------------------------------------------------------------------
all: init CdrServer.exe CdrDocs #$(SERVICE)

service: $(SERVICE)

install: CdrServer.exe
	-net stop Cdr
	cp CdrServer.exe ../bin/CdrServer.exe
	net start Cdr

#--------------------------------------------------------------------------
# Initialization, saving two generations of old errors
#--------------------------------------------------------------------------
init:
	-cp errs.1 errs.2
	-cp errs errs.1
	-rm -f errs

#--------------------------------------------------------------------------
# Rule for building CDR server.
#--------------------------------------------------------------------------

CdrServer.exe: CdrServer.obj $(OBJS) $(CDRLIB)
	$(LN) $(LFLAGS) /out:CdrServer.exe CdrServer.obj $(OBJS) $(LIBS) >> errs

#--------------------------------------------------------------------------
# Rule for building CDR service.
#--------------------------------------------------------------------------

$(SERVICE): CdrService.obj service.obj
	$(CC) -MT -Fe$(SERVICE) CdrService.obj service.obj advapi32.lib

CdrService.obj: CdrService.c service.h
	$(CC) -c $(SVC_CFLAGS) CdrService.c

service.obj: service.c service.h
	$(CC) -c $(SVC_CFLAGS) service.c

#--------------------------------------------------------------------------
# We're not doing dependency checking.
# Any change to any header file recompiles all the source.
#--------------------------------------------------------------------------
$(OBJS): $(HDRS)

$(LIBOBJS): $(HDRS)

CdrServer.obj: CdrServer.cpp $(HDRS)

#--------------------------------------------------------------------------
# Rule for building CDR library.
#--------------------------------------------------------------------------
$(CDRLIB): $(LIBOBJS)
	lib /OUT:$(CDRLIB) $(LIBOBJS) >> errs

#--------------------------------------------------------------------------
# For Bison, we're not attempting a generic rule
#--------------------------------------------------------------------------
BISON_SIMPLE=/etc/bison.cdrtest
CdrSearch_tab.obj: CdrSearch_tab.c $(HDRS)
	$(CC) $(CFLAGS) -c -TP CdrSearch_tab.c >> errs

CdrSearch_tab.c: CdrSearch.y $(HDRS)
	bison -p CdrSearch CdrSearch.y

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
