import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
cursor.execute("ALTER TABLE ctl ALTER COLUMN val NTEXT NOT NULL")
conn.commit()
print("ctl table successfully altered")
