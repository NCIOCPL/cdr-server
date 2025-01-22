#!/usr/bin/env python

"""Add index to speed up the document version history report.

Consider also adding

SET TRANSACTION ISOLATION LEVEL READ UNCOMMITTED

in that report.
"""
from argparse import ArgumentParser
from cdrapi import db

parser = ArgumentParser()
parser.add_argument("--tier", "-t", default="DEV")
opts = parser.parse_args()
conn = db.connect(tier=opts.tier)
cursor = conn.cursor()
cursor.execute("""\
CREATE NONCLUSTERED INDEX pub_proc_status_completed_idx
ON pub_proc (status, completed)
INCLUDE (pub_system, output_dir, started)
""")
conn.commit()
