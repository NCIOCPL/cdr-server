#----------------------------------------------------------------------
#
# $Id: base_data_sql.py,v 1.5 2002-02-08 15:01:06 bkline Exp $
#
# Generate SQL statements for loading the base CDR database records.
#
# $Log: not supported by cvs2svn $
# Revision 1.4  2001/12/21 23:15:27  bkline
# Retained primary keys for issue and dev_tasks tables.
#
# Revision 1.3  2001/12/19 20:46:52  bkline
# Added exit(0) to keep make happy.
#
# Revision 1.2  2001/12/19 20:34:30  bkline
# Specified mmdb2 as data source.
#
# Revision 1.1  2001/12/06 02:54:11  bkline
# Initial revision
#
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# Load necessary modules.
#----------------------------------------------------------------------
import binascii, cdrdb, sys

#----------------------------------------------------------------------
# Initialize maps for reconstructing foreign key references.
#----------------------------------------------------------------------
usr             = {}
format          = {}
doc_type        = {}
action          = {}
grp             = {}
link_type       = {}
link_prop_type  = {}
query_term_rule = {}
query_term_def  = {}
used_filters    = {}
used_schemas    = {}

#----------------------------------------------------------------------
# Wrap a (possibly NULL) string value suitable for use in a SQL query.
#----------------------------------------------------------------------
def quote(val):
    if val == None:
        return "NULL"
    if type(val) == type(""):
        return "'" + val.replace("'", "''") + "'"
    if type(val) == type(u""):
        return u"'" + val.replace(u"'", u"''") + u"'"
    return "'" + str(val) + "'"

#----------------------------------------------------------------------
# Generate SQL queries for populating the usr table.
#----------------------------------------------------------------------
def load_usr():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT id, name, password, created, fullname, office, email, phone,
       expired, comment
  FROM usr""")
    for row in cursor.fetchall():
        usr[row[0]] = row[1]
        print """\
