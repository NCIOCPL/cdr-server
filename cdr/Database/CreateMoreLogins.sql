/*
 * $Id: CreateMoreLogins.sql,v 1.1 2001-12-19 20:14:31 bkline Exp $
 *
 * Run this script as database superuser to create the CdrGuest and
 * CdrPublishing SQL Server logins.
 *
 * $Log: not supported by cvs2svn $
 */

/*
 * Have to use the master database to create databases and logins.
 */
USE master
GO

/*
 * Don't need this anymore.
 */
REVOKE CREATE DATABASE FROM cdr
GO

/*
 * Create the CdrGuest login if not already present.
 */
IF NOT EXISTS (SELECT * 
                 FROM master.dbo.syslogins 
                WHERE loginname = 'CdrGuest')
BEGIN
    EXEC sp_addlogin 'CdrGuest', 'readonly', 'cdr', 'us_english'
END
GO

/*
 * Create the CdrPublishing login if not already present.
 */
IF NOT EXISTS (SELECT * 
                 FROM master.dbo.syslogins 
                WHERE loginname = 'CdrPublishing')
BEGIN
    EXEC sp_addlogin 'CdrPublishing', '***REMOVED***', 'cdr', 'us_english'
END
GO

/*
 * Add the users to the access list for the cdr database.
 */
USE cdr
GO
EXEC sp_grantdbaccess 'CdrGuest', 'CdrGuest'
GO
EXEC sp_grantdbaccess 'CdrPublishing', 'CdrPublishing'
GO

/*
 * Make cdr the default database for the cdr login.
 */
EXEC sp_defaultdb 'cdr', 'cdr'
GO
