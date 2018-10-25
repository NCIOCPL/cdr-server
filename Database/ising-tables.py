import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()

DOCTYPES = "glossary", "media"
SUFFIXES = "job", "job_history", "state"
for doctype in DOCTYPES:
    for suffix in SUFFIXES:
        table = "{}_translation_{}".format(doctype, suffix)
        try:
            cursor.execute("DROP TABLE {}".format(table))
            conn.commit()
        except:
            pass
print("older table versions dropped (if present) ...")

cursor.execute("""\
CREATE TABLE media_translation_state
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)""")
states = (
    "Ready For Translation",
    "Translation of Labels Pending",
    "Translation Peer-Review Pending (only labels)",
    "Translation Peer-Review Complete (only labels)",
    "Spanish Labels Sent to Artist",
    "Spanish Illustration Approved",
    "Translation of Caption and Cont. Desc. Pending",
    "Translation Peer-Review Pending (Caption and Cont. Desc.)",
    "Translation Peer-Review Complete (Caption and Cont. Desc.)",
    "Translation Made Publishable"
)
position = 10
insert = "INSERT INTO media_translation_state (value_name, value_pos) "
insert += "VALUES (?, ?)"
for state in states:
    cursor.execute(insert, (state, position))
    position += 10
conn.commit()

cursor.execute("""\
CREATE TABLE media_translation_job
 (english_id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES media_translation_state,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL)""")
conn.commit()

cursor.execute("""\
CREATE TABLE media_translation_job_history
 (history_id INTEGER IDENTITY PRIMARY KEY,
  english_id INTEGER NOT NULL REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES media_translation_state,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL)""")
conn.commit()
print("media translation queue tables created ...")

cursor.execute("""\
CREATE TABLE glossary_translation_state
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)""")
states = (
    "Ready For Translation",
    "Ready for Translation Peer Review 1",
    "Ready for Translation  Peer Review 2",
    "Translation Made Publishable"
)
position = 10
insert = "INSERT INTO glossary_translation_state (value_name, value_pos) "
insert += "VALUES (?, ?)"
for state in states:
    cursor.execute(insert, (state, position))
    position += 10
conn.commit()

cursor.execute("""\
CREATE TABLE glossary_translation_job
     (doc_id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES glossary_translation_state,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL)""")
conn.commit()

cursor.execute("""\
CREATE TABLE glossary_translation_job_history
 (history_id INTEGER IDENTITY PRIMARY KEY,
      doc_id INTEGER NOT NULL REFERENCES all_docs,
    state_id INTEGER NOT NULL REFERENCES glossary_translation_state,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER NOT NULL REFERENCES usr,
    comments NTEXT NULL)""")
conn.commit()
print("glossary translation queue tables created ...")

for doctype in DOCTYPES:
    for suffix in SUFFIXES:
        table = "{}_translation_{}".format(doctype, suffix)
        cursor.execute("GRANT SELECT ON {} TO CdrGuest".format(table))
conn.commit()
print("select permissions granted ...")
print("done")
