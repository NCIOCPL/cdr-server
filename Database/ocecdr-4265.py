import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
cursor.execute("""\
CREATE TABLE glossifier
         (pk INTEGER PRIMARY KEY,
   refreshed DATETIME NOT NULL,
       terms TEXT NOT NULL)""")
conn.commit()
cursor.execute("INSERT INTO glossifier VALUES(1, GETDATE(), '')")
cursor.execute("GRANT SELECT ON glossifier TO CdrGuest")
conn.commit()
