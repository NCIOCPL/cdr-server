#----------------------------------------------------------------------
#
# $Id: RestoreFilterSets.py,v 1.1 2003-06-10 17:38:07 bkline Exp $
#
# Drop, recreate, and populate the filter sets on the development server.
#
# $Log: not supported by cvs2svn $
#
#----------------------------------------------------------------------
import cdrdb, sys

def unFix(s):
    if not s: return None
    return s.replace("@@TAB@@", "\t").replace("@@NL@@", "\n")

filterSets       = open("FilterSets.tab").readlines()
filterSetMembers = open("FilterSetMembers.tab").readlines()
conn             = cdrdb.connect()
cursor           = conn.cursor()
try:
    cursor.execute("DROP TABLE filter_set_member")
    conn.commit()
except:
    pass
try:
    cursor.execute("DROP TABLE filter_set")
    conn.commit()
except:
    pass
cursor.execute("""\
CREATE TABLE filter_set
         (id INTEGER IDENTITY NOT NULL PRIMARY KEY,
        name VARCHAR(80)      NOT NULL UNIQUE,
 description NVARCHAR(256)    NOT NULL,
       notes NTEXT                NULL)""")
conn.commit()
cursor.execute("""\
CREATE TABLE filter_set_member
 (filter_set INTEGER NOT NULL REFERENCES filter_set,
    position INTEGER NOT NULL,
      filter INTEGER     NULL REFERENCES all_docs,
      subset INTEGER     NULL REFERENCES filter_set,
  CONSTRAINT filter_set_member_pk PRIMARY KEY(filter_set, position))""")
conn.commit()
cursor.execute("GRANT SELECT ON filter_set TO CdrGuest")
conn.commit()
cursor.execute("GRANT SELECT ON filter_set_member TO CdrGuest")
conn.commit()
cursor.execute("SET IDENTITY_INSERT filter_set ON")
conn.commit()
for filterSet in filterSets:
    fields = filterSet.split("\t")
    id     = int(fields[0])
    name   = unFix(fields[1])
    desc   = unFix(fields[2])
    notes  = unFix(fields[3])
    cursor.execute("""\
        INSERT INTO filter_set(id, name, description, notes) 
             VALUES (?, ?, ?, ?)""", (id, name, desc, notes))
    conn.commit()
cursor.execute("SET IDENTITY_INSERT filter_set OFF")
conn.commit()
filters = {}
cursor.execute("""
    SELECT d.id, d.title
      FROM document d
      JOIN doc_type t
        ON t.id = d.doc_type
     WHERE t.name = 'Filter'""")
for row in cursor.fetchall():
    key = row[1].upper().strip()
    if filters.has_key(key):
        sys.stderr.write("WARNING: two filters have key %s; "
                         "using %d instead of %d\n" % (key,
                                                       filters[key],
                                                       row[0]))
    else:
        filters[key] = row[0]
for filterSetMember in filterSetMembers:
    fields   = filterSetMember.split("\t")
    setId    = int(fields[0])
    position = int(fields[1])
    if fields[2]:
        key = unFix(fields[2]).upper().strip()
        if filters.has_key(key):
            filterId = filters[key]
            subsetId = None
        else:
            sys.stderr.write("WARNING: Filter %s not found!\n" % key)
            continue
    else:
        filterId = None
        if not fields[3]:
            sys.stderr.write("WARNING: filter and subset both null for "
                             "filter set %d position %d\n" % (setId, position))
            continue
        else:
            subsetId = int(fields[3])
    cursor.execute("""\
        INSERT INTO filter_set_member (filter_set, position, filter, subset)
             VALUES (?, ?, ?, ?)""", (setId, position, filterId, subsetId))
    conn.commit()
