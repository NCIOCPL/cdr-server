#!/usr/bin/env python

"""
Get a list of versions whose XML can be moved to the archived version DB.

Sends the Python code representation of the list of id/version tuples
for the archivable versions to the standard output device. Writes the
count of the archivable versions to the standard error device.

To just get a count of the versions, use count-versions-to-archive.py.
"""

from sys import stderr
from cdrapi import db
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument("--tier", default="PROD")
opts = parser.parse_args()
cursor = db.connect(user="CdrGuest", tier=opts.tier).cursor()
cursor.execute("""\
SELECT v.id, v.num
  FROM all_doc_versions v
 WHERE v.xml IS NOT NULL
   AND NOT EXISTS (
    SELECT a.id, a.num
      FROM cdr_archived_versions..doc_version_xml a
     WHERE a.id = v.id
       AND a.num = v.num
)""")
versions = cursor.fetchall()
print(repr(versions))
stderr.write("{:d} versions\n".format(len(versions)))
