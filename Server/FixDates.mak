FixDates.exe: FixDates.cpp ..\lib\cdr.lib
	cl /MT /GR /GX /I../include /I/usr/xml4c/include \
FixDates.cpp ..\lib\cdr.lib odbc32.lib user32.lib
