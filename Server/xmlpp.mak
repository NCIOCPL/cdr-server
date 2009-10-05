.SUFFIXES:
../bin/xmlpp.exe: xmlpp.l
	flex xmlpp.l
	mv lexyy.c xmlpp.cpp
	cl /GX /Fe../bin/xmlpp.exe xmlpp.cpp
