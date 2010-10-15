# *********************************************************************
#
# $Id: promoteFilter.py,v 1.2 2009-07-28 21:48:02 venglisc Exp $
#
# Promote filters to FRANCK and BACH
#
# $Log: not supported by cvs2svn $
# Revision 1.1  2009/07/27 18:17:42  venglisc
# Initial copy of the program to promote a filter to the CDR.  This program
# is similar to promote.py but it retrieves the CVS copy via HTTP instead
# of running a shell command. (Bug 4608)
#
# *********************************************************************

#----------------------------------------------------------------------
# Import required modules.
#----------------------------------------------------------------------
import cdr, os, sys, string, socket, urllib2

#----------------------------------------------------------------------
# Edit only on Dev machine.
#----------------------------------------------------------------------
localhost = socket.gethostname()
if string.upper(localhost) == "FRANCK" or \
   string.upper(localhost) == "MAHLER":
    localhost = "Dev"
elif string.upper(localhost) == "BACH":
    localhost = "Prod"

if len(sys.argv) < 4:
    sys.stderr.write('usage: promote.py user passwd NNNNN-V\n')
    sys.stderr.write('       NNNNN = filter ID\n')
    sys.stderr.write('           V = filter version (integer)\n')
    sys.exit(1)

#----------------------------------------------------------------------
# Set some initial values.
#----------------------------------------------------------------------
LOGNAME   = "promote-filter.log"
l         = cdr.Log(LOGNAME)

tmpPath   = 'D:\\cdr\\tmp'
l.write("tempPath: %s" % tmpPath)
filtDir   = 'promoteFilters'
filtPath  = '%s\\%s' % (tmpPath, filtDir)

user      = sys.argv[1]
passwd    = sys.argv[2]
filters   = sys.argv[3:]

#----------------------------------------------------------------------
# Checking out the documents that need to be promoted via http.
#----------------------------------------------------------------------
def getFilterVersion(docId, version = None):

    # Set up cvs strings and repository
    # ---------------------------------
    cvshost = "verdi.nci.nih.gov"
    app     = 'cgi-bin/cdr/cvsweb.cgi/~checkout~/cdr/Filters'
    rev     = '1.%s' % version
    parms   = 'CDR%010d.xml?rev=%s;content-type=application/xml' % (docId, rev)
    url     = 'http://%s/%s/%s' % (cvshost, app, parms)
    reader  = urllib2.urlopen(url)
    doc     = reader.read()

    file= "CDR%010d_V%s.xml" % (docId, rev)
    f   = open(file, 'w')
    f.write(doc)
    f.close()

    return file

# ===================================================================
# Main Starts here
# ===================================================================
l.write('promoteFilter.py - Started', stdout = False)
l.write('Arguments: %s' % sys.argv,   stdout = False)
l.write('',                           stdout = False)

# Prompt user for comment to use to promote filter
# -------------------------------------------------
comment    = raw_input('\nEnter text for filter comment: ') or None
l.write("Comment: %s" % comment)
if not comment:
   l.write("*** Error:  Can't promote without comment")
   sys.exit("*** Error: Can't promote without comment")

l.write("",                                                 stdout = True) 
l.write("Updating Filter(s) to current Repository Version", stdout = True)
l.write("================================================", stdout = True)
l.write("",                                                 stdout = True)

for filtId in filters:
    # If a filter ID has been specified as "NNNNN-K" the "K" is an older
    # version of the document that should be promoted.  When using the 
    # cdr.exNormalize function it's part of the fragment ID
    # ------------------------------------------------------------------
    filterIds = cdr.exNormalize(filtId)
    docId     = filterIds[1]
    try:
        cvsVersion = int(filterIds[2][1:])
    except:
        l.write("*** Error:  Version must be an integer")
        sys.exit("*** Error: Version must be an integer: %s" % str(cvsVersion))

    l.write("  CDR%010d.xml" % filterIds[1], stdout = True)
    l.write("  -----------------",           stdout = True)
    l.write("",                              stdout = True)

    # Filters must be migrated by CVS version when going to the 
    # production server
    # ---------------------------------------------------------
    if not cvsVersion:
        sys.exit("*** Error: Must specify Version")

    # Getting CVS info
    # ------------------
    cvsFile = getFilterVersion(docId, cvsVersion)

    l.write("   Copy   Revision: 1.%s" % cvsVersion, stdout = True)
    l.write('   Copy w/ comment: "%s"' % comment,    stdout = True)
    l.write("",                                      stdout = True)

    # Checking out document before we can copy the filter
    # ---------------------------------------------------
    try:
        coDoc = cdr.getDoc((user, passwd), docId, 'Y')
        if coDoc.startswith("<Errors"):
            l.write("*** Error: Checking out document unsuccessful\n%s" %
                                                  coDoc, stdout = True)
            sys.exit(1)

    except Exception, info:
        l.write("*** Error:  Checking out document failed\n%s" % str(info))
        sys.exit("*** Error: Checking out document failed - %s" % str(info))

    # Promote the filter to the CDR server and unlock the document
    # ------------------------------------------------------------
    try:
        res = cdr.repDoc((user, passwd), 
                         file = cvsFile, 
                         checkIn = 'Y', ver = 'Y', val = 'Y', 
                         comment = "CVS-V1.%s: %s" % (cvsVersion, comment))
        print res
        l.write("----------------------------------------------------------",
                    stdout = True)
        l.write("", stdout = True)
    except Exception, info:
        print "Replacing document failed\n%s" % str(info)

sys.exit(0)
