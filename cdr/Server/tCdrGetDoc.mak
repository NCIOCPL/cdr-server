../bin/tCdrGetDoc.exe: tCdrGetDoc.cpp CdrGetDoc.obj ../lib/cdr.lib
	cl -MT -GR -GX -I../include /I/usr/xml4c/include \
tCdrGetDoc.cpp CdrGetDoc.obj -Fe../bin/tCdrGetDoc.exe ../lib/cdr.lib \
odbc32.lib user32.lib
