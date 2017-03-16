#----------------------------------------------------------------------
# Utility for comparing two copies of the cdr_archived_versions table.
#----------------------------------------------------------------------
import cdrdb, sys

def getXml(cursor, docId, docVersion, tableName):
    cursor.execute("""\
        SELECT xml
          FROM %s
         WHERE id = ?
           AND num = ?""" % tableName, (docId, docVersion))
    return cursor.fetchall()[0][0]

def getIdsAndNums(cursor, tableName):
    cursor.execute("""\
        SELECT id, num
          FROM %s
      ORDER BY id, num""" % tableName)
    return cursor.fetchall()

PROD_TABLE = 'cdr_archived_versions..doc_version_xml'
localTable = len(sys.argv) > 1 and sys.argv[1] or PROD_TABLE
prodCursor = cdrdb.connect('CdrGuest', dataSource = 'bach.nci.nih.gov').cursor()
localCursor = cdrdb.connect('CdrGuest').cursor()
localVersions = getIdsAndNums(localCursor, localTable)
prodVersions = getIdsAndNums(prodCursor, PROD_TABLE)
if localVersions != prodVersions:
    sys.stderr.write("IDs and version numbers don't match\n")
    sys.exit(1)
prodVersions = None
done = 0
for docId, docVersion in localVersions:
    localXml = getXml(localCursor, docId, docVersion, localTable)
    prodXml   = getXml(prodCursor, docId, docVersion, PROD_TABLE)
    print ("CDR%010d V%05d" % (docId, docVersion)),
    done += 1
    sys.stderr.write("\rchecked %d of %d versions" % (done, len(localVersions)))
    if localXml != prodXml:
        print "DIFFERENT!"
        sys.stderr.write("\nCDR%d/%d DOES NOT MATCH\n" % (docId, docVersion))
    else:
        print "OK!"
sys.stderr.write("\nDONE\n")
