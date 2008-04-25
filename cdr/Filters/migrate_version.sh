#!/usr/bin/bash
# **************************************************************************
# File Name:    migrate.sh
#               ---------- 
# This script is used to migrate the current version of a filter from CVS
# to FRANCK.
# It accepts multiple input parameters (CDR docID) on the command line.
#
# Once invoked, the script will request a comment to be entered which will
# be inserted as part of the CDR version.  If multiple documents are being
# versioned all documents will receive the same comment.  If the comment
# is left blank (enter return) the comments will be selected individually
# from the Filter_version.txt file.
# --------------------------------------------------------------------------
# Created:      Volker Englisch         2003-07-16
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
read CTEXT
 
# Set the directory locations
# ---------------------------
if [ "$HOSTNAME" = "BACH" ]
then
   cd f:/home/venglisch/cdr/Filters
else
   cd d:/home/venglisch/cdr/Filters
fi

echo ""
echo "Updating Filter to current Repository Version"
echo "============================================="
echo "***************"
echo ""
 
for i in $*
do
  CDRID=`echo $i | awk '{printf("CDR%010s\n", $1)}'`
  ID=$i
  VERS=`grep $CDRID Filter_version.txt | awk -F"/" '{print $3}'`

  # If common text for all CDR documents is not defined select the
  # comment text from the version document.
  # --------------------------------------------------------------
  if [ "$CTEXT" = "" ]
  then
    TEXT=`grep $CDRID Filter_version.txt | awk -F"/" '{print $4}'`
  else
    TEXT=$CTEXT
  fi

  echo "   $CDRID.xml"
  echo "   -----------------"
  $CVS status $CDRID.xml | grep "^   [WR][oe][rp][ko]"
  echo "   Updating   revision: $VERS"
  echo "   Updating    comment: \"$TEXT\""

  if [ "$HOSTNAME" = "franck" ]
  then
      $CVS co -r$VERS -p cdr/Filters/$CDRID.xml > temp/${CDRID}_V$VERS.xml
  fi

  ###  $CDRBIN/checkoutcdrdoc.cmd      $ID     > dump
  $CDRBIN/CdrReplacePubDoc temp/${CDRID}_V$VERS.xml "CVS-V$VERS: $TEXT"
  echo ""
done

exit 0
