#----------------------------------------------------------------------
#
# $Id: PushDevDocs.py,v 1.9 2008-10-27 18:30:05 venglisc Exp $
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
    sys.stderr.write("usage: PushDevDocs uid pwd dev-machine\n")
    sys.stderr.write(" e.g.: PushDevDocs melvyn lead.pudding MAHLER\n")
    sys.exit(1)
uid      = sys.argv[1]
pwd      = sys.argv[2]
devGroup = 'Developers'
server   = sys.argv[3]
conn     = cdrdb.connect('CdrGuest', dataSource = server)
cursor   = conn.cursor()
logFile  = open("PushDevDocs.log", "w")
session  = cdr.login(uid, pwd, host = server)
reason   = 'preserving work on development server'

#----------------------------------------------------------------------
# Walk through the documents in the RepDocs subdirectory.  For each
# document, if the document is locked by someone other than the 
# operator, break the lock.  Then check the document out and check
# the current working version preserved from the development server
# back in.
#----------------------------------------------------------------------
log('Updating existing documents ...', 1)
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

                # Note: this can fail if someone sneaks in a lock.
                # That's extremely unlikely, and it will be logged.
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
        resp = cdr.repDoc(session, doc = doc, checkIn = 'Y', ver = 'Y', 
                          reason = reason, host = server, val = 'Y')
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
        else:
            log("Replaced %s on %s" % (resp, server))
    except:
        cdr.logout(session, host = server)
        raise

# ---------------------------------------------------------------------
# Before we can add the new documents we have to make sure that the 
# new document types exist in the doc_type table.
# We have captured the necessary information in the file 
# NewDocTypes.tab.  Use the information to add the entries in the table
# with the addDoctype() function.
# ---------------------------------------------------------------------
log('Creating new DocType entries...', 1)
def unFix(s):
    if not s: return None
    return s.replace("@@TAB", "\t").replace("@@NL@@", "\n")

newDocTypes = open("NewDocTypes.tab").readlines()

# If the file is empty we don't need to add more docTypes 
# and skip this section.
# -------------------------------------------------------------------
if newDocTypes:
    log("Adding new document types.", 1)
    # Get the group information.  This needs to be modified to allow
    # the user (presumably a member of the Developers group) to store
    # the documents for the new document types
    # ---------------------------------------------------------------
    grp = cdr.getGroup(session, devGroup)
    
    titleFilters = {}
    for docTypeInfo in newDocTypes:
        fields = docTypeInfo.split("\t")

        # We will have a problem if the name of the schema file doesn't 
        # match the name of the DocType.  We probably should extract 
        # that name along with the doc_type information in the future.
        # -------------------------------------------------------------
        dtinfo = cdr.addDoctype(session, cdr.dtinfo(
                                         type       = fields[1],
                                         format     = 'xml',
                                         versioning = fields[4],
                                         schema     = fields[1] + '.xml',
                                         comment    = unFix(fields[9])))

        # Populate the titleFilters dictionary to update the doc_type
        # table once all new documents have been added.
        # --------------------------------------------------------------
        titleFilters[fields[1]] = fields[10]

        # We encountered an error creating the document type
        # --------------------------------------------------
        if dtinfo.error: 
            log("*** Failure creating docType: %s" % fields[1], 1)
            log("*** %s" % dtinfo.error, 1)
            if dtinfo.error.startswith('Document type already exists:'):
                pass
            else:
                sys.exit(1)

        # Need to add permissions for the new docType to the grp/user 
        # running this update so we're able to add/modify/... documents
        # -------------------------------------------------------------
        for act in ('ADD DOCUMENT',     'DELETE DOCTYPE', 
                    'DELETE DOCUMENT',  'FILTER DOCUMENT', 
                    'FORCE CHECKIN',    'FORCE CHECKOUT',
                    'GET DOCTYPE',      'GET SCHEMA', 
                    'MODIFY DOCTYPE',   'MODIFY DOCUMENT', 
                    'PUBLISH DOCUMENT', 'VALIDATE DOCUMENT'):
            grp.actions[act].append(fields[1])

    # Save the permissions allowing the group to make changes to the 
    # new document types to be added.
    # --------------------------------------------------------------
    errmsg = cdr.putGroup(session, devGroup, grp)
    if errmsg:
        log("Failure saving group permissions: %s" % errmsg)
        sys.exit(1)

    #----------------------------------------------------------------------
    # Update the doc_type table to set the correct title filter for the 
    # newly added document types
    #----------------------------------------------------------------------
    log("Updating doc_type table with title filters", 1)
    conn     = cdrdb.connect('cdr', dataSource = server)
    cursor   = conn.cursor()
    for docType in titleFilters:
        cursor.execute("""\
           UPDATE doc_type SET title_filter = (SELECT id
                                                 FROM document
                                                WHERE title = '%s')
            WHERE name = '%s'""" % (titleFilters[docType], docType))
    conn.commit()
else:
    log("Document types up-to-date.", 1)


#----------------------------------------------------------------------
# We're waiting to add new documents until the docTypes have been added.
#
# For each document in the AddDocs subdirectory, add the document,
# check it out (to get the version with the document ID embedded),
# and check it back in.
#----------------------------------------------------------------------
log('Adding new documents ...', 1)
for name in glob.glob("AddDocs/*.xml"):
    try:
        id = extractId(name)
        idString = "CDR%010d" % id
        doc = open(name, 'rb').read()
        resp = cdr.addDoc(session, doc = doc, checkIn = 'N', 
                          reason = reason, host = server)
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
            continue
        doc = cdr.getDoc(session, resp, 'Y', host = server)
        if doc.startswith("<Err"):
            log("Failure locking %s: %s" % (idString, doc), 1)
            continue
        resp = cdr.repDoc(session, doc = doc, checkIn = 'Y', ver = 'Y', 
                          reason = reason, host = server, val = 'Y')
        if not resp.startswith("CDR"):
            log("Failure saving %s: %s" % (idString, resp), 1)
        else:
            log("Added old file %s as %s to %s" % (name, resp, server))
    except:
        cdr.logout(session, host = server)
        raise

#----------------------------------------------------------------------
# Don't leave a mess behind.
#----------------------------------------------------------------------
log('Done', 1)
cdr.logout(session, host = server)
