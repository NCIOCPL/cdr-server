#---------------------------------------------------------------------
# Move XML from the cdr..all_doc_versions table to the
# cdr_archived_versions..doc_version_xml, copying the document ID
# and version number to identify the XML.
#
# This script should be run periodically, perhaps once or twice in a
# year, to keep the active database to a manageable size, while placing
# never changing static data (the versioned XML) in a larger but unchanging
# database that only needs a new backup and server refresh immediately
# after this script is run.  Backup and refresh times are dramatically
# reduced thereby, and bugs in Microsoft's copy programs that copy large
# files are avoided.
#
#                                               Author: Alan Meyer
#                                         Initial date: April, 2010
#
# BZIssue::4624
#---------------------------------------------------------------------
import sys, os.path, getopt, cdr, cdrdb

SCRIPT = "ArchiveDocVersionXML"

# These operations are expensive, leave plenty of time
TIMEOUT = 60 * 60 * 18

# Messages here
logf        = None
logFileName = "%s.log" % SCRIPT
logDirName  = cdr.DEFAULT_LOGDIR
logFullName = os.path.join(cdr.DEFAULT_LOGDIR, logFileName)

# Set program defaults
cfgBatchSize  = 1000
cfgBatchCount = 999999999
cfgSingleUser = False
cfgReadOnly   = False


def fatal(msg):
    """
    Log an error and abort.

    Pass:
        msg - Error message.
    """
    global logf

    fatalMsg = "FATAL ERROR: %s\n" % msg

    if logf:
        logf.write(fatalMsg, stderr=True)
        sys.stderr.write("Message logged to %s\n" % logFullName)

    else:
        sys.stderr.write(fatalMsg)

    # Abort
    sys.exit(1)

def usage():
    """
    Print usage message and exit.
    """
    sys.stderr.write("""
Move doc_version XML from the cdr to the cdr_archived_version database
on the server on which this script is run.

usage: %s {options} command
    options:
        --batchsize num   Copy this many rows in a batch.
                          High batchsize = most efficient.
                          Default=%d.
        --batchcount num  Copy this many batches, useful for debugging.
                          Default=all.
        --logpath dirpath Write messages to dirpath/%s.
                          Append if file already exists.
                          Default = %s.
        --readonly        Uses SQL account that can read but not write to DB.
        --help            Print this usage message.

    commands:
        copy        Do not null out xml, just copy to archive.
        null        Do not copy, just null xml already copied.
        reportcopy  No database update, just count how many would copy.
        reportnull  No database update, just count how many would null.

    A complete transfer of XML requires two runs:

        ArchiveDocVersionXML.py usr pw copy
        ArchiveDocVersionXML.py usr pw null

    However, either form is safe to run by itself.

    Errors are reported to stderr and appended to logfile.

Considerations to think about:

    Is there a current, verified backup of both the cdr and
    cdr_archived_versions?

    Have users been notified if a slowdown in operations is expected?

Notes:

    The option to run with the database in single user mode has been removed
    for compatibility with the CBIIT environment.  Read the code if you need
    to try it anyway.
""" % (SCRIPT, cfgBatchSize, logFileName, logDirName))
    sys.exit(1)

