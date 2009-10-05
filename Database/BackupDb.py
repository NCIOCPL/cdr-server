#----------------------------------------------------------------------
#
# $Id: BackupDb.py,v 1.1 2001-12-23 02:55:01 bkline Exp $
#
# Creates script for taking a backup of the CDR database.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------

import time

tsec = time.localtime(time.time())
tstr = "%04d%02d%02d" % (tsec[0], tsec[1], tsec[2])
name = "cdr_%s.bak" % tstr
print """\
USE master
GO

EXEC sp_dboption 'cdr', 'single user', 'TRUE'
GO

BACKUP DATABASE cdr TO DISK = 'd:/MSSQL7/BACKUP/%s'
GO

EXEC sp_dboption 'cdr', 'single user', 'FALSE'
GO

""" % name
