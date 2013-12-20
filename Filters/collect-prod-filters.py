#----------------------------------------------------------------------
#
# $Id$
#
# Collect all of the CDR filter documents from the production server,
# saving them for use by the CGI filterdiffs.py script for comparing
# filters between PROD and the lower tiers.
#
# OCECDR-3623
#
#----------------------------------------------------------------------
import sys, cdrdb

def getFilters(directory):
    conn = cdrdb.connect('CdrGuest')
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.title, d.xml
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'Filter'""", timeout=300)
    rows = cursor.fetchall()
    for row in rows:
        title = row[0].replace(" ", "@@SPACE@@") \
                      .replace(":", "@@COLON@@") \
                      .replace("/", "@@SLASH@@") \
                      .replace("*", "@@STAR@@")
        xml = row[1].replace("\r", "")
        filename = "%s/@@MARK@@%s@@MARK@@" % (directory, title)
        open(filename, "w").write(xml.encode('utf-8'))

if len(sys.argv) != 2:
    sys.stderr.write("usage: collect-prod-filters.py target-dir\n")
else:
    getFilters(sys.argv[1])
