from cdrapi import db
conn = db.connect()
cursor = conn.cursor()
cursor.execute("ALTER TABLE batch_job ALTER COLUMN progress NVARCHAR(MAX) NULL")
conn.commit()
