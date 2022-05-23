#!/usr/bin/env python

from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
try:
    cursor.execute("""\
  CREATE TABLE summary_translation_job_attachment
(attachment_id INTEGER IDENTITY PRIMARY KEY,
    english_id INTEGER NOT NULL,
    file_bytes VARBINARY(MAX) NOT NULL,
     file_name NVARCHAR(256) NOT NULL,
    registered DATETIME2)""")
    print("created summary_translatioon_job_attachment table")
    cursor.execute("""\
ALTER TABLE summary_translation_job_attachment ADD FOREIGN KEY(english_id)
 REFERENCES all_docs""")
    print("foreign key constraint created")
except Exception:
    print("table already exists")
cursor.execute("""\
GRANT SELECT ON summary_translation_job_attachment TO CdrGuest""")
print("select permission granted to guest account")
conn.commit()
