/*
 * $Id: CreateMoreLogins.sql,v 1.3 2002-07-24 00:26:11 ameyer Exp $
 *
 * Run this script as database superuser to create the CdrGuest and
 * CdrPublishing SQL Server logins.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/12/24 01:04:41  bkline
 * Added GRANT statements.
 *
 * Revision 1.1  2001/12/19 20:14:31  bkline
 * Initial revision
 *
 */

/*
 * Have to use the master database to create databases and logins.
 */
USE master
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

/*
 * Ignore failures here (in case they're not already members); we do this
 * to avoid problems caused by restoring from a backup made on another
 * machine.
 */
EXEC sp_revokedbaccess 'CdrGuest'
GO
EXEC sp_revokedbaccess 'CdrPublishing'
GO

/*
 * Let them back in.
 */
EXEC sp_grantdbaccess 'CdrGuest'
GO
EXEC sp_grantdbaccess 'CdrPublishing'
GO

/*
 * Make cdr the default database for the cdr login.
 */
EXEC sp_defaultdb 'cdr', 'cdr'
GO

/*
 * Grant specific rights to the two new users.
 */

GRANT SELECT ON usr TO CdrGuest
GO
GRANT SELECT ON usr TO CdrPublishing
GO
GRANT SELECT ON session TO CdrGuest
GO
GRANT SELECT ON session TO CdrPublishing
GO
GRANT SELECT ON format TO CdrGuest
GO
GRANT SELECT ON format TO CdrPublishing
GO
GRANT SELECT ON doc_type TO CdrGuest
GO
GRANT SELECT ON doc_type TO CdrPublishing
GO
GRANT SELECT ON action TO CdrGuest
GO
GRANT SELECT ON action TO CdrPublishing
GO
GRANT SELECT ON grp TO CdrGuest
GO
GRANT SELECT ON grp TO CdrPublishing
GO
GRANT SELECT ON doc_status TO CdrGuest
GO
GRANT SELECT ON doc_status TO CdrPublishing
GO
GRANT SELECT ON active_status TO CdrGuest
GO
GRANT SELECT ON active_status TO CdrPublishing
GO
GRANT SELECT ON all_docs TO CdrGuest
GO
GRANT SELECT ON all_docs TO CdrPublishing
GO
GRANT SELECT ON ready_for_review TO CdrGuest
GO
GRANT SELECT ON ready_for_review TO CdrPublishing
GO
GRANT SELECT ON checkout TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON checkout TO CdrPublishing
GO
GRANT SELECT ON doc_blob TO CdrGuest
GO
GRANT SELECT ON doc_blob TO CdrPublishing
GO
GRANT SELECT ON audit_trail TO CdrGuest
GO
GRANT SELECT ON audit_trail TO CdrPublishing
GO
GRANT SELECT ON debug_log TO CdrGuest
GO
GRANT SELECT, INSERT ON debug_log TO CdrPublishing
GO
GRANT SELECT ON doc_version TO CdrGuest
GO
GRANT SELECT ON doc_version TO CdrPublishing
GO
GRANT SELECT ON version_label TO CdrGuest
GO
GRANT SELECT ON version_label TO CdrPublishing
GO
GRANT SELECT ON doc_version_label TO CdrGuest
GO
GRANT SELECT ON doc_version_label TO CdrPublishing
GO
GRANT SELECT ON grp_action TO CdrGuest
GO
GRANT SELECT ON grp_action TO CdrPublishing
GO
GRANT SELECT ON grp_usr TO CdrGuest
GO
GRANT SELECT ON grp_usr TO CdrPublishing
GO
GRANT SELECT ON link_type TO CdrGuest
GO
GRANT SELECT ON link_type TO CdrPublishing
GO
GRANT SELECT ON link_xml TO CdrGuest
GO
GRANT SELECT ON link_xml TO CdrPublishing
GO
GRANT SELECT ON link_target TO CdrGuest
GO
GRANT SELECT ON link_target TO CdrPublishing
GO
GRANT SELECT ON link_prop_type TO CdrGuest
GO
GRANT SELECT ON link_prop_type TO CdrPublishing
GO
GRANT SELECT ON link_properties TO CdrGuest
GO
GRANT SELECT ON link_properties TO CdrPublishing
GO
GRANT SELECT ON link_net TO CdrGuest
GO
GRANT SELECT ON link_net TO CdrPublishing
GO
GRANT SELECT ON link_fragment TO CdrGuest
GO
GRANT SELECT ON link_fragment TO CdrPublishing
GO
GRANT SELECT ON query_term TO CdrGuest
GO
GRANT SELECT ON query_term TO CdrPublishing
GO
GRANT SELECT ON query_term_rule TO CdrGuest
GO
GRANT SELECT ON query_term_rule TO CdrPublishing
GO
GRANT SELECT ON query_term_def TO CdrGuest
GO
GRANT SELECT ON query_term_def TO CdrPublishing
GO
GRANT SELECT ON dev_task_status TO CdrGuest
GO
GRANT SELECT ON dev_task TO CdrGuest
GO
GRANT SELECT ON issue_priority TO CdrGuest
GO
GRANT SELECT ON issue_user TO CdrGuest
GO
GRANT SELECT ON issue TO CdrGuest
GO
GRANT SELECT ON pub_proc TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON pub_proc TO CdrPublishing
GO
GRANT SELECT ON pub_proc_parm TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON pub_proc_parm TO CdrPublishing
GO
GRANT SELECT ON pub_proc_doc TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON pub_proc_doc TO CdrPublishing
GO
GRANT SELECT, UPDATE, INSERT ON batch_job TO CdrPublishing
GO
GRANT SELECT ON batch_job TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON batch_job_parm TO CdrPublishing
GO
GRANT SELECT ON batch_job_parm TO CdrGuest
GO
