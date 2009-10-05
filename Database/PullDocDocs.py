#----------------------------------------------------------------------
#
# $Id: PullDocDocs.py,v 1.1 2002-10-24 13:32:53 bkline Exp $
#
# Pulls documentation document which need to be copied from the
# production server to the development server.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------
import cdr, cdrdb, os, re, sys, time

#----------------------------------------------------------------------
# Capture processing information.  Optional argument indicates whether
# information should be displayed as well (default is no).
#----------------------------------------------------------------------
def log(msg, show = 0):
    logFile.write("%s\n" % msg)
    if show:
        sys.stderr.write("%s\n" % msg)

#----------------------------------------------------------------------
# Object representing a CDR documentation document.
#----------------------------------------------------------------------
class Doc:
    def __init__(self, id, title, data):
        self.id     = id
        self.title  = title
        self.data   = data

#----------------------------------------------------------------------
# Load a set of CDR documentation documents from one of the servers 
# (development or production).  Returns a dictionary for the set, 
# indexed by the uppercase versions of the document titles.
#----------------------------------------------------------------------
def loadDocs(cursor, server):
    docs = {}
    cursor.execute("""\
        SELECT d.id, d.title, d.xml
          FROM document d
          JOIN doc_type t
            ON t.id = d.doc_type
         WHERE t.name LIKE 'Documentation%'""")
    for row in cursor.fetchall():
        id    = row[0]
        title = row[1]
        data  = row[2]
        key   = title.upper()
        if docs.has_key(key):
            log("duplicate found for doc '%s' on %s: %d and %d" %
                    (title, server, docs[key].id, id), 1)
            sys.exit(1)
        else:
            docs[key] = Doc(id, title, data.replace('\r', '').strip())
    return docs

#----------------------------------------------------------------------
# Create a new output directory based on the current date/time.  This
# avoids the risk of having a job clobber the results of an earlier run.
#----------------------------------------------------------------------
def makeOutputDir():
    dir = time.strftime('DocDocs-%Y%m%d%H%M%S')
    os.mkdir(dir)
    os.mkdir(dir + '/RepDocs')
    os.mkdir(dir + '/AddDocs')
    return dir

#----------------------------------------------------------------------
# Initialize parameters for the job.
#----------------------------------------------------------------------
if len(sys.argv) != 3:
    sys.stderr.write('usage: PullDevDocs prod-machine dev-machine\n')
    sys.stderr.write(' e.g.: PullDevDocs BACH MAHLER\n')
    sys.exit(1)
outputDir = makeOutputDir()
logFile   = open('%s/PullDocDocs.log' % outputDir, 'w')
prdServer = sys.argv[1]
devServer = sys.argv[2]
prdConn   = cdrdb.connect(user = 'CdrGuest', dataSource = prdServer)
devConn   = cdrdb.connect(user = 'CdrGuest', dataSource = devServer)
prdCursor = prdConn.cursor()
devCursor = devConn.cursor()
pattern   = re.compile(r"""\s+Id\s*=\s*['"]CDR\d+['"]""")

#----------------------------------------------------------------------
# Gather all documentation documents from the production and the
# development servers.  For each document which does not yet exist
# on the development machine, drop a copy of the document in a
# subdirectory for new documents to be added (after stripping out
# the document ID left over from the development machine; the added
# document will get a new ID).  For each document which exists on
# both servers but with different data, add a copy of the document
# in a subdirectory for documents to be replaced (after ensuring
# that the document ID is the one for the development server, not the
# production server).
#----------------------------------------------------------------------
log('Comparing docs between %s and %s' % (prdServer, devServer), 1)
prdDocs = loadDocs(prdCursor, prdServer)
devDocs = loadDocs(devCursor, devServer)
for key in prdDocs.keys():
    prdId = prdDocs[key].id
    if devDocs.has_key(key):

        # The document exists on both servers.  Are they identical?
        devId = devDocs[key].id
        if prdDocs[key].data == devDocs[key].data:
            log("doc %d on %s matches %d on %s" % 
                    (devId, devServer, prdId, prdServer))
        else:

            # The documents don't match.  Prepare replacement.
            log("doc %d on %s differs from %d on %s" % 
                    (devId, devServer, prdId, prdServer))
            resp = cdr.getDoc('guest', 'CDR%010d' % prdId, host = prdServer)
            if resp.find('<Err') != -1:
                log('Failure retrieving CDR%010d from %s: %s' % (
                            prdId, prdServer, resp.strip()), 1)
                sys.exit(1)
            else:
                idAttr = " Id='CDR%010d'" % devId
                doc = pattern.sub(idAttr, resp.replace('\r', ''), 1)
                name = "%s/RepDocs/%d.xml" % (outputDir, devId)
                open(name, "wb").write(doc)
            
    else:

        # The document hasn't yet been added to the production server.
        log("doc %d on %s not found on %s" % 
                (prdId, prdServer, devServer))
        resp = cdr.getDoc('guest', 'CDR%010d' % prdId, host = prdServer)
        if resp.find('<Err') != -1:
            log('Failure retrieving CDR%010d from %s: %s' % (
                        prdId, prdServer, resp), 1)
            sys.exit(1)
        else:
            doc = pattern.sub('', resp.replace('\r', ''), 1)
            name = "%s/AddDocs/%d.xml" % (outputDir, prdId)
            open(name, "wb").write(doc)

# Log documents found on the development but not the production server.
for key in devDocs.keys():
    devId = devDocs[key].id
    if not prdDocs.has_key(key):
        log("doc %d on %s not found on %s" % 
                (devId, devServer, prdServer))
