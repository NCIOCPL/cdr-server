#!/usr/bin/bash
# **************************************************************************
# File Name:	check_pversion.sh
# 		-----------------
# This script is used to check the current version that is in CDR.  It
# extracts a given version from CVS, extracts the current version from CDR
# and compares them both.
# It accepts two parameters (CDR docID, Version number) on the command line.
# --------------------------------------------------------------------------
# Created:	Volker Englisch		2003-07-28
# **************************************************************************
# Checking input variables
# ------------------------
if [ "$2" = "" ]
then 
   echo "usage:  `basename $0` CDRID Version"
   exit 1
fi
 
# Setting Variables
# -----------------
CDRBIN="d:/home/venglisch/cdr/bin"

# Set the directory locations
# ---------------------------
cd d:/home/venglisch/verdi/Filters

echo ""
echo "Retrieve Filter version from CVS Repository Version"
echo "==================================================="
echo "***************"
echo ""

CDRID=`echo $1 | awk '{printf("CDR%010s\n", $1)}'`
cvs co -r$2 -p cdr/Filters/$CDRID.xml > temp/$1_$2.xml
$CDRBIN/GetCdrDoc.cmd $1          > temp/$1_cdr.xml
diff temp/$1_$2.xml temp/$1_cdr.xml | d:/bin/less.bat

exit 0
