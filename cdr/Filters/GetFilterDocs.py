import cdr, cdrdb
session = cdr.login('rmk','***REDACTED***')
conn = cdrdb.connect('CdrGuest')
curs = conn.cursor()
curs.execute("""\
        SELECT document.id
          FROM document
          JOIN doc_type
            ON doc_type.id = document.doc_type
         WHERE doc_type.name = 'Filter'
      ORDER BY document.id""")
for row in curs.fetchall():
    docId = "CDR%010d" % row[0]
    print "fetching %s" % docId
    doc = cdr.getDoc(session, docId)
    open("%s.xml" % docId, "w").write(doc.replace("\r", ""))
