#----------------------------------------------------------------------
#
# $Id$
#
# Utility for grabbing rows from the doc_version_xml table in the
# cdr_archived_versions database on the production server which are
# not already in that database on the local server (Franck or Mahler).
# Takes a single optional command-line parameter to limit the number
# of rows to process.
#
#----------------------------------------------------------------------
import cdrdb, sys

limit = len(sys.argv) > 1 and int(sys.argv[1]) or sys.maxint
prodCursor = cdrdb.connect('CdrGuest', dataSource='bach.nci.nih.gov',
                           db='cdr_archived_versions').cursor()
conn = cdrdb.connect(db='cdr_archived_versions')
cursor = conn.cursor()
prodCursor.execute("""\
    SELECT id, num
      FROM doc_version_xml
  ORDER BY id, num""")
prodRows = prodCursor.fetchall()
print "Production table has %d rows" % len(prodRows)
cursor.execute("SELECT id, num FROM doc_version_xml")
rows = cursor.fetchall()
print "Local table has %d rows" % len(rows)
localSet = set([tuple(row) for row in rows])
checked = copied = 0
willCopy = len(prodRows) - len(localSet)
if willCopy > limit:
    willCopy = limit
print "Will copy %d rows" % willCopy
lastDocId = lastDocVersion = None
for docId, docVersion in prodRows:
    if (docId, docVersion) not in localSet:
        prodCursor.execute("""\
            SELECT xml
              FROM doc_version_xml
             WHERE id = ?
               AND num = ?""", (docId, docVersion))
        docXml = prodCursor.fetchall()[0][0]
        cursor.execute("""\
            INSERT INTO cdr_archived_versions..doc_version_xml (id, num, xml)
                 VALUES (?, ?, ?)""", (docId, docVersion, docXml),
                       timeout=600)
        conn.commit()
        lastDocId = docId
        lastDocVersion = docVersion
        copied += 1
    checked += 1
    sys.stderr.write("\rchecked %d rows; copied %d rows " % (checked, copied))
    if lastDocId:
        sys.stderr.write("(last=CDR%010dV%d)   " % (lastDocId, lastDocVersion))
    if copied >= limit:
        break
