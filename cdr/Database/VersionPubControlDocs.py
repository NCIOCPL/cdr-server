#----------------------------------------------------------------------
#
# $Id: VersionPubControlDocs.py,v 1.1 2002-04-20 02:46:58 bkline Exp $
#
# Check out publishing control documents and create validated versions.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------

import cdr, cdrdb, re

#----------------------------------------------------------------------
# Build list of documents to be processed.
#----------------------------------------------------------------------
def buildDocList():
    print "building doc list"
    docList = []
    conn   = cdrdb.connect(dataSource = "CDRDEV")
    cursor = conn.cursor()
    cursor.execute("""\
            SELECT d.id
              FROM document d
              JOIN doc_type t
                ON t.id = d.doc_type
             WHERE t.name = 'PublishingSystem'""")
    for row in cursor.fetchall():
        docList.append(row[0])
    cursor.close()
    print "processing %d documents" % len(docList)
    return docList

#----------------------------------------------------------------------
# Main entry point.
#----------------------------------------------------------------------
if __name__ == "__main__":
    sess = cdr.login('rmk', '***REDACTED***')
    for docId in buildDocList():
        doc = cdr.getDoc(sess, docId, 'Y')
        print cdr.repDoc(sess, doc = doc, checkIn = 'Y', val = 'Y', ver = 'Y')