class SingleUser:
    """
    Controls all info used to go in or out of single user mode.

    Single user mode for the database used to be the default but is now
    optional and hidden.  It is optional because it is conceivable that there
    migght be a case where it can work and be useful and the code is already
    written and tested in the old OCE environment.

    It is hidden because it is untested in the CBIIT environment and does
    things that, if they are even allowed, could be deleterious to other
    users of CBIIT servers.
    """
    def __init__(self, logf):
        """
        Initialize.
        """
        global cfgReadOnly
        if cfgReadOnly:
            fatal("Cannot go into single user mode when in read only mode")

        self.logf = logf

        # True  = Single-user mode
        # False = Multi-user mode
        # None  = Unknown
        self.dbSingleUser = None

        # Hold an open connection, only allowed one in single user mode.
        self.conn = None

        # We try to restore services in the event of an error
        # But we don't want an error in restoreServices() to trigger
        #   itself recursively.
        self.inRestore = False

        # Which services were stopped, if any
        self.cdrServerStopped      = False
        self.cdrPubServiceStopped  = False
        self.SqlServerAgentStopped = False
        self.SqlServerStopped      = False


    def checkSingleUser(self):
        """
        Determine whether a database is in single user mode.

        Return:
            True  = CDR Database is in single user mode.
            False = Multi-user mode
            None  = Internal error
        """
        if not self.conn:
            fatal("Internal error, no connection available to checkSingleUser")

        row = None
        try:
            cursor = self.conn.cursor()
            cursor.execute("SP_DBOPTION 'cdr', 'single user'")
            row    = cursor.fetchone()
            cursor.close()
        except cdrdb.Error, info:
            self.logf.write(
                "Error checking single user mode for database: " % info)

        # Don't know what happened yet
        retVal = None

        if row:
            sqlResult = row[1].lower()
            if sqlResult == 'on':
                retVal = True
            elif sqlResult == 'off':
                retVal = False

        # Record results
        self.dbSingleUser = retVal

        # Log results
        self.logf.write("DB single-user mode = %s" % retVal, stdout=True)

        # Unknown result is ominous
        if retVal is None:
            self.logf.write('Unexpected result "%s" fetching single user mode'
                            ' for db' % sqlResult, stderr=True)

        return retVal


    def runCommand(self, cmd):
        """
        Execute a shell command.
        Check and log the results.

        Pass:
            cmd - Command string.

        Return:
            True  = Success
            False = Error
        """
        code   = None
        output = None
        error  = None

        try:
            result = cdr.runCommand(cmd)
        except Exception, info:
            # Exceptions are currently trapped in cdr.runCommand
            # But maybe we'll fix that
            self.logf.write('Exception running command "%s": %s' %
                             (cmd, info), stderr=True)

        # Log results
        if type(result) in (type(()), type([])):
            code, output, error = result
        else:
            self.logf.write("runCommand returned %s" % result)

        self.logf.write(('Ran command "%s"' % cmd,
                         '    return code = %s' % code,
                         '         output = "%s"' % output,
                         '          error = "%s"' % error), stdout=True)

        # No return code = success
        if not code:
            return True
        return False


    def setSingleUser(self):
        """
        Stop all CDR services.
        Put CDR database in single user mode.

        Return:
            Connection set in single user mode.
        """
        self.logf.write("Initiating setSingleUser")

        # Initial optimism
        okay = True

        # This is what we'll return
        self.conn = None

        # Error message
        msg = None

        # Stop any services that depend on multi-user mode, in the right order
        if not self.cdrPubServiceStopped:
            if not self.runCommand("net stop CdrPublishing"):
                # Failed
                okay = False
                msg = "Could not stop Publishing Service"
            else:
                self.cdrPubServiceStopped = True

        if okay and not self.cdrServerStopped:
            okay = self.runCommand("net stop Cdr")
            if okay:
                self.cdrServerStopped = True
            else:
                msg = "Could not stop CDR Service"

        # SQL Server
        if okay:
            # Must stop Agent first or else "net stop" will hang on a prompt
            #   asking if we understand that it will stop too.
            okay = self.runCommand("net stop SQLSERVERAGENT")
            if okay:
                self.SqlServerAgentStopped = True
            else:
                msg = "Could not stop SQL Server Agent"
        if okay:
            # Stop the service
            okay = self.runCommand("net stop MSSQLSERVER")
            if okay:
                self.SqlServerStopped = True
            else:
                msg = "Could not stop SQL Server"
        if okay:
            # Restart it
            okay = self.runCommand("net start MSSQLSERVER")
            if okay:
                self.SqlServerStopped = False
            else:
                msg = "Could not restart SQL Server"
        if okay:
            self.logf.write("Setting database to single-user mode",
                             stdout=True)
            try:
                # Must use cursor with autocommit because dboption can only
                #   be set in a single statement transaction.
                self.conn = cdrdb.connect()
                self.conn.setAutoCommit(True)
                cursor = self.conn.cursor()

                # Put cdr database in single user mode
                # No one will get to the other and probably no one will
                #   get to this one since the CdrServer is down.
                cursor.execute("SP_DBOPTION 'cdr', 'single user', 'true'")
            except cdrdb.Error, info:
                okay = False
                msg  = "Error setting single user mode: %s" % info

        # Make sure it really worked.
        if okay:
            okay = self.checkSingleUser()
            if okay:
                self.dbSingleUser = True
            else:
                msg = "checkSingleUser returned failure"

        # If anything broke, try to fix things and then quit
        if not okay:
            self.restoreServices()
            fatal("Exiting after setSingleUser failure: %s" % msg)

        self.logf.write("Completed setSingleUser")

        # Here's the single user's connection
        return self.conn


    def restoreServices(self):
        """
        Attempt to restore anything that is stopped, in the right order.
        """
        # Prevent recursive calls to restore
        if self.inRestore:
            return
        self.inRestore = True

        self.logf.write("Initiating restoreServices")

        okay = True
        msg  = None

        # Restore in right order
        if self.SqlServerStopped:
            okay = self.runCommand("net start MSSQLSERVER")
            if okay:
                self.SqlServerStopped = False

                # The connection we had is gone, replace it
                self.conn = cdrdb.connect()

                # By default, DB comes up in multi-user mode
                self.dbSingleUser = False
            else:
                msg = "Unable to restart SQL Server"

        if okay and self.dbSingleUser:
            # Immediately go to multi-user mode
            # XXX Not sure this will always work.
            if self.conn:
                self.logf.write("Restoring database to multi-user mode",
                                 stdout=True)
                try:
                    cursor = self.conn.cursor()
                    cursor.execute("SP_DBOPTION 'cdr', 'single user', 'false'")
                except cdrdb.Error, info:
                    okay = False
                    self.logf.write("Error going multi-user: %s" % info)

            if self.checkSingleUser():
                okay = False
                msg  = "Still in single user mode after trying to change"

        if okay and self.SqlServerAgentStopped:
            okay = self.runCommand("net start SQLSERVERAGENT")
            if okay:
                self.SqlServerStopped = False
            else:
                msg = "Unable to restart SQL Server Agent"

        if okay and self.cdrServerStopped:
            okay = self.runCommand("net start Cdr")
            if okay:
                self.cdrServerStopped = False
            else:
                msg  = "Unable to restart CdrServer"

        if okay and self.cdrPubServiceStopped:
            okay = self.runCommand("net start CdrPublishing")
            if okay:
                self.cdrPubServiceStopped = False
            else:
                msg  = "Unable to restart CdrPublishing"

        # Finished protected code
        self.inRestore = False

        if okay:
            self.logf.write("Completed restoreServices")
        else:
            self.log.write("Incomplete restore of services: %s" % msg)


