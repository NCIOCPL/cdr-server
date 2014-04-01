#----------------------------------------------------------------------
#
# $Id$
#
# Compare filters in Subversion against those in the CDR.
#
#----------------------------------------------------------------------
import cdr
import cdrdb
import glob
import re
import sys
import time

class Filter:
    def __init__(self, title, doc, filename=None):
        self.title = title
        self.doc = doc
        self.filename = filename

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
query = cdrdb.Query("document d", "d.title", "d.xml")
query.join("doc_type t", "t.id = d.doc_type")
query.where(cdrdb.Query.Condition("t.name", "Filter"))
for title, doc in query.execute().fetchall():
    repository[title.strip().lower()] = Filter(title, doc.encode("utf-8"))
now = time.strftime("%Y%m%d%H%M%S")
fp = open("filter-diffs.%s" % now, "w")
for title in sorted(local):
    if title not in repository:
        print "Only in the CDR: %s" % repr(local[title].title)
    else:
        localXml = local[title].doc
        repoXml = repository[title].doc.replace("\r", "")
        diff = cdr.diffXmlDocs(localXml, repoXml)
        if diff:
            fp.write("%s\n" % local[title].filename)
            print (" %s " % local[title].title).center(78, "*")
            print diff
fp.close()
for title in sorted(repository):
    if title not in local:
        print "Only in %s: %s" % (repr(directory),
                                  repr(repository[title].title))
