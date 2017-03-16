#----------------------------------------------------------------------
# Create and populate tables for CDR translation workflow management.
# JIRA::OCECDR-4193
#----------------------------------------------------------------------
import datetime
import cdrdb

# Timestamp for backups.
TIMESTAMP = datetime.datetime.now().strftime("%Y%m%d%H%M%S")

# Table name suffixes.
SUFFIXES = (
    "translation_job",
    "translation_job_history",
    "translation_state",
    "change_type"
)

# Valid values.
CHANGE_TYPES = (
    "New Summary",
    "Editorial Change",
    "Major Changes",
    "Comprehensive Revision",
    "Reformat"
)
STATES = (
    "Ready For Translation",
    "Translation Pending",
    "Translation Pending in Trados",
    "Translation In Progress",
    "Translation Complete",
    "Translation Peer Review 1 Pending",
    "Translation Peer Review 1 Pending in Trados",
    "Translation Peer Review 1 in Progress",
    "Translation Peer Review 1 Complete",
    "Translation Peer Review 2",
    "Translation Peer Review 2 Complete",
    "Implement Changes from Peer-Reviews",
    "Translation Made Publishable"
)

# Add a single row to a lookup value table.
def add_row(table, value, position):
    cursor.execute("""\
INSERT INTO %s (value_name, value_pos)
     VALUES (?, ?)""" % table, (value, position))

# Connect.
conn = cdrdb.connect()
cursor = conn.cursor()
print "connected ..."

# Back up existing tables just in case.
for suffix in SUFFIXES:
    table = "summary_%s" % suffix
    backup = "x_%s_%s" % (table, TIMESTAMP)
    try:
        cursor.execute("SELECT * INTO %s FROM %s" % (backup, table))
        conn.commit()
        print "backed up %s as %s ..." % (table, backup)
    except:
        pass

# Start with a clean slate.
for suffix in SUFFIXES:
    try:
        cursor.execute("DROP TABLE summary_%s" % suffix)
        conn.commit()
    except:
        pass
print "old tables dropped ..."

# Create and populate the summary change type table.
cursor.execute("""\
CREATE TABLE summary_change_type
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)""")
conn.commit()
position = 10
for value in CHANGE_TYPES:
    add_row("summary_change_type", value, position)
    position += 10
conn.commit()
print "summary change type table created and populated ..."

# Create and populate the translation state table.
cursor.execute("""\
CREATE TABLE summary_translation_state
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)""")
conn.commit()
position = 10
for value in STATES:
    add_row("summary_translation_state", value, position)
    position += 10
conn.commit()
print "translation state table created and populated ..."

# Create the table for the translation jobs.
cursor.execute("""\
CREATE TABLE summary_translation_job
 (english_id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES summary_translation_state,
  state_date DATETIME NOT NULL,
 change_type INTEGER NOT NULL REFERENCES summary_change_type,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL,
    notified DATETIME NULL)""")
conn.commit()
print "translation queue table created ..."

# Create the table for the translation jobs.
cursor.execute("""\
CREATE TABLE summary_translation_job_history
 (history_id INTEGER IDENTITY PRIMARY KEY,
  english_id INTEGER NOT NULL REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES summary_translation_state,
  state_date DATETIME NOT NULL,
 change_type INTEGER NOT NULL REFERENCES summary_change_type,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL,
    notified DATETIME NULL)""")
conn.commit()
print "translation queue history table created ..."

# Make the tables readable by CdrGuest.
for suffix in SUFFIXES:
    cursor.execute("GRANT SELECT ON summary_%s TO CdrGuest" % suffix)
conn.commit()
print "read permissions granted ..."
print "OCECDR-4193 script done"

