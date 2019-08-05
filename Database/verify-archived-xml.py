#!/usr/bin/env python

"""
Ensure that XML copied to the version archive table matches the source XML.

This is a step in the process of topping up the version table in the archive
database with XML from the CDR all_doc_versions table, before we replace
the XML in the all_doc_versions table with NULLs. Usually run on PROD.
Takes less than 10 minutes.

https://collaborate.nci.nih.gov/display/OCECTBWIKI/CDR+Version+XML+Archive+Database
"""

from datetime import datetime
from sys import stderr
from cdrapi import db

def get_xml(cursor, table, version):
    cursor.execute(f"""\
SELECT xml
  FROM {table}
 WHERE id = ?
   AND num = ?""", version)
    return cursor.fetchone().xml

start = datetime.now()
cursor = db.connect(user="CdrGuest", tier="PROD").cursor()
cursor.execute("""\
SELECT v.id, v.num
  FROM all_doc_versions v
  JOIN cdr_archived_versions..doc_version_xml a
    ON a.id = v.id
   AND a.num = v.num
 WHERE v.xml IS NOT NULL
 ORDER BY v.id, v.num""")
versions = cursor.fetchall()
done = 0
for i, ver in enumerate(versions):
    id, num = ver
    primary = get_xml(cursor, "all_doc_versions", ver)
    archived = get_xml(cursor, "cdr_archived_versions..doc_version_xml", ver)
    if primary != archived:
        stderr.write(f"\nMISMATCH FOR CDR{id:d}V{num:d}\n")
        exit(1)
    stderr.write(f"\rcompared {i+1:d} of {len(versions):d} versions")
elapsed = (datetime.now() - start).total_seconds()
stderr.write("\n")
print(f"verified {len(versions):d} in {elapsed:f} seconds")
