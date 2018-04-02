from cdrapi import db

conn = db.connect()
try:
    cursor.execute("DROP PROCEDURE cdr_set_next_job_ID")
    conn.commit()
except:
    pass
cursor.execute("""\
CREATE PROCEDURE cdr_set_next_job_ID
    @newID int
AS
    DBCC CHECKIDENT (pub_proc, RESEED, @newID)
""")
conn.commit()
