#!/usr/bin/bash
# **************************************************************************
# File Name:	all_versions.sh
# 		---------------
# This script creates a list of all of the filters along with their version
# --------------------------------------------------------------------------
# Created:	Volker Englisch		2004-01-13
# **************************************************************************
# Checking input variables
# ------------------------
# if [ "$1" = "" ]
# then 
#    echo "usage:  `basename $0` CDRID ..."
#    exit 1
# fi
#  
# Setting Variables
# -----------------
CDRBIN="d:/home/venglisch/cdr/bin"

# Set the directory locations
# ---------------------------
if [ "$HOSTNAME" = "bach" ]
then 
   echo "List must be created on FRANCK"
   exit 1
else
   cd d:/home/venglisch/verdi/Filters
fi
pwd
# Bringing directory listing up-to-date
# -----------------------------------
cvs update CDR*.xml

echo ""
echo "Creating List..."

echo "List of all CDR Filters `date +'%x'`"       > filters_sort.txt
echo "======================================="   >> filters_sort.txt
echo ""                                          >> filters_sort.txt
echo "Filter ID         Vers  Last Update"       >> filters_sort.txt
echo "----------------- ----- ---------------"   >> filters_sort.txt
awk -F"/" '{printf("%14s %5s %s\n", $2,$3,$4)}' CVS/Entries |\
     grep CDR |\
     cut -c1-35,45-48 |\
     sort                                        >> filters_sort.txt

echo "List of all CDR Filters `date +'%x'`"       > filters_date.txt
echo "======================================="   >> filters_date.txt
echo ""                                          >> filters_date.txt
echo "Filter ID         Vers  Last Update"       >> filters_date.txt
echo "----------------- ----- ---------------"   >> filters_date.txt
awk -F"/" '{printf("%14s %5s %s\n", $2,$3,$4)}' CVS/Entries |
     grep CDR |\
     cut -c1-35,45-48 |\
     sort  -k6 -k4M -k5                          >> filters_date.txt

echo "Done."
exit 0
