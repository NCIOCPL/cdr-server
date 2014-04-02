#----------------------------------------------------------------------
#
# $Id$
#
# Compare filters in Subversion against those in the CDR.
#
#----------------------------------------------------------------------
import cdrdb
import difflib
import glob
import re
import sys
import time

def compare(me, you):
    differ = difflib.Differ()
    diffs = differ.compare(me.splitlines(True),you.splitlines(True))

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

directory = len(sys.argv) > 2 and sys.argv[2] or "."
local = {}
for name in glob.glob("%s/CDR00*.xml" % directory):
    doc = open(name).read()
    match = re.search("<!-- Filter title:([^>]+)-->", doc)
    if not match:
        raise Exception("no filter title in %s\n" % repr(name))
    title = match.group(1).strip()
    local[title.lower()] = Filter(title, doc, name)
repository = {}
query = cdrdb.Query("document d", "d.id", "d.title", "d.xml")
query.join("doc_type t", "t.id = d.doc_type")
query.where(cdrdb.Query.Condition("t.name", "Filter"))
for doc_id, title, doc in query.execute().fetchall():
    repository[title.strip().lower()] = Filter(title, doc, doc_id=doc_id)
now = time.strftime("%Y%m%d%H%M%S")
fp = open("filter-diffs.%s" % now, "w")
for title in sorted(local):
    if title not in repository:
        print "Only in SVN: %s" % repr(local[title].title)
    else:
        localXml = local[title].doc
        repoXml = repository[title].doc.encode("utf-8").replace("\r", "")
        diff = compare(localXml, repoXml)
        if diff:
            fp.write("%s\n" % local[title].filename)
            print (" %s " % local[title].title).center(78, "*")
            print "< SVN DOCUMENT %s" % local[title].filename
            print "> CDR DOCUMENT CDR%010d" % repository[title].doc_id
            print diff
fp.close()
for title in sorted(repository):
    if title not in local:
        print "Only in the CDR: %s" % repr(repository[title].title)
