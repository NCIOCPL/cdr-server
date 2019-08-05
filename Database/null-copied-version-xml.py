#!/usr/bin/env python

"""
Remove the XML from all_doc_versions for archived versions.

This is the last step in the process of topping up the version table
in the archive database with XML from the CDR all_doc_versions table.
Be sure you have run verify-archived-xml.py before running this script.
If the number of versions to be processed is large, it is probably best
to temporarily put the database in simple recovery mode, so that it
will not log the transactions. If you do that, be sure to take the CDR
database out of the AlwaysON availability group to prevent spurious
alerts. Be sure to restore it to the group and restore the recovery
mode to what it was before performing this task.

It hasn't been necessary to use simple recovery mode since May 2018,
when we had 866,772 versions to archive. If this is done at least once
a year, we'll never get near that number again.

https://collaborate.nci.nih.gov/display/OCECTBWIKI/CDR+Version+XML+Archive+Database
"""

from argparse import ArgumentParser
from datetime import datetime
from time import sleep
from cdrapi import db
from cdr import Logging

logger = Logging.get_logger("version-archiver")
parser = ArgumentParser()
parser.add_argument("--batch-size", type=int, default=100)
parser.add_argument("--max-batches", type=int, default=100000)
parser.add_argument("--tier")
opts = parser.parse_args()
try:
    conn = db.connect(tier=opts.tier)
    cursor = conn.cursor()
except:
    logger.exception("unable to connect to database")
    raise
cursor.execute("""\
SELECT MIN(v.dt) AS cutoff
  FROM CDR..all_doc_versions v
  JOIN cdr_archived_versions..doc_version_xml a
    ON a.id = v.id
   AND a.num = v.num
 WHERE v.xml IS NOT NULL""")
cutoff = str(cursor.fetchone().cutoff)[:10]
args = opts.max_batches, opts.batch_size
logger.info("cutoff is %s", cutoff)
logger.info("running %d batches of %d versions each", *args)
query = """\
UPDATE TOP ({:d}) dv
   SET xml = NULL
  FROM CDR..all_doc_versions dv
  JOIN cdr_archived_versions..doc_version_xml av
    ON av.id = dv.id
   AND av.num = dv.num
 WHERE dv.dt > '{}'
   AND dv.xml IS NOT NULL
""".format(opts.batch_size, cutoff)
batches = failures = 0
while batches < opts.max_batches:
    start = datetime.now()
    try:
        cursor.execute(query)
        conn.commit()
    except:
        logger.exception("UPDATE failure")
        failures += 1
        if failures > 5:
            break
        sleep(5 * failures)
        continue
    failures = 0
    nulled = cursor.rowcount
    batches += 1
    args = nulled, batches, (datetime.now() - start).total_seconds()
    logger.info("nulled %d xml columns in batch %d in %f seconds", *args)
    if not nulled:
        break
    sleep(.2)
logger.info("done")
