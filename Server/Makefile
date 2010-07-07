#--------------------------------------------------------------------------
#
# $Id$
#
# Rules for building CDR server.
#
# The 'VCVER' macro is created so that we can point the linker to the
# correct location for the regular expression libraries.  The values
# track the versions of the Microsoft runtime libraries (e.g., 7 or
# 71 or 8), not the version numbers for the C++ compiler (which jump
# around with a less predictable pattern).
#
# $Log: not supported by cvs2svn $
# Initial revision: 2000/04/11 14:15:32  bkline
# See svn log for history.
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# Makefile variables
#--------------------------------------------------------------------------

DRV    =d:
CDR    =$(DRV)/cdr
XERCES =$(DRV)/usr/xerces
XERINC =$(XERCES)/src
!IF "$(DBG)" == ""
XERLIB =$(XERCES)/lib/xerces-c_2
!ELSE
XERLIB =$(XERCES)/lib/xerces-c_2D
!ENDIF
SABLOT =$(DRV)/usr/sablot
REGEX  =$(DRV)/usr/regex
VCVER  =71
REVER  =vc$(VCVER)
CDRINC =.
INC    =-I$(XERINC) -I$(SABLOT)/include -I$(CDRINC) -I$(REGEX)
CC     =cl
BISON  =bison
LN     =link
CP     =cp -f
CVSUPD =svn update
DOC    =$(DRV)/InetPub/wwwroot/cdr/ccdoc/
DOCS   =$(DOC)default.htm
CDRBIN =$(CDR)/bin
SERVICE=$(CDRBIN)/CdrService.exe
CFLAGS =$(INC) -W3 -GX -EHa -MD -GR -nologo
LFLAGS =/LIBPATH:$(XML4C)/lib /LIBPATH:$(SABLOT)/lib \
        /LIBPATH:$(CDR)/lib /LIBPATH:$(REGEX)/libs/regex/build/$(REVER)
CDRLIB =cdr.lib
LIBS   =$(CDRLIB) $(XERLIB).lib sablot.lib wsock32.lib odbc32.lib user32.lib \
        AdvAPI32.lib
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
        CdrValidateDoc.obj \
        CdrVersion.obj \
        CdrTiming.obj \
        catchexp.obj \
        analyzeException.obj \
        CdrCommand.obj \
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
all: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) _all

cvsupdate:
	$(CVSUPD)

service: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) CdrService.exe

install: cvsupdate
	$(MAKE) VCVER=$(VCVER) DRV=$(DRV) SABLOT=$(SABLOT) DBG=$(DBG) _install

_all: init CdrServer.exe CdrTestClient.exe ParseSchema.exe CdrService.exe \
    # CdrDocs

_install: CdrServer.exe CdrDocs
	net stop Cdr
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

CdrServer.exe: CdrServer.obj $(CDRLIB)
	$(LN) $(LFLAGS) /out:CdrServer.exe CdrServer.obj $(LIBS) >> errs

#--------------------------------------------------------------------------
# Rule for building CDR service.
#--------------------------------------------------------------------------

CdrService.exe: CdrService.obj service.obj
	$(CC) -MT CdrService.obj service.obj advapi32.lib

CdrService.obj: CdrService.c service.h
	$(CC) -c $(SVCFLGS) CdrService.c

service.obj: service.c service.h
	$(CC) -c $(SVCFLGS) service.c

#--------------------------------------------------------------------------
# Rule for building schema parser.
#
# /OPT:NOREF bloats the code but prevents a warning on ADVAPI32.DLL
#  being discarded by default /OPT:REF processing.
# See Bugzilla BZIssue::4663 comment #4.
#--------------------------------------------------------------------------
ParseSchema.exe: ParseSchema.obj $(HDRS) $(CDRLIB)
	$(CC) $(CFLAGS) ParseSchema.cpp /link $(LFLAGS) /ignore:4089 $(LIBS) >> errs

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
    if exist $(DOC) (rm $(DOC)ccdoc*html)
    if exist $(DOC) ( \
      ccdoc -html $(DOC) -db $(DOC)ccdoc.db -root cdr \
            -header $(DOC)save/serverhdr.html $(HDRS) \
    )
    if exist $(DOC) ($(CP) $(DOC)ccdoc.cdr.pkg.html $(DOCS))

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
	rm -f *.obj *~ *.bak CdrSearch_tab.c CdrSearch.tab.c ccdoc.ctf cdr.lib
