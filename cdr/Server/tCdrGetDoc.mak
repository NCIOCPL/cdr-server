tCdrGetDoc.exe: tCdrGetDoc.cpp
	cl /MT /GR /GX /I../include /I/usr/xml4c/include \
tCdrGetDoc.cpp CdrGetDoc.obj ..\lib\cdr.lib odbc32.lib user32.lib
