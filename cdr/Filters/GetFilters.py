import cdrdb
conn = cdrdb.connect('CdrGuest')
curs = conn.cursor()
curs.execute("""\
        SELECT document.id, 
               document.xml
          FROM document
          JOIN doc_type
            ON doc_type.id = document.doc_type
         WHERE doc_type.name = 'Filter'
      ORDER BY document.id""")
row = curs.fetchone()
while row:
    open("CDR%010d.xml" % row[0], "w").write(row[1].replace("\r", ""))
    row = curs.fetchone()
