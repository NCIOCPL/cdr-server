#----------------------------------------------------------------------
#
# $Id: BackUpSmallTables.py,v 1.2 2001-12-19 19:59:20 bkline Exp $
#
# $Log: not supported by cvs2svn $
# Revision 1.1  2001/05/04 11:57:32  bkline
# Initial revision
#
#----------------------------------------------------------------------
import os, sys, time

#----------------------------------------------------------------------
# Log the processing for the job.
#----------------------------------------------------------------------
log = open("backup.log", "a")
log.write("Backup started %s\n" % time.ctime(time.time()))

#----------------------------------------------------------------------
# Extract command-line arguments.
#----------------------------------------------------------------------
if len(sys.argv) < 4:
    sys.stderr.write("usage: BackUpSmallTables.py uid pwd archive-name\n")
    log.write("invalid command line: ")
    for arg in sys.argv:
        log.write("%s " % arg)
    log.write("\n================================================\n")
    sys.exit(1)

uid = sys.argv[1]
pwd = sys.argv[2]
zip = sys.argv[3]

#----------------------------------------------------------------------
# Make sure the archive doesn't already exist.
#----------------------------------------------------------------------
if not zip.lower().endswith(".zip"):
    zip += ".zip"
if os.path.exists(zip):
    sys.stderr.write("%s already exists; exiting...\n" % zip)
    log.write("%s already exists: exiting...\n" % zip)
    log.write("================================================\n")
    sys.exit(1)

#----------------------------------------------------------------------
# List of tables to be backed up.
#----------------------------------------------------------------------
tables = ('action',
          'active_status',
          'common',
          'ctl',
          'ctl_grp',
          'dev_task',
          'dev_task_status',
          'doc_status',
          'doc_type',
          'format',
          'grp',
          'grp_action',
          'grp_usr',
          'id_category',
          'issue',
          'issue_priority',
          'issue_user',
          'link_prop_type',
          'link_properties',
          'link_target',
          'link_type',
          'link_xml',
          'query_term_def',
          'query_term_rule',
          'report_task',
          'usr'
         )

#----------------------------------------------------------------------
# Put the BCP files in their own directory.
#----------------------------------------------------------------------
try:
    os.mkdir("backup")
except:
    pass

#----------------------------------------------------------------------
# Back up each table and log the activity.
#----------------------------------------------------------------------
for table in tables:
    print "backing up %s ..." % table
    try:
        os.unlink("backup/%s.txt" % table)
    except:
        pass
    cmd = "bcp cdr.dbo.%s out backup/%s.txt -U %s -P %s -c" % (table,
                                                               table,
                                                               uid,
                                                               pwd)
    p = os.popen(cmd)
    log.write(p.read())
    if p.close():
        sys.stderr.write("failure backing up %s\n" % table)
        log.write("failure backing up %s\n" % table)
    p = os.popen("zip %s backup/%s.txt" % (zip, table))
    log.write(p.read())
    if p.close():
        sys.stderr.write("failure archiving backup/%s.txt\n" % table)
        log.write("failure archiving backup/%s.txt\n" % table)

log.write("Backup finished %s\n=========================================\n"
        % time.ctime(time.time()))
