#--------------------------------------------------------------------------
#
# $Id: Makefile,v 1.62 2002-09-09 12:42:55 bkline Exp $
#
# Rules for building CDR server.
#
# $Log: not supported by cvs2svn $
# Revision 1.61  2002/09/07 17:05:34  bkline
# Changed VCVER from 6 to 7.
#
# Revision 1.60  2002/07/24 15:09:36  bkline
# Added performance enhancements to link and query_term table updates.
#
# Revision 1.59  2002/07/22 14:26:06  bkline
# Added commented-out line for debugging Sablotron.
#
# Revision 1.58  2002/06/12 21:27:14  bkline
# Changed wwwtest to wwwroot.
#
# Revision 1.57  2002/06/12 20:24:35  bkline
# Added option for Visual Studio 7.
#
# Revision 1.56  2002/04/04 19:04:47  bkline
# Turned debugging symbols back on for standard build.
#
# Revision 1.55  2002/04/04 01:06:56  bkline
# Added CdrPublish module.
#
# Revision 1.54  2002/03/09 04:20:55  bkline
# Added -D_NDEBUG flag.
#
# Revision 1.53  2002/03/07 18:15:54  bkline
# Update for new version of regex package.
#
# Revision 1.52  2002/02/12 23:03:27  ameyer
# Eliminated reference to custom bison.cdrtest parser.  Moved new parser to
# /etc/bison.simple instead.
#
# Revision 1.51  2002/01/31 21:44:11  mruben
# fixed make of ccdoc to use $(DRV)
#
# Revision 1.50  2002/01/31 21:23:26  mruben
# Changed BISON_SIMPLE variable to avoid clash with environment variable
#
# Revision 1.49  2002/01/31 16:42:22  bkline
# Added DRV=$(DRV) to recursive make invocation.
#
# Revision 1.48  2002/01/31 13:19:31  bkline
# Added definition for DRV macro.
#
# Revision 1.47  2002/01/29 23:55:49  bkline
# Made bison rules more portable.
#
# Revision 1.46  2002/01/28 23:11:53  bkline
# Backed out changes to CP macro; added code for cvs update at
# appropriate spots.
#
# Revision 1.45  2002/01/25 15:10:01  bkline
# Working on getting cvs update invoked the correct number of times.
#
# Revision 1.44  2002/01/25 01:45:22  bkline
# Fixed rule for CdrTestClient to avoid problems with cl command-line
# option syntax, which gets confused by Unix path separators, which GNU
# make prefers.
#
# Revision 1.43  2002/01/25 00:35:29  ameyer
# Added cvs update to the init target.
# Modified CP macro to use MS copy silently.
# Added comments.
#
# Revision 1.42  2002/01/24 23:29:03  ameyer
# Added some extra control for what drive to use to enable the makefile to
# work on other machines while still building for the main CDR development
# machine.
# See cdrsetmak.bat, but beware of pulling in things from a local machine
# that should not be installed in cdr.lib or cdrserver.exe!
#
# Revision 1.41  2002/01/08 18:27:57  bkline
# Rebuilt CDR Service with DEBUG turned on.
#
# Revision 1.40  2002/01/02 20:01:06  bkline
# Pointed CDRINC to currect directory and added HeadDebug.h to header list.
#
# Revision 1.39  2001/12/14 15:14:54  bkline
# Used macro XML4C in LFLAGS.
#
# Revision 1.38  2001/11/28 20:36:16  bkline
# Added some targets to the `clean' rule.
#
# Revision 1.37  2001/11/08 23:00:20  bkline
# Added CdrLock.h
#
# Revision 1.36  2001/10/26 20:13:46  bkline
# Added /LIBPATH:$(REGEX)/lib to LFLAGS.
#
# Revision 1.35  2001/09/25 14:22:02  ameyer
# Added CdrFilter.h, CdrMergeProt.obj (by someone else).
#
# Revision 1.34  2001/04/13 12:29:26  bkline
# Added CdrActions.obj.
#
# Revision 1.33  2001/04/08 22:44:01  bkline
# Added CdrGetTree.obj.
#
# Revision 1.32  2001/02/28 03:19:15  ameyer
# Added CdrLinkProcs.h,.cpp.
#
# Revision 1.31  2001/02/28 02:34:54  bkline
# Added rule for building test client.
#
# Revision 1.30  2001/01/17 21:46:52  bkline
# Removed CdrGetSchema.obj.
#
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

