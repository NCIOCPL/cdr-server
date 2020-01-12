"""Compare filters in Subversion against those in the CDR.
"""

import difflib
import glob
import re
import sys
import time
from cdrapi import db

def compare(me, you, full=False):
    differ = difflib.Differ()
    diffs = differ.compare(me.splitlines(True),you.splitlines(True))
    if full:
        return "".join(diffs)
    changes = []
    for line in diffs:
        if line[0] != ' ':
            changes.append(line)
    return "".join(changes)

class Filter:
    def __init__(self, title, doc, filename=None, doc_id=None):
        self.title = title
        self.doc = doc
        self.filename = filename
        self.doc_id = doc_id

directory = len(sys.argv) > 1 and sys.argv[1] or "."
full_diffs = len(sys.argv) > 2 and sys.argv[2] == "-f"
local = {}
for name in glob.glob("%s/CDR00*.xml" % directory):
    doc = open(name, encoding="utf-8").read()
    match = re.search("<!-- Filter title:([^>]+)-->", doc)
    if not match:
        raise Exception("no filter title in %s\n" % repr(name))
    title = match.group(1).strip()
    local[title.lower()] = Filter(title, doc, name)
repository = {}
query = db.Query("document d", "d.id", "d.title", "d.xml")
query.join("doc_type t", "t.id = d.doc_type")
query.where(query.Condition("t.name", "Filter"))
for doc_id, title, doc in query.execute().fetchall():
    repository[title.strip().lower()] = Filter(title, doc, doc_id=doc_id)
now = time.strftime("%Y%m%d%H%M%S")
fp = open("filter-diffs.%s" % now, "w")
for title in sorted(local):
    if title not in repository:
        print("Only in Git: %s" % repr(local[title].title))
    else:
        localXml = local[title].doc
        repoXml = repository[title].doc.replace("\r", "")
        diff = compare(localXml, repoXml)
        if diff:
            fp.write("%s\n" % local[title].filename)
            print("diff %s %s" % (local[title].filename,
                                  repr(local[title].title)))
            #print (" %s " % local[title].title).center(78, "*")
            print("< GIT DOCUMENT %s" % local[title].filename)
            print("> CDR DOCUMENT CDR%010d" % repository[title].doc_id)
            if full_diffs:
                print(compare(localXml, repoXml, full_diffs))
            else:
                print(diff)
fp.close()
for title in sorted(repository):
    if title not in local:
        args = repository[title].doc_id, repository[title].title
        print("Only in the CDR: [CDR%010d] %s" % args)
