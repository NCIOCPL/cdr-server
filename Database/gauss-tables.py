import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()

for table in ("session_log", "api_request", "export_spec", "media_manifest"):
    try:
        cursor.execute("DROP TABLE {}".format(table))
        conn.commit()
    except:
        pass
print("older table versions dropped (if present) ...")

cursor.execute("""\
CREATE TABLE session_log
   (entry_id INTEGER IDENTITY PRIMARY KEY,
   thread_id INTEGER NOT NULL,
    recorded DATETIME NOT NULL,
     message NTEXT NOT NULL)""")
cursor.execute("""\
CREATE INDEX session_log_recorded ON session_log(recorded, entry_id)
""")
cursor.execute("GRANT SELECT ON session_log TO CdrGuest")
cursor.execute("""\
CREATE TABLE api_request
 (request_id INTEGER IDENTITY PRIMARY KEY,
  process_id INTEGER,
   thread_id INTEGER,
    received DATETIME,
     request NTEXT)""")
cursor.execute("CREATE INDEX api_request_received ON api_request(received)")
cursor.execute("GRANT SELECT ON api_request TO CdrGuest")
conn.commit()
print("logging tables created ...")

cursor.execute("""\
CREATE TABLE export_spec
     (job_id INTEGER NOT NULL,
	 spec_id INTEGER NOT NULL,
	 filters NTEXT NOT NULL,
	  subdir NVARCHAR(32) NULL,
PRIMARY KEY (job_id, spec_id))""")
cursor.execute("""
GRANT INSERT,UPDATE,SELECT,DELETE ON export_spec TO CdrPublishing""")
cursor.execute("GRANT SELECT ON export_spec TO CdrGuest")
print("export_spec table created ...")

cursor.execute("""\
CREATE TABLE media_manifest
     (job_id INTEGER NOT NULL,
	  doc_id INTEGER NOT NULL,
   blob_date DATETIME NOT NULL,
    filename NVARCHAR(32) NOT NULL,
	   title NTEXT NOT NULL,
PRIMARY KEY (job_id, doc_id))""")
cursor.execute("""\
GRANT SELECT, INSERT, UPDATE ON media_manifest TO CdrPublishing""")
cursor.execute("GRANT SELECT ON media_manifest TO CdrGuest")
print("media_manifest table created ...")
conn.commit()