DRV    =d:
CDR    =$(DRV)/cdr
XML4C  =$(DRV)/usr/xml4c
SABLOT =$(DRV)/usr/sablot
REGEX  =$(DRV)/usr/regex
VCVER  =7
REVER  =vc$(VCVER)
CDRINC =.
INC    =-I$(XML4C)/include -I$(SABLOT)/include -I$(CDRINC) -I$(REGEX)
CFLAGS =$(INC) -W3 -GX -MDd -GR -nologo -Zi #-D_NDEBUG
LFLAGS =/LIBPATH:$(XML4C)/lib /LIBPATH:$(SABLOT)/lib \
        /LIBPATH:$(CDR)/lib /LIBPATH:$(REGEX)/libs/regex/build/$(REVER) /DEBUG
CC     =cl
BISON  =bison
LN     =link
CP     =cp -f
CVSUPD =cvs update
DOC    =$(DRV)/InetPub/wwwroot/cdr/
DOCS   =$(DOC)default.htm
CDRLIB =$(CDR)/lib/cdr.lib
CDRBIN =$(CDR)/bin
CDRSVR =$(CDRBIN)/CdrServer.exe
SERVICE=$(CDRBIN)/CdrService.exe
SVC_CFLAGS =-DCRTAPI1=_cdecl -DCRTAPI2=_cdecl -D_X86_=1 -D_WINNT -DDEBUG \
            -D_WIN32_WINNT=0x0400 -DWINVER=0x0400 -DWIN32  -D_WIN32 -D_MT -MT
HDRS   =$(CDRINC)/CdrBlob.h \
        $(CDRINC)/CdrCommand.h \
        $(CDRINC)/CdrDbConnection.h \
        $(CDRINC)/CdrDbPreparedStatement.h \
        $(CDRINC)/CdrDbResultSet.h \
        $(CDRINC)/CdrDbStatement.h \
        $(CDRINC)/CdrDoc.h \
        $(CDRINC)/CdrDom.h \
        $(CDRINC)/CdrException.h \
        $(CDRINC)/CdrFilter.h \
        $(CDRINC)/CdrGetDoc.h \
        $(CDRINC)/CdrInt.h \
        $(CDRINC)/CdrLink.h \
        $(CDRINC)/CdrLinkProcs.h \
        $(CDRINC)/CdrLock.h \
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
        $(CDRINC)/CdrTiming.h \
        $(CDRINC)/HeapDebug.h \
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
        CdrActions.obj \
        CdrDoc.obj \
        CdrValidateDoc.obj \
        CdrLink.obj \
        CdrLinkProcs.obj \
        CdrSearch.obj \
        CdrSearch_tab.obj \
        CdrGetDoc.obj \
        CdrFilter.obj \
        CdrVersion.obj \
        CdrReport.obj \
        CdrDocTypes.obj \
        CdrShutdown.obj \
        CdrGetTree.obj \
        CdrMergeProt.obj \
        CdrDOMtoString.obj \
        CdrPublish.obj

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
all: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) _all

cvsupdate:
	$(CVSUPD)

service: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) $(SERVICE)

install: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) _install

_all: init CdrServer.exe CdrTestClient.exe #CdrDocs $(SERVICE)

_install: CdrServer.exe CdrDocs
	-net stop Cdr
	$(CP) CdrServer.exe $(CDRBIN)
	net start Cdr

#--------------------------------------------------------------------------
# Initialization, saving two generations of old errors
#--------------------------------------------------------------------------
init:
	-$(CP) errs.1 errs.2
	-$(CP) errs errs.1
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
# Rule for building test client.
#--------------------------------------------------------------------------

CdrTestClient.exe: CdrTestClient.cpp $(CDRLIB)
	$(CC) $(CFLAGS) CdrTestClient.cpp /link /LIBPATH:$(CDR)/lib \
    cdr.lib wsock32.lib

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
# Note that our version of bison requires a modified 
#   bison.simple parser to handle C++ syntax
# See /etc/bison.simple, /etc/bison.simple.readme
#--------------------------------------------------------------------------
CdrSearch_tab.obj: CdrSearch_tab.c $(HDRS)
	$(CC) $(CFLAGS) -c -TP CdrSearch_tab.c >> errs

CdrSearch_tab.c: CdrSearch.y $(HDRS)
	$(BISON) -p CdrSearch CdrSearch.y -o CdrSearch_tab.c

#--------------------------------------------------------------------------
# Rule for building CDR programmer documentation.
# Uses hardwired "cp" command to force overwrite of readonly files
# Only use this on the server.  See definition of CdrDocs above.
#--------------------------------------------------------------------------
CdrDocs: $(DOCS)

$(DOCS): $(HDRS)
	ccdoc -html $(DOC) -imgurl images/ $(HDRS)
	$(CP) $(DOC)ccdoc.index.html $(DOCS)

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
	rm -f *.obj *~ *.bak CdrSearch_tab.c CdrSearch.tab.c ccdoc.ctf
