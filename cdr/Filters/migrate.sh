#!/usr/bin/bash
# **************************************************************************
# File Name:	migrate.sh
# 		----------
# This script is used to migrate the current version of a filter from CVS
# to FRANCK.  
# It accepts multiple input parameters (CDR docID) on the command line.
# --------------------------------------------------------------------------
# Created:	Volker Englisch		2003-07-16
# **************************************************************************
# Checking input variables
# ------------------------
if [ "$1" = "" ]
then 
   echo "usage:  `basename $0` CDRID ..."
   exit 1
fi
 
# Setting Variables
# -----------------
CDRBIN="d:/home/venglisch/cdr/bin"
CVS="/cygdrive/d/bin/cvs.exe"
echo -n "Enter common text for Comment field: "
read TEXT

# Set the directory locations
# ---------------------------
if [ "$HOSTNAME" = "BACH" ]
then 
   cd f:/home/venglisch/cdr/Filters
else
   cd d:/home/venglisch/cdr/Filters
fi
pwd

echo ""
echo "Updating Filter to current Repository Version"
echo "============================================="
echo "***************"
echo ""

for i in $*
do
  CDRID=`echo $i | awk '{printf("CDR%010s\n", $1)}'`
  ID=$i
  VERS=`$CVS status $CDRID.xml | grep 'Repos' | awk '{print $3}'`
  echo "   $CDRID.xml"
  echo "   -----------------"
  $CVS status $CDRID.xml | grep "^   [WR][oe][rp][ko]"
  echo "   Updating   revision: $VERS"
  echo "   Updating    comment: \"$TEXT\""
  if [ "$HOSTNAME" = "franck" ]
  then 
    $CVS co -p cdr/Filters/$CDRID.xml > temp/${CDRID}_V$VERS.xml
  fi
  ###  $CDRBIN/checkoutcdrdoc.cmd      $ID     > dump
  $CDRBIN/VersionCdrDoc temp/${CDRID}_V$VERS.xml "CVS-V$VERS: $TEXT"
  echo ""
done

exit 0
