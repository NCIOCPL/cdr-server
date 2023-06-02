#!/usr/bin/env python

"""Remove GENETICSPROFESSIONAL documents from pub_proc_cg table.

For Pauling release.
"""

from argparse import ArgumentParser
from cdrapi import db

DELETE = """\
DELETE pub_proc_cg
  FROM pub_proc_cg
  JOIN document ON document.id = pub_proc_cg.id
  JOIN doc_type ON doc_type.id = document.doc_type
 WHERE doc_type.name = 'Person'
"""
parser = ArgumentParser()
parser.add_argument("--tier")
opts = parser.parse_args()
conn = db.connect(tier=opts.tier)
cursor = conn.cursor()
result = cursor.execute(DELETE)
print("removed", result.rowcount, "rows")
conn.commit()