# Usage
if len(sys.argv) < 2 or sys.argv[1] == '--help':
    print("len1=%d" % len(sys.argv))
    usage()

# Args
opts = "batchsize= batchcount= logpath= readonly singleuser help".split()
try:
    optlist, args = getopt.getopt(sys.argv[1:], "", opts)
except getopt.error, info:
    fatal("parsing arguments: %s" % info)

# Set config values from parms
for opt in optlist:
    if opt[0] == "--batchsize":
        try:
            cfgBatchSize = int(opt[1])
        except ValueError:
            fatal("--batchsize requires numeric argument")
    elif opt[0] == "--batchcount":
        try:
            cfgBatchCount = int(opt[1])
        except ValueError:
            fatal("--batchcount requires numeric argument")
    elif opt[0] == "--logpath":
        # Construct new full filename and test accessibility
        logDirName  = opt[1]
        logFullName = os.path.join(logDirName, logFileName)
        try:
            fp = open(logFullName, "a")
        except IOError as e:
            fatal("Cannot access %s: %s" % (logFullName, e.strerror))
        fp.close()
    elif opt[0] == "--readonly":
        cfgReadOnly = True
    elif opt[0] == "--singleuser":
        cfgSingleUser = True

# All command line args received?
if len(args) != 1:
    print("len2=%d" % len(args))
    usage()

# usr = args[0]
# pw  = args[1]
# cmd = args[2].lower()
cmd = args[0].lower()

##########
# Check user credentials.
#
# Removed the following.  If CBIIT is running the script, they would
# need CDR credentials - which they don't have and it might be inconvenient
# to give them.
#
# NOTE: If userid and password are to be required again, must re-add
#       them to command line parameter processing.
#
# session = cdr.login(usr, pw)
# if session.startswith("<Err"):
#     fatal("Unable to log in user '%s': %s" % (usr, session))
#
# # But don't need them for anything else and session will close
# #   anyway if we go to single user mode
# cdr.logout(session)
##########

