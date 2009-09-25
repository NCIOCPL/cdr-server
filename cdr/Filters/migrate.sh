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

CURRCVS=`echo $CVSROOT`
$CVS -d$CVSROOT logout  >  dump

CVSROOT=":pserver:venglisc:baseba11@verdi.nci.nih.gov:/usr/local/cvsroot"
$CVS -d$CVSROOT login   >> dump

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
echo "Updating Filter(s) to current Repository Version"
echo "------------------------------------------------"
echo ""

for i in $*
do
  CDRID=`echo $i | awk '{printf("CDR%010s\n", $1)}'`
  ID=$i
  VERS=`$CVS status $CDRID.xml | grep 'Repos' | awk '{print $3}'`
  echo "   $CDRID.xml"
  echo "   -----------------"
  $CVS status $CDRID.xml | grep "^   [WR][oe][rp][ko]" |\
       sed 's/\/usr\/local\/cvsroot\///'
  echo "   Updating   revision: $VERS"
  echo "   Updating w/ comment: \"$TEXT\""
  if [ "$HOSTNAME" = "franck" ]
  then 
    if [[ -f temp/${CDRID}_V$VERS.xml ]]
    then
      chmod 660 temp/${CDRID}_V$VERS.xml
    fi
    $CVS co -p cdr/Filters/$CDRID.xml > temp/${CDRID}_V$VERS.xml
  fi
  $CDRBIN/CdrCheckoutDoc.cmd      $ID     >> dump
  $CDRBIN/VersionCdrDoc temp/${CDRID}_V$VERS.xml "CVS-V$VERS: $TEXT"
  $CDRBIN/CdrUnlockDoc.cmd        $ID     >> dump
  echo ""
  echo "-------------------------------------------------------------------"
  echo ""
done

$CVS -d$CVSROOT logout  >> dump

CVSROOT="`echo $CURRCVS | sed 's/@/:baseba11@/'`"
$CVS -d$CVSROOT login   >> dump
exit 0
