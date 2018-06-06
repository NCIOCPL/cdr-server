import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
for table in ("session_log", "api_request"):
    try:
        cursor.execute("DROP TABLE {}".format(table))
        conn.commit()
    except:
        pass
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
