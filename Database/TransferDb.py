#----------------------------------------------------------------------
#
# $Id: TransferDb.py,v 1.3 2002-08-22 17:57:38 pzhang Exp $
#
# Transfers newly generated cdr database from BACH to MOHLER.
#
# $Log: not supported by cvs2svn $
# Revision 1.2  2002/02/14 19:03:28  bkline
# Switched from test database (foo) to cdr.
#
# Revision 1.1  2001/12/24 01:07:50  bkline
# Initial revision
#
#----------------------------------------------------------------------

import cdrdb

db   = 'cdr'
conn = cdrdb.connect(dataSource = 'BACH')
curs = conn.cursor()
conn.setAutoCommit()
curs.execute("USE master")
curs.execute("EXEC sp_dboption '%s', 'single user', 'TRUE'" % db)
curs.execute("BACKUP DATABASE %s TO DISK = 'd:/cdr/Database/cdr.bak'" % db)
curs.execute("EXEC sp_dboption '%s', 'single user', 'FALSE'" % db)

conn = cdrdb.connect(dataSource = 'MOHLER')
curs = conn.cursor()
conn.setAutoCommit()
curs.execute("USE master")
curs.execute("EXEC sp_dboption '%s', 'single user', 'TRUE'" % db)
curs.execute("""\
RESTORE DATABASE %s 
FROM DISK = '\\\\BACH\\cdr\\Database\\cdr.bak'""" % db)
curs.execute("EXEC sp_dboption '%s', 'single user', 'FALSE'" % db)
curs.execute("USE %s" % db)
curs.execute("EXEC sp_changedbowner 'cdr'")
