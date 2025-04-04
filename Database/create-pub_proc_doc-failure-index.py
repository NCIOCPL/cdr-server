#!/usr/bin/env python

from argparse import ArgumentParser
from datetime import datetime
from cdrapi import db

SQL = (
    "CREATE NONCLUSTERED INDEX pub_proc_doc_failure_index "
    "ON pub_proc_doc (failure)"
)
start = datetime.now()
parser = ArgumentParser()
parser.add_argument("--tier", "-t")
opts = parser.parse_args()
try:
    conn = db.connect(tier=opts.tier)
    cursor = conn.cursor()
    cursor.execute(SQL, timeout=900)
    cursor.commit()
    elapsed = datetime.now() - start
    print("index created in", elapsed)
except Exception as e:
    elapsed = datetime.now() - start
    print("after", elapsed, e)
