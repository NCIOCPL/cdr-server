#----------------------------------------------------------------------
#
# $Id: TransferDb.py,v 1.1 2001-12-24 01:07:50 bkline Exp $
#
# Transfers newly generated cdr database from CDRDEV to MMDB2.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------

import cdrdb

db   = 'foo'
conn = cdrdb.connect(dataSource = 'CDRDEV')
curs = conn.cursor()
conn.setAutoCommit()
curs.execute("USE master")
curs.execute("EXEC sp_dboption '%s', 'single user', 'TRUE'" % db)
curs.execute("BACKUP DATABASE %s TO DISK = 'd:/cdr/Database/cdr.bak'" % db)
curs.execute("EXEC sp_dboption '%s', 'single user', 'FALSE'" % db)

conn = cdrdb.connect(dataSource = 'MMDB2')
curs = conn.cursor()
conn.setAutoCommit()
curs.execute("USE master")
curs.execute("EXEC sp_dboption '%s', 'single user', 'TRUE'" % db)
curs.execute("""\
RESTORE DATABASE %s 
FROM DISK = '\\\\CDRDEV\\cdr\\Database\\cdr.bak'""" % db)
curs.execute("EXEC sp_dboption '%s', 'single user', 'FALSE'" % db)
curs.execute("USE %s" % db)
curs.execute("EXEC sp_changedbowner 'cdr'")
