/*
 * $Id: CreateCdrLogin.sql,v 1.1 2001-12-19 20:14:31 bkline Exp $
 *
 * Run this script as database superuser to create the CDR SQL Server login.
 *
 * $Log: not supported by cvs2svn $
 */

/*
 * Have to use the master database to create databases and logins.
 */
USE master
GO

/*
 * Start fresh.  Do this here in case the cdr login is not the owner of an
 * existing cdr database.
 */
IF EXISTS (SELECT name 
             FROM master.dbo.sysdatabases 
            WHERE name = 'cdr')
	DROP DATABASE cdr
GO

/*
 * Create the cdr login if it's not already here.
 */
IF NOT EXISTS (SELECT * 
                 FROM master.dbo.syslogins 
                WHERE loginname = 'cdr')
BEGIN
    EXEC sp_addlogin 'cdr', '***REMOVED***', 'master', 'us_english'
END
GO

/*
 * Make sure we're not defaulting to the deleted database.
 */
EXEC sp_defaultdb 'cdr', 'master'

/*
 * Make sure the cdr login is a user of the master database.
 */
IF NOT EXISTS (SELECT * 
                 FROM dbo.sysusers 
                WHERE name = 'cdr' 
                  AND UID < 16382)
	EXEC sp_grantdbaccess 'cdr', 'cdr'
GO

/*
 * Let the cdr login create our database.
 */
GRANT CREATE DATABASE TO cdr
GO
