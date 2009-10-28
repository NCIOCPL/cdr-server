# *********************************************************************
#
# $Id$
#
# Promote filters to FRANCK and BACH
#
# $Log: not supported by cvs2svn $
# Revision 1.2  2009/07/28 21:48:02  venglisc
# Fixed some bugs found during testing. (Bug 4608)
#
# Revision 1.1  2009/07/27 18:16:23  venglisc
# Initial copy of the new program to promote a filter to the CDR. (Bug 4608)
#
# *********************************************************************

#----------------------------------------------------------------------
# Import required modules.
#----------------------------------------------------------------------
import cdr, os, sys, string, socket

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
    sys.stderr.write('usage: promote.py user passwd NNNNN[-V]\n')
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
# Checking out the documents that need to be promoted
# Every document will be checked out twice: the latest CVS version and
# the version to be promoted.
# Only FRANCK does the checking out; BACH only uses the documents that
# had been created/promoted earlier to ensure the versions on both 
# systems are identical.
# If we're just promoting the latest CVS version the versions might get
# out of sync if a new CVS version is being created after the version 
# has been promoted to FRANCK.
#----------------------------------------------------------------------
def getFilterVersion(docId, version = None):

    # Set up cvs strings and repository
    # ---------------------------------
    cvsroot = "-d:pserver:anon@%s" % cdr.CVSROOT
    l.write("initializing CVS workspace: \n     CVSROOT=%s" % cvsroot)

    cvsDoc   = 'CDR%010d.xml' % docId
    cvsRevDoc= 'CDR%010d_V1.%s.xml'
    cvsRevision = [-1, -1]

    # Change to the CVS repository, if it doesn't exist create it
    # -----------------------------------------------------------
    try:
        os.chdir(filtPath)
    except Exception, info:
        l.write("*** Warning: chdir %s\n%s" % (filtPath, str(info)), 
                                                                stdout = True)
        l.write("*** Creating CVS repository in %s" % filtPath, stdout = True)
        l.write("",                                             stdout = True)
        os.chdir(tmpPath)
        cmd = "cvs %s checkout -d %s cdr/Filters/CDR0000000140.xml" % (cvsroot,
                                                                       filtDir)
        res = cdr.runCommand(cmd)

    # Check out the latest version of the document to be promoted
    # (This is done so it is a member of your sandbox)
    # -----------------------------------------------------------
    try:
        try:
            l.write("checking out %s" % cvsDoc)
            os.chdir(tmpPath)
            cmd = "cvs %s checkout -d %s cdr/Filters/%s" % (cvsroot, 
                                                            filtDir, cvsDoc)
            res = cdr.runCommand(cmd)
        except Exception, info:
            l.write("*** Error: cvs checkout %s failed\n%s" % (cvsDoc, 
                                                     str(info)), stdout = True)

        # Identify the version ID of the last version in CVS
        # --------------------------------------------------
        os.chdir(filtPath)
        cmd = "cvs %s status %s" % (cvsroot, cvsDoc)
        res = cdr.runCommand(cmd)

        result = res.output.splitlines()
        cvsRevision = result[3].rsplit('.')

        if not version:  version = cvsRevision[1]

        # Check out the version that needs to be promoted, this is
        # possibly an older version
        # --------------------------------------------------------
        cmd = "cvs %s checkout -r1.%s -p cdr/Filters/%s" % (cvsroot, 
                                                            version, 
                                                            cvsDoc)
        os.chdir(tmpPath)
        res = cdr.runCommand(cmd, joinErr2Out = False, 
                                  returnNoneOnSuccess = False)
        os.chdir(filtPath)
        
        file= "%s\\%s" % (filtPath, cvsRevDoc % (docId, version))
        f   = open(file, 'w')
        f.write(res.output.replace('\r', ''))
        f.close()

    except Exception, info:
        errorMessage = "Unknown failure running CVS command: %s" % str(info)
    except:
        errorMessage = "REALLY unknown failure running CVS command!!!"

    return cvsRevision[1]  


# ===================================================================
# Main Starts here
# ===================================================================
l.write('promote.py - Started',     stdout = False)
l.write('Arguments: %s' % sys.argv, stdout = False)
l.write('',                         stdout = False)

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
    if localhost == 'Prod':
        if not cvsVersion:
            sys.exit("*** Error: Must specify Version for BACH")

    # Getting CVS info
    # ------------------
    lastCvsVersion = getFilterVersion(docId, cvsVersion)

    version = cvsVersion or lastCvsVersion

    cmd = "cvs status CDR%010d.xml" % docId
    res = cdr.runCommand(cmd, joinErr2Out = False, 
                              returnNoneOnSuccess = False)
    
    if res.code:
        l.write( '*** Error in CVS status: %s' %            res.output)
        sys.exit('*** Error in CVS status: \n%s\n%s\n%s' % (res.output,
                                                            res.error,
                                                            res.code))

    statInfo = res.output.splitlines()
    l.write(statInfo[3], stdout = True)
    l.write(statInfo[4].replace('/usr/local/cvsroot/', ''), stdout = True)

    # If we're updating an older cvsVersion check first if it exists
    # -----------------------------------------------------------
    if not filterIds[2] == '':
        if int(cvsVersion) > int(lastCvsVersion):
            l.write("*** Error:   Cannot promote specified version")
            sys.exit("*** Error:  Cannot promote specified version")

    l.write(" ---------------------------",           stdout = True)
    l.write("   Updating   Revision: 1.%s" % version, stdout = True)
    l.write('   Updating w/ comment: "%s"' % comment, stdout = True)
    l.write("",                                       stdout = True)

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
        os.chdir(filtPath)
        res = cdr.repDoc((user, passwd), 
                         file = 'CDR%010d_V1.%s.xml' % (docId, version), 
                         checkIn = 'Y', ver = 'Y', val = 'Y', 
                         comment = "CVS-V1.%s: %s" % (version, comment))
        print res
        l.write("----------------------------------------------------------",
                    stdout = True)
        l.write("", stdout = True)
    except Exception, info:
        l.write("*** Error: Replacing document failed\n%s" % str(info), 
                                                         stdout = True)
        sys.exit("           Confirm that document isn't locked")
sys.exit(0)
