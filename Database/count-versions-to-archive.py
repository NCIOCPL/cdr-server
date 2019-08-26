#!/usr/bin/env python

"""
Find out how many versions can be moved to the version archive database.

When the number gets into the 25K range, think about getting CBIIT to
top up the version archive database.

https://collaborate.nci.nih.gov/display/OCECTBWIKI/CDR+Version+XML+Archive+Database
"""

from cdrapi import db
from argparse import ArgumentParser

TABLE = "cdr_archived_versions.dbo.doc_version_xml"

parser = ArgumentParser()
parser.add_argument("--tier", default="PROD")
opts = parser.parse_args()
cursor = db.connect(user="CdrGuest", tier=opts.tier).cursor()
cursor.execute("""\
SELECT COUNT(*) n
  FROM all_doc_versions v
 WHERE v.xml IS NOT NULL
   AND NOT EXISTS (
    SELECT a.id, a.num
      FROM cdr_archived_versions..doc_version_xml a
     WHERE a.id = v.id
       AND a.num = v.num
)""")
print(f"{cursor.fetchone().n} versions can be archived.")
