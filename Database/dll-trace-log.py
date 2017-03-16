#----------------------------------------------------------------------
# Keep track of what the CDR DLL does for troubleshooting.
# JIRA::OCECDR-4229
#----------------------------------------------------------------------
import cdrdb

# Connect.
conn = cdrdb.connect()
cursor = conn.cursor()
print "connected ..."

# Drop the table if it exists.
try:
    cursor.execute("DROP TABLE dll_trace_log")
    conn.commit()
except:
    pass

# Create the table.
cursor.execute("""\
CREATE TABLE dll_trace_log
     (log_id INTEGER IDENTITY PRIMARY KEY,
   log_saved DATETIME NOT NULL,
    cdr_user VARCHAR(64),
  session_id VARCHAR(64),
    log_data TEXT NOT NULL)""")
conn.commit()

# Grant read permissions
cursor.execute("GRANT SELECT on dll_trace_log TO CdrGuest")
conn.commit()
