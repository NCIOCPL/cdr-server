#--------------------------------------------------------------------------
#
# $Id$
#
# Rules for building CDR server.
#
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Makefile variables
#--------------------------------------------------------------------------

DRV    =d:
CDR    =$(DRV)/cdr
XERCES =$(DRV)/usr/xerces
XERINC =$(XERCES)/include
!IF "$(DBG)" == ""
XERLIB =$(XERCES)/lib/xerces-c_3
!ELSE
XERLIB =$(XERCES)/lib/xerces-c_3D
!ENDIF
SABLOT =$(DRV)/usr/sablot
CDRINC =.
INC    =-I$(XERINC) -I$(SABLOT)/include -I$(CDRINC)
CC     =cl
BISON  =bison
LN     =link
CP     =cp -f
SVNUPD =svn update
CDRBIN =$(CDR)/bin
CDRLDIR=$(CDR)/lib
CDRSVR =$(CDRBIN)/CdrServer.exe
SERVICE=CdrService.exe
SECURE =-D_CRT_SECURE_NO_WARNINGS
NONSTD =-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_NON_CONFORMING_SWPRINTFS
COMPAT =$(SECURE) $(NONSTD)
CFLAGS =$(INC) -W3 -EHca -MD -GR -nologo $(COMPAT)
LFLAGS =/LIBPATH:$(SABLOT)/lib /LIBPATH:$(CDR)/lib
CDRLIB =cdr.lib
THRDPTY=$(XERLIB).lib sablot.lib
LIBS   =$(CDRLIB) $(THRDPTY) wsock32.lib odbc32.lib user32.lib AdvAPI32.lib 
SVCFLGS=-DCRTAPI1=_cdecl -DCRTAPI2=_cdecl -D_X86_=1 -D_WINNT \
        -D_WIN32_WINNT=0x0400 -DWINVER=0x0400 -DWIN32  -D_WIN32 -D_MT -MT
!IF "$(DBG)" == ""
CFLAGS =$(CFLAGS) -O2 -D_NDEBUG
!ELSE
CFLAGS =$(CFLAGS) -Zi
LFLAGS =$(LFLAGS) /DEBUG
SVCFLGS=-DDEBUG $(SVCFLGS)
!ENDIF
HDRS   =$(CDRINC)/CdrBlob.h \
        $(CDRINC)/CdrBlobExtern.h \
        $(CDRINC)/CdrCtl.h \
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
        $(CDRINC)/CdrSysValue.h \
        $(CDRINC)/CdrValidateDoc.h \
        $(CDRINC)/CdrValidationCtl.h \
        $(CDRINC)/CdrVersion.h \
        $(CDRINC)/CdrXsd.h \
        $(CDRINC)/CdrTiming.h \
        $(CDRINC)/CdrCache.h \
        $(CDRINC)/HeapDebug.h \
        $(CDRINC)/catchexp.h
LIBOBJS=CdrString.obj \
        CdrBlob.obj \
        CdrXsd.obj \
        CdrDom.obj \
        CdrDbConnection.obj \
        CdrDbStatement.obj \
        CdrDbPreparedStatement.obj \
        CdrDbResultSet.obj \
        CdrFilter.obj \
        CdrCache.obj \
        CdrGetDoc.obj \
        CdrLog.obj \
        CdrSession.obj \
        CdrParserInput.obj \
        CdrRegEx.obj \
        CdrVersion.obj \
        CdrTiming.obj \
        catchexp.obj \
        analyzeException.obj
OBJS   =CdrCommand.obj \
        CdrCtl.obj \
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
        CdrSysValue.obj \
        CdrReport.obj \
        CdrDocTypes.obj \
        CdrShutdown.obj \
        CdrGetTree.obj \
        CdrMergeProt.obj \
        CdrPublish.obj \
        CdrExternalMap.obj \
        CdrGlossaryMap.obj


#--------------------------------------------------------------------------
# File suffixes examined for rule interpretation
#--------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .obj .c .cpp

#--------------------------------------------------------------------------
# Targets
#--------------------------------------------------------------------------
all: svnupdate
	$(MAKE) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) _all

svnupdate:
	$(SVNUPD)

service: svnupdate
	$(MAKE) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) $(SERVICE)

install: svnupdate
	$(MAKE) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) _install

_all: init CdrServer.exe CdrTestClient.exe

_install: CdrServer.exe
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
	$(CC) -c $(SVCFLGS) CdrService.c

service.obj: service.c service.h
	$(CC) -c $(SVCFLGS) service.c

#--------------------------------------------------------------------------
# Rules for building tool for shutting down the server from a test-mode
# run.
#--------------------------------------------------------------------------

ShutdownCdr.exe: ShutdownCdr.cpp
	$(CC) $(CFLAGS) ShutdownCdr.cpp wsock32.lib

#--------------------------------------------------------------------------
# Rules for building test clients.
#--------------------------------------------------------------------------

CdrTestClient.exe: CdrTestClient.cpp catchexp.obj analyzeException.obj
	$(CC) $(CFLAGS) CdrTestClient.cpp catchexp.obj analyzeException.obj \
    wsock32.lib

tCdrRegEx.exe: tCdrRegEx.cpp $(CDRLIB) CdrCtl.obj CdrSession.obj
	$(CC) $(CFLAGS) tCdrRegEx.cpp CdrRegEx.obj CdrString.obj CdrLog.obj \
    CdrDbConnection.obj CdrDbResultSet.obj CdrDbStatement.obj \
    CdrCtl.obj CdrDbPreparedStatement.obj CdrDom.obj CdrSession.obj \
    $(XERLIB).lib wsock32.lib odbc32.lib advapi32.lib

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
	rm -f *.obj *~ *.bak CdrSearch_tab.c CdrSearch.tab.c cdr.lib
