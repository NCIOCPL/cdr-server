import cdr, cdrdb, os
session = cdr.login('rmk','***REDACTED***')
conn = cdrdb.connect('CdrGuest')
curs = conn.cursor()
curs.execute("""\
        SELECT document.id
          FROM document
          JOIN doc_type
            ON doc_type.id = document.doc_type
         WHERE doc_type.name = 'Filter'
           AND document.title LIKE '%OBSOLETE%'
      ORDER BY document.id""")
for row in curs.fetchall():
    fileName = "CDR%010d.xml" % row[0]
    os.unlink(fileName)
    print "cvs remove %s" % fileName
    cdr.delDoc(session, "CDR%010d" % row[0], 
               reason = "Deleting obsolete filter")
