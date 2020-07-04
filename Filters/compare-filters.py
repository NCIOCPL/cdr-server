#!/usr/bin/env python

"""Compare filters in Subversion against those in the CDR.
"""

import argparse
import datetime
import difflib
import glob
import re
import sys
from cdrapi import db

def compare(me, you, full=False):
    differ = difflib.Differ()
    diffs = differ.compare(me.splitlines(True),you.splitlines(True))
    if full:
        return "".join(diffs)
    changes = []
    for line in diffs:
        if line[0] != " ":
            changes.append(line)
    return "".join(changes)

class Filter:
    def __init__(self, title, doc, filename=None, doc_id=None):
        self.title = title
        self.doc = doc
        self.filename = filename
        self.doc_id = doc_id

parser = argparse.ArgumentParser()
parser.add_argument("--directory", "-d", default=".")
parser.add_argument("--full", "-f", action="store_true")
opts = parser.parse_args()
local = {}
for name in glob.glob(f"{opts.directory}/CDR00*.xml"):
    doc = open(name, encoding="utf-8").read()
    match = re.search("<!-- Filter title:([^>]+)-->", doc)
    if not match:
        raise Exception(f"no filter title in {name!r}")
    title = match.group(1).strip()
    local[title.lower()] = Filter(title, doc, name)
repository = {}
query = db.Query("document d", "d.id", "d.title", "d.xml")
query.join("doc_type t", "t.id = d.doc_type")
query.where(query.Condition("t.name", "Filter"))
for doc_id, title, doc in query.execute().fetchall():
    repository[title.strip().lower()] = Filter(title, doc, doc_id=doc_id)
now = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
with open(f"filter-diffs.{now}", "w") as fp:
    for title in sorted(local):
        if title not in repository:
            print(f"Only in Git: {local[title].title!r}")
        else:
            local_xml = local[title].doc
            repo_xml = repository[title].doc.replace("\r", "")
            diff = compare(local_xml, repo_xml)
            if diff:
                fp.write(f"{local[title].filename}\n")
                print(f"diff {local[title].filename} {local[title].title!r}")
                print(f"< GIT DOCUMENT {local[title].filename}")
                print(f"> CDR DOCUMENT CDR{repository[title].doc_id:010d}")
                if opts.full:
                    print(compare(local_xml, repo_xml, True))
                else:
                    print(diff)
for title in sorted(repository):
    if title not in local:
        args = repository[title].doc_id, repository[title].title
        print("Only in the CDR: [CDR{:010d}] {}".format(*args))
