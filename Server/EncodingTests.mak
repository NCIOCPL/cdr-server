#--------------------------------------------------------------------------
#
# $Id$
#
# Rules for building test programs for blob encoding/decoding.
#
# $Log: not supported by cvs2svn $
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
CDRLIB =$(CDR)/lib/cdr.lib
HDRS   =$(CDRINC)/CdrBlob.h \
        $(CDRINC)/CdrCommand.h \
        $(CDRINC)/CdrDbConnection.h \
        $(CDRINC)/CdrDbPreparedStatement.h \
        $(CDRINC)/CdrDbResultSet.h \
        $(CDRINC)/CdrDbStatement.h \
        $(CDRINC)/CdrDoc.h \
        $(CDRINC)/CdrDom.h \
        $(CDRINC)/CdrException.h \
        $(CDRINC)/CdrGetDoc.h \
        $(CDRINC)/CdrInt.h \
        $(CDRINC)/CdrLink.h \
        $(CDRINC)/CdrLinkProcs.h \
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
all: EncodeBlob.exe DecodeBlob.exe

#--------------------------------------------------------------------------
# Rules for building test programs.
#--------------------------------------------------------------------------
EncodeBlob.exe: EncodeBlob.obj $(CDRLIB)
	$(LN) $(LFLAGS) /out:EncodeBlob.exe EncodeBlob.obj $(LIBS) >> errs

EncodeBlob.obj: EncodeBlob.cpp $(HDRS)
	$(CC) $(CFLAGS) -c EncodeBlob.cpp
    
DecodeBlob.exe: DecodeBlob.obj $(CDRLIB)
	$(LN) $(LFLAGS) /out:DecodeBlob.exe DecodeBlob.obj $(LIBS) >> errs

DecodeBlob.obj: DecodeBlob.cpp $(HDRS)
	$(CC) $(CFLAGS) -c DecodeBlob.cpp

#--------------------------------------------------------------------------
# Use the central makefile for building the library.
#--------------------------------------------------------------------------
$(CDRLIB):
	nmake $(CDRLIB)

#--------------------------------------------------------------------------
# Generic rules for object files
#--------------------------------------------------------------------------
.c.obj:
	$(CC) -c $(CFLAGS) $*.c >> errs

.cpp.obj:
	$(CC) -c $(CFLAGS) $*.cpp >> errs