INSERT INTO usr(name, password, created, fullname, office, email, phone,
                expired, comment)
     VALUES (%s, %s, GETDATE(), %s, %s, %s, %s, %s, %s)""" % (
           quote(row[1]), quote(row[2]), quote(row[4]), quote(row[5]),
           quote(row[6]), quote(row[7]), quote(row[8]), quote(row[9]))
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for populating the format table.
#----------------------------------------------------------------------
def load_format():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM format")
    for row in cursor.fetchall():
        format[row[0]] = row[1]
        print "INSERT INTO format(name, comment) VALUES (%s, %s)" % (
                quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the doc_type table.
#----------------------------------------------------------------------
def load_doc_type():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT id, name, format, versioning, active, comment, title_filter, xml_schema
  FROM doc_type""")
    for row in cursor.fetchall():
        doc_type[row[0]] = row[1]
        if not used_filters.has_key(row[6]): used_filters[row[6]] = []
        if not used_schemas.has_key(row[7]): used_schemas[row[7]] = []
        used_filters[row[6]].append(row[1])
        used_schemas[row[7]].append(row[1])
        print """\
INSERT INTO doc_type(name, format, created, versioning, active, comment)
     SELECT %s, id, GETDATE(), %s, %s, %s
       FROM format
      WHERE name = %s""" % (
           quote(row[1]), quote(row[3]), quote(row[4]), quote(row[5]),
           quote(format[row[2]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the action table.
#----------------------------------------------------------------------
def load_action():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, doctype_specific, comment FROM action")
    for row in cursor.fetchall():
        action[row[0]] = row[1]
        print """\
INSERT INTO action(name, doctype_specific, comment) VALUES (%s, %s, %s)""" % (
            quote(row[1]), quote(row[2]), quote(row[3]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the grp table.
#----------------------------------------------------------------------
def load_grp():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM grp")
    for row in cursor.fetchall():
        grp[row[0]] = row[1]
        print """\
INSERT INTO grp(name, comment) VALUES (%s, %s)""" % (
            quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the doc_status table.
#----------------------------------------------------------------------
def load_doc_status():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM doc_status")
    for row in cursor.fetchall():
        print """\
INSERT INTO doc_status(id, name, comment) VALUES (%s, %s, %s)""" % (
            quote(row[0]), quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the active_status table.
#----------------------------------------------------------------------
def load_active_status():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM active_status")
    for row in cursor.fetchall():
        print """\
INSERT INTO active_status(id, name, comment) VALUES (%s, %s, %s)""" % (
            quote(row[0]), quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the grp_action table.
#----------------------------------------------------------------------
def load_grp_action():
    cursor = conn.cursor()
    cursor.execute("SELECT grp, action, doc_type, comment FROM grp_action")
    for row in cursor.fetchall():
        print """\
INSERT INTO grp_action(grp, action, doc_type, comment)
     SELECT g.id, a.id, t.id, %s
       FROM grp g, action a, doc_type t
      WHERE g.name = %s
        AND a.name = %s
        AND t.name = %s""" % (
            quote(row[3]), quote(grp[row[0]]), quote(action[row[1]]),
            quote(doc_type[row[2]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the grp_usr table.
#----------------------------------------------------------------------
def load_grp_usr():
    cursor = conn.cursor()
    cursor.execute("SELECT grp, usr, comment FROM grp_usr")
    for row in cursor.fetchall():
        print """\
INSERT INTO grp_usr(grp, usr, comment)
     SELECT g.id, u.id, %s
       FROM grp g, usr u
      WHERE g.name = %s
        AND u.name = %s""" % (
            quote(row[2]), quote(grp[row[0]]), quote(usr[row[1]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the link_type table.
#----------------------------------------------------------------------
def load_link_type():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM link_type")
    for row in cursor.fetchall():
        link_type[row[0]] = row[1]
        print """\
INSERT INTO link_type(name, comment) VALUES (%s, %s)""" % (
            quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the link_xml table.
#----------------------------------------------------------------------
def load_link_xml():
    cursor = conn.cursor()
    cursor.execute("SELECT doc_type, element, link_id FROM link_xml")
    for row in cursor.fetchall():
        print """\
INSERT INTO link_xml(doc_type, element, link_id)
     SELECT t.id, %s, l.id
       FROM doc_type t, link_type l
      WHERE t.name = %s
        AND l.name = %s""" % (
            quote(row[1]), quote(doc_type[row[0]]), quote(link_type[row[2]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the link_target table.
#----------------------------------------------------------------------
def load_link_target():
    cursor = conn.cursor()
    cursor.execute("SELECT source_link_type, target_doc_type FROM link_target")
    for row in cursor.fetchall():
        print """\
INSERT INTO link_target(source_link_type, target_doc_type)
     SELECT l.id, t.id
       FROM link_type l, doc_type t
      WHERE l.name = %s
        AND t.name = %s""" % (
            quote(link_type[row[0]]), quote(doc_type[row[1]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the link_prop_type table.
#----------------------------------------------------------------------
def load_link_prop_type():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, comment FROM link_prop_type")
    for row in cursor.fetchall():
        link_prop_type[row[0]] = row[1]
        print """\
INSERT INTO link_prop_type(name, comment) VALUES (%s, %s)""" % (
            quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the link_properties table.
#----------------------------------------------------------------------
def load_link_properties():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT link_id, property_id, value, comment FROM link_properties""")
    for row in cursor.fetchall():
        print """\
INSERT INTO link_properties (link_id, property_id, value, comment)
     SELECT l.id, p.id, %s, %s
       FROM link_type l, link_prop_type p
      WHERE l.name = %s
        AND p.name = %s""" % (
            quote(row[2]), quote(row[3]), 
            quote(link_type[row[0]]), quote(link_prop_type[row[1]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the query_term_rule table.
#----------------------------------------------------------------------
def load_query_term_rule():
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, rule_def FROM query_term_rule")
    for row in cursor.fetchall():
        query_term_rule[row[0]] = row[1]
        print """\
INSERT INTO query_term_rule(name, rule_def) VALUES (%s, %s)""" % (
            quote(row[1]), quote(row[2]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the query_term_def table.
#----------------------------------------------------------------------
def load_query_term_def():
    cursor = conn.cursor()
    cursor.execute("SELECT path, term_rule FROM query_term_def")
    for row in cursor.fetchall():
        if row[1] == None:
            print "INSERT INTO query_term_def(path) VALUES(%s)" % (
                quote(row[0]))
        else:
            print """\
INSERT INTO query_term_def (path, term_rule)
     SELECT %s, r.id
       FROM query_term_rule r
      WHERE r.name = %s""" % (
            quote(row[0]), quote(query_term_rule[row[1]]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the dev_task_status table.
#----------------------------------------------------------------------
def load_dev_task_status():
    cursor = conn.cursor()
    cursor.execute("SELECT status FROM dev_task_status")
    for row in cursor.fetchall():
        print "INSERT INTO dev_task_status(status) VALUES (%s)" % (
            quote(row[0]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the issue_priority table.
#----------------------------------------------------------------------
def load_issue_priority():
    cursor = conn.cursor()
    cursor.execute("SELECT priority FROM issue_priority")
    for row in cursor.fetchall():
        print "INSERT INTO issue_priority(priority) VALUES (%s)" % (
            quote(row[0]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the issue_user table.
#----------------------------------------------------------------------
def load_issue_user():
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM issue_user")
    for row in cursor.fetchall():
        print "INSERT INTO issue_user(name) VALUES (%s)" % (quote(row[0]))
    print "GO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the dev_task table.
#----------------------------------------------------------------------
def load_dev_task():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT id, description, assigned_to, status, status_date, category,
       est_complete, notes
  FROM dev_task""")
    print "SET IDENTITY_INSERT dev_task ON\nGO"
    for row in cursor.fetchall():
        print """\
INSERT INTO dev_task(id, description, assigned_to, status, status_date,
                     category, est_complete, notes)
     VALUES (%s, %s, %s, %s, %s, %s, %s, %s)""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]), quote(row[6]), quote(row[7]))
    print "GO"
    print "SET IDENTITY_INSERT dev_task OFF\nGO"
    print "DBCC CHECKIDENT(dev_task, RESEED)\nGO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the issue table.
#----------------------------------------------------------------------
def load_issue():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT id, logged, logged_by, priority, description, assigned, assigned_to,
       resolved, resolved_by, notes
  FROM issue""")
    print "SET IDENTITY_INSERT issue ON\nGO"
    for row in cursor.fetchall():
        print """\
INSERT INTO issue(id, logged, logged_by, priority, description, assigned, 
                  assigned_to, resolved, resolved_by, notes)
     VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]), quote(row[6]), quote(row[7]),
            quote(row[8]), quote(row[9]))
    print "GO"
    print "SET IDENTITY_INSERT issue OFF\nGO"
    print "DBCC CHECKIDENT(issue, RESEED)\nGO"
    cursor.close()
    
#----------------------------------------------------------------------
# Generate SQL queries for populating the report_task table.
#----------------------------------------------------------------------
def load_report_task():
    cursor = conn.cursor()

    # We'll put the dev_task values back in by hand if we want them.
    cursor.execute("SELECT description, spec, sample, dev_task "
                   "FROM report_task")
    for row in cursor.fetchall():
        print """\
INSERT INTO report_task(description, spec, sample, dev_task)
     VALUES (%s, %s, %s, %s)""" % (quote(row[0]), quote(row[1]), 
                                   quote(row[2]), quote(row[3]))
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the css documents.
#----------------------------------------------------------------------
def load_css_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.val_status, d.val_date, d.title, d.xml, d.comment, d.active_status,
       b.data
  FROM document d
  JOIN doc_blob b
    ON b.id = d.id
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'css'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'css'""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]))
        print """\
INSERT INTO doc_blob(id, data)
     SELECT @@IDENTITY, 0x%s""" % binascii.b2a_hex(row[6])
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the Filter documents.
#----------------------------------------------------------------------
def load_filter_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.id, d.val_status, d.val_date, d.title, d.xml, d.comment, 
       d.active_status
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'Filter'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'Filter'""" % (
            quote(row[1]), quote(row[2]), quote(row[3]), quote(row[4]),
            quote(row[5]), quote(row[6]))
        if used_filters.has_key(row[0]):
            for doc_type in used_filters[row[0]]:
                print """\
UPDATE doc_type SET title_filter = @@IDENTITY WHERE name = %s""" % (
                quote(doc_type), )
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the PublishingSystem documents.
#----------------------------------------------------------------------
def load_publishing_system_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.val_status, d.val_date, d.title, d.xml, d.comment, d.active_status
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'PublishingSystem'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'PublishingSystem'""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]))
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the schema documents.
#----------------------------------------------------------------------
def load_schema_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.id, d.val_status, d.val_date, d.title, d.xml, d.comment, 
       d.active_status
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'schema'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'schema'""" % (
            quote(row[1]), quote(row[2]), quote(row[3]), quote(row[4]),
            quote(row[5]), quote(row[6]))
        if used_schemas.has_key(row[0]):
            for doc_type in used_schemas[row[0]]:
                print """\
UPDATE doc_type SET xml_schema = @@IDENTITY WHERE name = %s""" % (
                quote(doc_type),)
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the Documentation documents.
#----------------------------------------------------------------------
def load_documentation_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.val_status, d.val_date, d.title, d.xml, d.comment, d.active_status
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'Documentation'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'Documentation'""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]))
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Generate SQL queries for loading the MiscellaneousDocument documents.
#----------------------------------------------------------------------
def load_miscellaneous_docs():
    cursor = conn.cursor()
    cursor.execute("""\
SELECT d.val_status, d.val_date, d.title, d.xml, d.comment, d.active_status
  FROM document d
  JOIN doc_type t
    ON t.id = d.doc_type
 WHERE t.name = 'MiscellaneousDocument'""")
    for row in cursor.fetchall():
        print """\
INSERT INTO document(val_status, val_date, doc_type, title, xml, 
                     comment, active_status, last_frag_id)
     SELECT %s, %s, id, %s, %s, %s, %s, 0
       FROM doc_type
      WHERE name = 'MiscellaneousDocument'""" % (
            quote(row[0]), quote(row[1]), quote(row[2]), quote(row[3]),
            quote(row[4]), quote(row[5]))
    print "GO"
    cursor.close()

#----------------------------------------------------------------------
# Do this in the right database.
#----------------------------------------------------------------------
print "USE cdr\nGO\n"

#----------------------------------------------------------------------
# Log on to the CDR database.
#----------------------------------------------------------------------
conn = cdrdb.connect(dataSource = 'mmdb2')

#----------------------------------------------------------------------
# Load the data.
#----------------------------------------------------------------------
load_usr()
load_format()
load_doc_type()
load_action()
load_grp()
load_doc_status()
load_active_status()
load_grp_action()
load_grp_usr()
load_link_type()
load_link_xml()
load_link_target()
load_link_prop_type()
load_link_properties()
load_query_term_rule()
load_query_term_def()
load_dev_task_status()
load_issue_priority()
load_issue_user()
load_dev_task()
load_issue()
load_report_task()
load_css_docs()
load_filter_docs()
load_publishing_system_docs()
load_schema_docs()
load_documentation_docs()
load_miscellaneous_docs()
sys.exit(0)
