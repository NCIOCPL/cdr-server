#!/usr/bin/env python

"""One-off database update script for the fall 2019 CDR Kepler release.
"""

from cdrapi import db

conn = db.connect()
cursor = conn.cursor()

# Fix the batch_job table.
print("Converting progress column for batch jobs to Unicode")
cursor.execute("ALTER TABLE batch_job ALTER COLUMN progress NVARCHAR(MAX) NULL")
conn.commit()

# Fix the session_log table.
print("Altering session log's thread ID to be cross-platform")
cursor.execute("ALTER TABLE session_log ALTER COLUMN thread_id BIGINT NOT NULL")
conn.commit()

# Fix the query table.
print("Print converting saved queries to Unicode")
cursor.execute("ALTER TABLE query ALTER COLUMN value NVARCHAR(MAX) NULL")
conn.commit()
print("Removing duplicates from query table")
rows = cursor.execute("SELECT DISTINCT name, value FROM query").fetchall()
cursor.execute("DROP TABLE query")
cursor.execute("""\
CREATE TABLE query
  (name NVARCHAR(160) NOT NULL PRIMARY KEY,
  value NVARCHAR(MAX) NULL)""")
for row in rows:
    cursor.execute("INSERT INTO query (name, value) VALUES(?, ?)", tuple(row))
cursor.execute("GRANT ALL ON query TO CdrGuest")
conn.commit()

# Fix the ctl table.
print("Adding uniqueness constraint to ctl table")
cursor.execute("""\
ALTER TABLE ctl ADD CONSTRAINT ctl_unique UNIQUE (grp, name, inactivated)""")
conn.commit()

# Fix the external mapping tables.
print("cleaning up external mapping tables")
GTU_SELECT = "SELECT id, name FROM external_map_usage WHERE name LIKE '%gloss%'"
DELETE_RULES = "DELETE FROM external_map_rule WHERE element LIKE '{}%'"
DELETE_TYPES = "DELETE FROM external_map_type"
DELETE_MAPPINGS = "DELETE FROM external_map WHERE usage NOT IN ({})"
DELETE_USAGES = "DELETE FROM external_map_usage WHERE id NOT IN ({})"
INSERT = "INSERT INTO external_map_type (usage, doc_type) VALUES (?,?)"
DOCTYPE_SELECT = "SELECT id FROM doc_type WHERE name = 'GlossaryTermName'"
cursor.execute(DOCTYPE_SELECT)
glossary_term_name_type_id = cursor.fetchone().id
print("GlossaryTermName document type id is", glossary_term_name_type_id)
rows = cursor.execute(GTU_SELECT).fetchall()
glossary_mapping_usages = dict([tuple(row) for row in rows])
for usage in sorted(glossary_mapping_usages.values()):
    print(f"preserving {usage} mappings")
glossary_mapping_ids = list(glossary_mapping_usages)
placeholders = ",".join(["?"] * len(glossary_mapping_ids))
print(f"glossary-mapping usage IDs: {glossary_mapping_ids}")
print("deleting obsolete external mapping rules")
delete = DELETE_RULES.format("ExternalSiteContact")
cursor.execute(delete)
print(cursor.rowcount, "external mapping rules deleted")
print("deleting obsolete external mappings")
delete = DELETE_MAPPINGS.format(placeholders)
cursor.execute(delete, list(glossary_mapping_ids))
print(cursor.rowcount, "external mappings deleted")
print("deleting obsolete mapping types")
cursor.execute(DELETE_TYPES)
print(cursor.rowcount, "external mapping document types deleted")
print("adding new rows to the mapping type constraint table")
for usage_id in glossary_mapping_usages:
    cursor.execute(INSERT, (usage_id, glossary_term_name_type_id))
    print(cursor.rowcount, "row(s) added to type table for usage", usage_id)
delete = DELETE_USAGES.format(placeholders)
print("deleting obsolete external map usages")
cursor.execute(delete, list(glossary_mapping_ids))
print(cursor.rowcount, "external mapping usages deleted")
conn.commit()
print("extern mapping tables cleanup done")

# Fix the term_audio_mp3 table.
print("fixing term_audio_mp3 Unicode columns")
cursor.execute(
"ALTER TABLE term_audio_mp3 ALTER COLUMN term_name NVARCHAR(256) NOT NULL")
cursor.execute(
"ALTER TABLE term_audio_mp3 ALTER COLUMN reader_note NVARCHAR(2048) NULL")
cursor.execute(
"ALTER TABLE term_audio_mp3 ALTER COLUMN reviewer_note NVARCHAR(2048) NULL")
print("fix of term_audio_mp3 table done")
conn.commit()
print("Database modifications complete")
