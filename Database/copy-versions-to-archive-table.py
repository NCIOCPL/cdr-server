#!/usr/bin/env python

"""
Copy new (since last run) version XML to the version archive database.

https://collaborate.nci.nih.gov/display/OCECTBWIKI/CDR+Version+XML+Archive+Database
"""

from argparse import ArgumentParser
from datetime import datetime
from time import sleep
from cdr import Logging
from cdrapi import db

logger = Logging.get_logger("version-archiver")
parser = ArgumentParser()
parser.add_argument("--batch-size", type=int, default=500)
parser.add_argument("--max-batches", type=int, default=100000)
parser.add_argument("--tier", default="PROD")
opts = parser.parse_args()
insert = """\
INSERT INTO cdr_archived_versions..doc_version_xml (id, num, xml)
SELECT TOP {:d} v.id, v.num, v.xml
  FROM all_doc_versions v
 WHERE v.xml IS NOT NULL
   AND NOT EXISTS (
    SELECT a.id, a.num
      FROM cdr_archived_versions..doc_version_xml a
     WHERE a.id = v.id
       AND a.num = v.num
)""".format(opts.batch_size)
conn = db.connect(tier=opts.tier)
cursor = conn.cursor()
batches = failures = 0
while batches < opts.max_batches:
    start = datetime.now()
    try:
        cursor.execute(insert)
        conn.commit()
    except Exception:
        logger.exception("INSERT failure")
        failures += 1
        if failures > 5:
            break
        duration = 5 * failures
        sleep(duration)
        continue
    copied = cursor.rowcount
    batches += 1
    args = copied, batches, (datetime.now() - start).total_seconds()
    logger.info("copied %d XML versions in batch %d in %f seconds", *args)
    if not copied:
        break
    sleep(1)
logger.info("done")