# Construct queries based on config parameters
insertQry = """
    INSERT INTO cdr_archived_versions..doc_version_xml (id, num, xml)
    SELECT TOP %d v.id, v.num, v.xml
      FROM all_doc_versions v
     WHERE v.xml IS NOT NULL
       AND NOT EXISTS (
         SELECT a2.id, a2.num
           FROM cdr_archived_versions..doc_version_xml a2
          WHERE a2.id = v.id
            AND a2.num = v.num
       )
     ORDER BY v.id, v.num
""" % cfgBatchSize

# Nulling not done in batches?
# XXX Consult Bob/Volker about this
nullQry = """
    UPDATE dv
       SET xml = NULL
      FROM all_doc_versions dv
      JOIN cdr_archived_versions..doc_version_xml av
        ON av.id = dv.id
       AND av.num = dv.num
     WHERE av.xml IS NOT NULL
       AND dv.xml IS NOT NULL
"""

# Report how many would copy
reportCopyQry = """
    SELECT COUNT(DISTINCT v.id), COUNT (v.num)
      FROM all_doc_versions v
     WHERE v.xml IS NOT NULL
       AND NOT EXISTS (
         SELECT a2.id, a2.num
           FROM cdr_archived_versions..doc_version_xml a2
          WHERE a2.id = v.id
            AND a2.num = v.num
       )
"""

# Report how many would null
reportNullQry = """
    SELECT COUNT(DISTINCT v.id), COUNT (v.num)
      FROM all_doc_versions v
     WHERE v.xml IS NOT NULL
       AND EXISTS (
         SELECT a2.id, a2.num
           FROM cdr_archived_versions..doc_version_xml a2
          WHERE a2.id = v.id
            AND a2.num = v.num
       )
"""

# Select query for command
if   cmd == "copy":       qry = insertQry
elif cmd == "null":       qry = nullQry
elif cmd == "reportcopy": qry = reportCopyQry
elif cmd == "reportnull": qry = reportNullQry
else:
    fatal("Invalid command: %s" % cmd)

# Only need single-user mode for modifications, not reports
if cmd in ("reportcopy", "reportnull"):
    cfgSingleUser = False

# For debug
progParms = """
Program parameters:
    command       = %s
    cfgBatchSize  = %d
    cfgBatchCount = %d
    cfgSingleUser = %s
    logFullName   = %s
""" % (cmd, cfgBatchSize, cfgBatchCount, cfgSingleUser, logFullName)

# Start logging
logf = cdr.Log(logFileName, logDirName, logPID=False)
logf.write(progParms)
logf.write("Will execute query: %s" % qry)

######## DEBUG #########
# sys.exit(1)
######## DEBUG #########

# Stats
batchesStats  = 0
documentStats = 0
versionStats  = 0

suObj = None
conn  = None

if cfgSingleUser:
    # Create an object for controlling CDR and database access
    suObj = SingleUser(logf)

    # Bring down Cdr, CdrPublishing, SQL Server
    # Restart SQL Server and put it in single user mode
    # Returned connection should be in single user mode
    conn = suObj.setSingleUser()
else:
    try:
        if cfgReadOnly:
            conn = cdrdb.connect("CdrGuest")
        else:
            conn = cdrdb.connect()
            conn.setAutoCommit(True)
    except cdrdb.Error, info:
        fatal("Unable to connect to SQL Server: %s" % info)

cursor = conn.cursor()

# Execute
i = 0
while i < cfgBatchCount:
    try:
        logf.write("Starting query", stdout=True)
        cursor.execute(qry, timeout=TIMEOUT)
        logf.write("Finished query", stdout=True)

        # Stats
        batchesStats += 1
        if cmd in ("copy", "null"):

            # No more rows?
            if cursor.rowcount == 0:
                break

            versionStats += cursor.rowcount
        else:
            row = cursor.fetchone()
            documentStats += row[0]
            versionStats  += row[1]

        if cmd == "copy":
            logf.write("Batch: %d   Total rows copied = %d" %
                        (batchesStats, versionStats), stdout=True)

        if cmd == "null":
            logf.write("Total items nulled = %d" % versionStats, stdout=True)
            # XXX No batches?
            break

        if cmd in ("reportcopy", "reportnull"):
            logf.write("""Total rows that would be affected:
        Unique documents: %8d
         Unique versions: %8d""" % (documentStats, versionStats), stdout=True)
            # No batches
            break

    except cdrdb.Error, info:
        fatal("Database error: %s" % info)

    i += 1

if suObj:
    # Bring everythng back up
    suObj.restoreServices()

# That's all
logf.write("Completed without errors", stdout=True)
