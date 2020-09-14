#!/usr/bin/env python

from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
cursor.execute("""\
DELETE FROM link_fragment WHERE doc_id IN (
    SELECT DISTINCT f.doc_id
      FROM link_fragment f
      JOIN all_docs d
        ON d.id = f.doc_id
     WHERE d.active_status = 'D')""")
print(f"deleted {cursor.rowcount} fragment rows")
cursor.execute("""\
DELETE FROM link_net WHERE target_doc IN (
    SELECT DISTINCT n.target_doc
      FROM link_net n
      JOIN all_docs d
        ON d.id = n.target_doc
     WHERE d.active_status = 'D')""")
print(f"deleted {cursor.rowcount} target rows")
cursor.execute("""\
DELETE FROM link_net WHERE source_doc IN (
    SELECT DISTINCT n.source_doc
      FROM link_net n
      JOIN all_docs d
        ON d.id = n.source_doc
     WHERE d.active_status = 'D')""")
print(f"deleted {cursor.rowcount} source rows")
conn.commit()
