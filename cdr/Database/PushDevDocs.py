#----------------------------------------------------------------------
#
# $Id: PushDevDocs.py,v 1.1 2002-09-07 14:12:49 bkline Exp $
#
# Replaces copies of CDR control documents which have been preserved
# from the development server, after a refresh of the database on
# the development server from the production server.  This allows us
# to work with the users' current production documents without losing
# work done by developers on the development server.
#----------------------------------------------------------------------
import cdr, cdrdb, glob, re, sys

#----------------------------------------------------------------------
# Capture processing information.  Optional argument indicates whether
# information should be displayed as well (default is no).
#----------------------------------------------------------------------
def log(msg, show = 0):
    logFile.write("%s\n" % msg)
    if show:
        sys.stderr.write("%s\n" % msg)

#----------------------------------------------------------------------
# Returns the integer corresponding to a CDR document ID.
#----------------------------------------------------------------------
def extractId(name):
    return int(re.sub('[^\d]', '', name))
    
#----------------------------------------------------------------------
# Find out who (if anyone) has a specified document checked out.
#----------------------------------------------------------------------
def findLocker(cursor, id):
    cursor.execute("""\
            SELECT u.name
              FROM usr u
              JOIN checkout c
                ON c.usr = u.id
             WHERE c.id = ?
               AND c.dt_in IS NULL
          ORDER BY dt_out DESC""", id)
    rows = cursor.fetchall()
    if rows: return rows[0][0]
    else: return None

#----------------------------------------------------------------------
# Initialize parameters for the job.
#----------------------------------------------------------------------
if len(sys.argv) != 4:
    sys.stderr.write("usage: PumpDevDocs uid pwd dev-machine\n")
    sys.stderr.write(" e.g.: PumpDevDocs melvyn lead.pudding MAHLER\n")
    sys.exit(1)
uid     = sys.argv[1]
pwd     = sys.argv[2]
server  = sys.argv[3]
conn    = cdrdb.connect('CdrGuest', dataSource = server)
cursor  = conn.cursor()
logFile = open("PumpDevDocs.log", "w")
session = cdr.login(uid, pwd, host = server)
reason  = 'preserving work on development server'

#----------------------------------------------------------------------
# Walk through the documents in the RepDocs subdirectory.  For each
# document, if the document is locked by someone other than the 
# operator, break the lock.  Then check the document out and check
# the current working version preserved from the development server
# back in.
#----------------------------------------------------------------------
for name in glob.glob("RepDocs/*.xml"):
    try:
        id = extractId(name)
        idString = "CDR%010d" % id
        locker = findLocker(cursor, id)
        if locker:
            if locker.upper() != uid.upper():
                resp = cdr.unlock(session, idString,
                        abandon = 'Y',
                        force = 'Y', 
                        reason = reason,
                        host = server)
                if resp:
                    log("Failure unlocking %s: %s" % (idString, resp), 1)
                    continue
                doc = cdr.getDoc(session, idString, 'Y', host = server)
                if doc.startswith("<Err"):
                    log("Failure locking %s: %s" % (idString, doc), 1)
                    continue
        else:
            doc = cdr.getDoc(session, idString, 'Y', host = server)
            if doc.startswith("<Err"):
                log("Failure locking %s: %s" % (idString, doc), 1)
                continue
        doc = open(name, 'rb').read()
        resp = cdr.repDoc(session, doc = doc, checkIn = 'Y', 
                reason = reason, host = server)
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
        else:
            log("Replaced %s on %s" % (resp, server))
    except:
        cdr.logout(session, host = server)
        raise

#----------------------------------------------------------------------
# For each document in the AddDocs subdirectory, add the document,
# check it out (to get the version with the document ID embedded),
# and check it back in.
#----------------------------------------------------------------------
for name in glob.glob("AddDocs/*.xml"):
    try:
        id = extractId(name)
        idString = "CDR%010d" % id
        doc = open(name, 'rb').read()
        resp = cdr.addDoc(session, doc = doc, checkIn = 'N', 
                reason = reason, host = server)
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
        doc = cdr.getDoc(session, idString, 'Y', host = server)
        if doc.startswith("<Err"):
            log("Failure locking %s: %s" % (idString, doc), 1)
            continue
        resp = cdr.repDoc(session, doc = doc, checkIn = 'Y', 
                reason = reason, host = server)
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
        else:
            log("Added %s to %s" % (resp, server))
    except:
        cdr.logout(session, host = server)
        raise

#----------------------------------------------------------------------
# Don't leave a mess behind.
#----------------------------------------------------------------------
cdr.logout(session, host = server)
