# *********************************************************************
#
# $Id: promoteFilter.py,v 1.2 2009-07-28 21:48:02 venglisc Exp $
#
# Promote filters to FRANCK and BACH
#
# Revision 1.1  2009/07/27 18:17:42  venglisc
# Initial copy of the program to promote a filter to the CDR.  This program
# is similar to promote.py but it retrieves the CVS copy via HTTP instead
# of running a shell command. (Bug 4608)
#
# *********************************************************************

#----------------------------------------------------------------------
# Import required modules.
#----------------------------------------------------------------------
import cdr, os, sys, string, socket, urllib2, time, pysvn

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
    sys.stderr.write('usage: promoteFilter.py user passwd NNNNN\n')
    sys.stderr.write('       NNNNN = filter ID\n')
    sys.stderr.write('       user/passwd = NIH credentials\n')
    #sys.stderr.write('           V = filter version (integer)\n')
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

user      = 'venglisc'
passwd    = 'gimte'
svnuid    = sys.argv[1]
svnpwd    = sys.argv[2]
filters   = sys.argv[3:]


# -------------------------------------------------------------------------
# Function to update the current working directory (a.k.a sandbox) to the
# current revision.
# Note:  By default we always want to update the latest repository copy
# -------------------------------------------------------------------------
def updateSvn(svnid, svnpw):
    #----------------------------------------------------------------------
    # Callback Functions to access SVN repository
    #----------------------------------------------------------------------
    def getLogin(realm, username, maySave):
        #l.write("getLogin: realm=%s; username=%s; maySave=%s" % (realm,
        #                                                          username,
        #                                                          maySave))
        return True, svnuid, svnpwd, False

    def trustCert(td):
        #l.write("trustCert: %s" % repr(td))
        return True, td['failures'], False

    # Set up svn strings and directories
    svnbase = cdr.SVNBASE
    now = time.strftime("%Y%m%d%H%M%S")
    wd = "%s-%s" % (now, os.getpid())

    l.write(" ")
    l.write("Initializing SVN workspace:")
    l.write("SVNBASE = %s" % svnbase)

    errorMessage = ""

    # If we're not in a SVN sandbox containing this file we exit again
    # ----------------------------------------------------------------
    try:
        # Set up svn login
        # ----------------
        l.write("Updating directory to latest revision")
        client = pysvn.Client()
        client.callback_get_login = getLogin
        client.callback_ssl_server_trust_prompt = trustCert

        path = os.getcwd()
        svnrev = client.update(path, recurse = False)
    except Exception, info:
        errorMessage = "Unknown failure running SVN command: %s" % str(info)
    except:
        errorMessage = "REALLY unknown failure running SVN command!!!"

    l.write("SVN update Done.")

    return svnrev


# -------------------------------------------------------------------------
# Function to get the last commit version of the filter to be udpated.
# -------------------------------------------------------------------------
def statusSvn(docId, svnid, svnpw):
    #----------------------------------------------------------------------
    # Callback Functions to access SVN repository
    #----------------------------------------------------------------------
    def getLogin(realm, username, maySave):
        return True, svnuid, svnpwd, False

    def trustCert(td):
        return True, td['failures'], False

    try:
        # Set up svn login
        # ----------------
        l.write("Getting filter info")
        client = pysvn.Client()
        client.callback_get_login = getLogin
        client.callback_ssl_server_trust_prompt = trustCert

        path = os.getcwd()
        filter = 'CDR%010d.xml' % docId
        filename = path + '\\' + filter
        status = client.status(filename)
        docStatus = status[0]
        revRep = docStatus.entry.revision.number
        revDoc = docStatus.entry.commit_revision.number
    except:
        return False

    l.write("SVN status Done.")
    return (revRep, revDoc)


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

# Updating sandbox to latest version
# ----------------------------------
rev = updateSvn(svnuid, svnpwd)
l.write("Repository Rev:  %s" % rev[0].number, stdout = True)
l.write("", stdout = True)

for filtId in filters:
    # If a filter ID has been specified as "NNNNN-K" the "K" is an older
    # version of the document that should be promoted.  When using the 
    # cdr.exNormalize function it's part of the fragment ID
    # ------------------------------------------------------------------
    filterIds = cdr.exNormalize(filtId)
    docId     = filterIds[1]
    filterDoc = 'CDR%010d.xml' % filterIds[1]

    l.write("%s" % filterDoc,     stdout = True)
    l.write("-----------------",  stdout = True)

    # Getting SVN info
    # ------------------
    revisions = statusSvn(docId, svnuid, svnpwd)
    if not revisions:
        sys.exit("\n*** Filter does not exist in working directory!!!")
    l.write("Change Revision: %d" % revisions[1], stdout = True)
    l.write("CDR comment:     %s" % comment,     stdout = True)
    l.write("",                                   stdout = True)

    # Checking out document from CDR before we can update the filter
    # --------------------------------------------------------------
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
                         file = filterDoc, 
                         checkIn = 'Y', ver = 'Y', val = 'Y', 
                         comment = "SVN-R%d(%d): %s" % (revisions[0], 
                                                        revisions[1], comment))
        print res
        l.write("----------------------------------------------------------",
                    stdout = True)
        l.write("", stdout = True)
    except Exception, info:
        print "Replacing document failed\n%s" % str(info)

sys.exit(0)
