/*
 *
 * $Id: ExportDb.sql,v 1.1 2001-12-24 01:06:11 bkline Exp $
 *
 * Transfers newly generated cdr database from CDRDEV to MMDB2.
 *
 * $Log: not supported by cvs2svn $
 */
USE master
GO
EXEC sp_dboption 'cdr', 'single user', 'TRUE'
GO
BACKUP DATABASE cdr TO DISK = 'd:/cdr/Database/cdr.bak'
GO
EXEC sp_dboption 'cdr', 'single user', 'TRUE'
GO
