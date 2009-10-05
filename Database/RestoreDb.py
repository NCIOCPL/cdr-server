#----------------------------------------------------------------------
#
# $Id: RestoreDb.py,v 1.1 2002-09-06 01:51:55 bkline Exp $
#
# Restore database from backup logs.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------
import cdrdb, sys

#----------------------------------------------------------------------
# Check command-line arguments.
#----------------------------------------------------------------------
if len(sys.argv) < 4:
    sys.stderr.write(
            "usage: RestoreDb db-name db-backup-file log-backup-file\n")
    sys.exit(1)
db          = sys.argv[1]
dbBackup    = sys.argv[2]
logBackup   = sys.argv[3]

#----------------------------------------------------------------------
# Connect to the database.
#----------------------------------------------------------------------
conn        = cdrdb.connect()
cursor      = conn.cursor()

#----------------------------------------------------------------------
# Determine how many data sets are in the transaction log backup.
#----------------------------------------------------------------------
cursor.execute("RESTORE HEADERONLY FROM DISK = '%s'" % logBackup)
rows        = cursor.fetchall()
if not rows: nRows = 0
else:        nRows = len(rows)
if nRows > 1:
    tail = ", NORECOVERY"
else:
    tail = ""

#----------------------------------------------------------------------
# Emit the first portion of the SQL.
#----------------------------------------------------------------------
print """\
USE master
GO
EXEC sp_dboption '%s', 'single user', 'TRUE'
GO
RESTORE DATABASE %s from DISK = '%s' WITH STATS%s
GO""" % (db, db, dbBackup, tail)
#----------------------------------------------------------------------
# Add one command for each data set in the transaction log backup.
#----------------------------------------------------------------------
for i in range(nRows):
    if i == nRows - 1:
        tail = ""
    else:
        tail = ", NORECOVERY"
    print """\
RESTORE LOG %s FROM DISK = '%s' WITH FILE = %d, STATS%s
GO""" % (db, logBackup, i + 1, tail)

#----------------------------------------------------------------------
# Switch back to multi-user mode.
#----------------------------------------------------------------------
print """\
EXEC sp_dboption '%s', 'single user', 'FALSE'
GO""" % db
