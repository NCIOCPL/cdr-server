xmlpp.exe: xmlpp.l
	flex xmlpp.l
	mv lexyy.c xmlpp.cpp
	cl /GX xmlpp.cpp

