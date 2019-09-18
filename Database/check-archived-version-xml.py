#!/usr/bin/env python

"""Compare two copies of the cdr_archived_versions table.
"""

from sys import stderr
from cdrapi import db

TABLE = "cdr_archived_versions..doc_version_xml"

def get_xml(cursor, doc_id, doc_version, tableName):
    cursor.execute("""\
        SELECT xml
          FROM %s
         WHERE id = ?
           AND num = ?""" % tableName, (doc_id, doc_version))
    return cursor.fetchone().xml

def get_ids_and_versions(cursor, tableName):
    cursor.execute("""\
        SELECT id, num
          FROM %s
      ORDER BY id, num""" % tableName)
    return cursor.fetchall()

prod_cursor = db.connect(user="CdrGuest", tier="PROD").cursor()
local_cursor = cdrdb.connect("CdrGuest").cursor()
local_versions = get_ids_and_versions(local_cursor, localTable)
prod_versions = get_ids_and_versions(prod_cursor, TABLE)
if local_versions != prod_versions:
    raise Exception("IDs and version numbers don't match")
prod_versions = None
done = 0
for doc_id, doc_version in local_versions:
    local_xml = get_xml(local_cursor, doc_id, doc_version, localTable)
    prod_xml   = get_xml(prod_cursor, doc_id, doc_version, TABLE)
    print(("CDR%010d V%05d" % (doc_id, doc_version)), end=" ")
    done += 1
    stderr.write("\rchecked %d of %d versions" % (done, len(local_versions)))
    if local_xml != prod_xml:
        print("DIFFERENT!")
        stderr.write("\nCDR%d/%d DOES NOT MATCH\n" % (doc_id, doc_version))
    else:
        print("OK!")
stderr.write("\nDONE\n")
