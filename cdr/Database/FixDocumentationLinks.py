#----------------------------------------------------------------------
#
# $Id: FixDocumentationLinks.py,v 1.1 2002-04-20 02:46:58 bkline Exp $
#
# Adjust inter-document links to match new document IDs.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------

import cdr, cdrdb, re

#----------------------------------------------------------------------
# Squeeze out whitespace from a title.
#----------------------------------------------------------------------
def normalize(str):
    return re.sub("\s+", "", str)

#----------------------------------------------------------------------
# Build hash mapping titles to new IDs.
#----------------------------------------------------------------------
def buildTitleMap():
    print "building title map"
    titleMap = {}
    conn     = cdrdb.connect(dataSource = "CDRDEV")
    cursor   = conn.cursor()
    cursor.execute("""\
            SELECT d.title,
                   d.id
              FROM document d
              JOIN doc_type t
                ON t.id = d.doc_type
             WHERE t.name = 'Documentation'""")
    for row in cursor.fetchall():
        titleMap[normalize(row[0])] = row[1]
    cursor.close()
    return titleMap

#----------------------------------------------------------------------
# Build hash mapping old IDs to new.
#----------------------------------------------------------------------
def buildIdMap():
    print "building id map"
    titleMap = buildTitleMap()
    idMap    = {}
    conn     = cdrdb.connect(dataSource = "MMDB2")
    cursor   = conn.cursor()
    cursor.execute("""\
            SELECT d.title,
                   d.id
              FROM document d
              JOIN doc_type t
                ON t.id = d.doc_type
             WHERE t.name = 'Documentation'""")
    for row in cursor.fetchall():
        idMap[row[1]] = titleMap.get(normalize(row[0]))
    cursor.close()
    return idMap

#----------------------------------------------------------------------
# Build list of documents we need to fix.
# (No longer used; we're processing all docs so they get validated
# and versioned.)
#----------------------------------------------------------------------
def buildDocList():
    print "building doc list"
    docList = []
    conn   = cdrdb.connect(dataSource = "CDRDEV")
    cursor = conn.cursor()
    cursor.execute("""\
            SELECT d.id,
                   d.xml
              FROM document d
              JOIN doc_type t
                ON t.id = d.doc_type
             WHERE t.name = 'Documentation'""")
    row = cursor.fetchone()
    while row:
        if row[1].find("cdr:ref") != -1:
            docList.append(row[0])
        row = cursor.fetchone()
    cursor.close()
    print "processing %d documents" % len(docList)
    return docList

#----------------------------------------------------------------------
# Callback for re.sub().
#----------------------------------------------------------------------
def swapola(match):
    id   = int(match.group(2))
    id   = idMap.get(id, id)
    link = "cdr:ref = 'CDR%010d'" % id
    print "match=%s replacement=%s" % (match.group(0), link)
    return link

#----------------------------------------------------------------------
# Replace any document links with mapped equivalents.
#----------------------------------------------------------------------
def fix(doc, pattern):
    return re.sub(pattern, swapola, doc)

#----------------------------------------------------------------------
# Main entry point.
#----------------------------------------------------------------------
if __name__ == "__main__":
    pattern = re.compile(r"""cdr:ref\s*=\s*(["'])CDR(\d+)\1""")
    idMap   = buildIdMap()
    #docList = buildDocList()
    #if docList:
    session = cdr.login('rmk', '***REDACTED***')
    for docId in idMap.keys():
        print "fixing CDR%010d" % docId
        doc = fix(cdr.getDoc(session, docId, 'Y'), pattern)
        print cdr.repDoc(session, doc = doc, checkIn = 'Y', 
                         val = 'Y', ver = 'Y')
