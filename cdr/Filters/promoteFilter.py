# *********************************************************************
#
# $Id: promoteFilter.py,v 1.1 2009-07-27 18:17:42 venglisc Exp $
#
# Promote filters to FRANCK and BACH
# The filter
#
# $Log: not supported by cvs2svn $
# *********************************************************************

#----------------------------------------------------------------------
# Import required modules.
#----------------------------------------------------------------------
import cdr, os, sys, tempfile, string, socket, time, urllib2

#----------------------------------------------------------------------
# Edit only on Dev machine.
#----------------------------------------------------------------------
localhost = socket.gethostname()
if string.upper(localhost) == "FRANCKx" or \
   string.upper(localhost) == "MAHLER":
    localhost = "Dev"
# elif string.upper(localhost) == "BACH":
elif string.upper(localhost) == "FRANCK":
    localhost = "Prod"

if len(sys.argv) < 4:
    sys.stderr.write('usage: promote.py user passwd NNNNN-V\n')
    sys.stderr.write('       NNNNN = filter ID\n')
    sys.stderr.write('           V = filter version\n')
    sys.exit(1)

#----------------------------------------------------------------------
# Set some initial values.
#----------------------------------------------------------------------
LOGNAME   = "promote-filter.log"
l         = cdr.Log(LOGNAME)

tmpPath   = 'D:\\cdr\\tmp'
l.write("%s" % tmpPath)
filtDir   = 'promoteFilters'
filtPath  = '%s\\%s' % (tmpPath, filtDir)

user      = sys.argv[1]
passwd    = sys.argv[2]
filters   = sys.argv[3:]
debugging = 1

#----------------------------------------------------------------------
# Logging to keep an eye on problems (mostly with CVS).
#----------------------------------------------------------------------
def debugLog(what):
    if debugging:
        try:
            f = open(LOGNAME + "d", "a")
            f.write("%s: %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S"), what))
            f.close()
        except Exception, info:
            sys.exit("Failure writing to %sd: %s" % (LOGNAME, str(info)))

#----------------------------------------------------------------------
# Object for results of an external command.
#----------------------------------------------------------------------
class CommandResult:
    def __init__(self, code, output):
        self.code   = code
        self.output = output

#----------------------------------------------------------------------
# Run an external command.
#----------------------------------------------------------------------
def runCommand(command):
    debugLog("runCommand(%s)" % command)
    try:
        commandStream = os.popen('%s 2>&1' % command)
        output = commandStream.read()
        code = commandStream.close()
        return CommandResult(code, output)
    except Exception, info:
        debugLog("failure running command: %s" % str(info))


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
l.write('promote.py - Started',     stdout = False)
l.write('Arguments: %s' % sys.argv, stdout = False)
l.write('',                         stdout = False)

# Prompt user for comment to use to promote filter
# -------------------------------------------------
comment    = raw_input('Enter text for filter comment: ') or None
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
    cvsVersion = filterIds[2][1:]
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
    l.write("", stdout = True)

    # Checking out document before we can copy the filter
    # ---------------------------------------------------
    try:
        cdr.getDoc((user, passwd), docId, 'Y')
    except Exception, info:
        print "Checking out document failed\n%s" % str(info)

    # Promote the filter to the CDR server and unlock the document
    # ------------------------------------------------------------
    try:
        #os.chdir(filtPath)
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
