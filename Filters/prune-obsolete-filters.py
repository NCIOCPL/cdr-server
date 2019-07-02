"""
Mark unused filters in the CDR as deleted

Compares filter titles in the CDR documents with those in the file system.
Uses the current working directory unless otherwise specified.
"""

from argparse import ArgumentParser
from glob import glob
from re import search
from cdrapi import db
from cdr import delDoc

MODES = "test", "live"

parser = ArgumentParser()
parser.add_argument("--directory", "-d", default=".")
parser.add_argument("--tier", "-t")
parser.add_argument("--mode", "-m", choices=MODES)
parser.add_argument("--session", "-s", required=True)
opts = parser.parse_args()
titles = set()
for name in glob("{}/CDR00*.xml".format(opts.directory)):
    with open(name) as fp:
        doc = fp.read()
    match = search("<!-- Filter title:([^>]+)-->", doc)
    if not match:
        raise Exception("no filter title in {!r}".format(name))
    title = match.group(1).strip().lower()
    if title in titles:
        raise Exception("duplicate title {!r} in {!r}".format(title, name))
    titles.add(title)
cursor = db.connect(user="CdrGuest", tier=opts.tier).cursor()
query = db.Query("document d", "d.id", "d.title")
query.join("doc_type t", "t.id = d.doc_type")
query.where("t.name = 'Filter'")
for doc_id, title in query.execute(cursor).fetchall():
    key = title.lower().strip()
    if key not in titles:
        if opts.mode == "live":
            cdr_id = delDoc(opts.session, doc_id)
            print(u"deleted {} ({!r})".format(cdr_id, title))
        else:
            print(u"CDR{:d} ({!r}) will be deleted".format(doc_id, title))
