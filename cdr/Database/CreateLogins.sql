/*
 * $Id: CreateLogins.sql,v 1.14 2002-11-14 01:11:14 bkline Exp $
 *
 * Run this script as database superuser to create the cdr user logins.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2002/11/08 15:38:40  pzhang
 * Rolled back to version 1.11 [left CdrGuest SELECT usr].
 *
 * Revision 1.12  2002/11/01 15:37:54  pzhang
 * Denied CdrGuest to select usr table.
 *
 * Revision 1.11  2002/11/01 05:20:39  ameyer
 * Granted delete rights on remailer_ids to CdrPublishing.  Allows process
 * to cleanup table rows used only during intermediate processing.
 *
 * Revision 1.10  2002/10/17 22:41:30  ameyer
 * Added batch_job permissions.
 *
 * Revision 1.9  2002/08/23 17:26:37  pzhang
 * Corrected mistakes.
 *
 * Revision 1.8  2002/08/23 17:23:24  pzhang
 * Added permission for PPC and PPCW
 *
 * Revision 1.7  2002/07/05 15:05:33  bkline
 * New views for primary pub jobs.
 *
 * Revision 1.6  2002/07/03 12:16:38  bkline
 * Added new views for reports.
 *
 * Revision 1.5  2002/06/07 20:06:35  bkline
 * New stored procedure to support the Person QC report.
 *
 * Revision 1.4  2002/06/04 18:50:58  ameyer
 * Granted rights on new remailer_ids table.
 *
 * Revision 1.3  2002/04/10 14:17:53  bkline
 * Granted SELECT rights to CdrGuest on failed_login_attempts view.
 *
 * Revision 1.2  2002/01/22 22:28:05  bkline
 * Added permissions for views.
 *
 * Revision 1.1  2001/12/24 01:06:11  bkline
 * Initial revision
 *
 */

/*
 * Have to use the master database to create databases and logins.
 */
USE master
GO

/*
 * Create the three logins.
 */
IF NOT EXISTS (SELECT * 
                 FROM master.dbo.syslogins 
                WHERE loginname = 'cdr')
BEGIN
    EXEC sp_addlogin 'cdr', '***REMOVED***', 'cdr', 'us_english'
END
GO

IF NOT EXISTS (SELECT * 
                 FROM master.dbo.syslogins 
                WHERE loginname = 'CdrGuest')
BEGIN
    EXEC sp_addlogin 'CdrGuest', 'readonly', 'cdr', 'us_english'
END
GO

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
 * Re-create user access to avoid problems caused by restoring from a backup
 * made on another machine.
 */
IF EXISTS (SELECT * 
             FROM dbo.sysusers 
            WHERE name = 'cdr' 
              AND UID < 16382)
	EXEC sp_revokedbaccess 'cdr'
GO

IF EXISTS (SELECT * 
             FROM dbo.sysusers 
            WHERE name = 'CdrGuest' 
              AND UID < 16382)
	EXEC sp_revokedbaccess 'CdrGuest'
GO

IF EXISTS (SELECT * 
             FROM dbo.sysusers 
            WHERE name = 'CdrPublishing' 
              AND UID < 16382)
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
 * Make cdr database owner.
 */
EXEC sp_changedbowner 'cdr'
GO

/*
 * Grant specific rights to the other two new users.
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
GRANT SELECT ON pub_proc_cg TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT, DELETE ON pub_proc_cg TO CdrPublishing
GO
GRANT SELECT ON pub_proc_cg_work TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT, DELETE ON pub_proc_cg_work TO CdrPublishing
GO
GRANT SELECT ON remailer_ids TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT, DELETE ON remailer_ids TO CdrPublishing
GO
GRANT SELECT ON active_doc TO CdrGuest
GO
GRANT SELECT ON active_doc TO CdrPublishing
GO
GRANT SELECT ON control_docs TO CdrGuest
GO
GRANT SELECT ON control_docs TO CdrPublishing
GO
GRANT SELECT ON creation_date TO CdrGuest
GO
GRANT SELECT ON creation_date TO CdrPublishing
GO
GRANT SELECT ON deleted_doc TO CdrGuest
GO
GRANT SELECT ON deleted_doc TO CdrPublishing
GO
GRANT SELECT, UPDATE, INSERT ON document TO CdrPublishing
GO
GRANT SELECT ON document TO CdrGuest
GO
GRANT SELECT ON doc_created TO CdrGuest
GO
GRANT SELECT ON doc_created TO CdrPublishing
GO
GRANT SELECT ON doc_header TO CdrGuest
GO
GRANT SELECT ON doc_header TO CdrPublishing
GO
GRANT SELECT ON last_doc_publication TO CdrGuest
GO
GRANT SELECT ON last_doc_publication TO CdrPublishing
GO
GRANT SELECT ON orphan_terms TO CdrGuest
GO
GRANT SELECT ON orphan_terms TO CdrPublishing
GO
GRANT SELECT ON pub_event TO CdrGuest
GO
GRANT SELECT ON pub_event TO CdrPublishing
GO
GRANT SELECT ON published_doc TO CdrGuest
GO
GRANT SELECT ON published_doc TO CdrPublishing
GO
GRANT SELECT ON term_children TO CdrGuest
GO
GRANT SELECT ON term_children TO CdrPublishing
GO
GRANT SELECT ON term_kids TO CdrGuest
GO
GRANT SELECT ON term_kids TO CdrPublishing
GO
GRANT SELECT ON term_parents TO CdrGuest
GO
GRANT SELECT ON term_parents TO CdrPublishing
GO
GRANT SELECT ON TermsWithParents TO CdrGuest
GO
GRANT SELECT ON TermsWithParents TO CdrPublishing
GO
GRANT SELECT ON failed_login_attempts TO CdrGuest
GO
GRANT EXECUTE ON cdr_get_count_of_links_to_persons TO CdrGuest
GO
GRANT SELECT ON doc_info TO CdrGuest
GO
GRANT SELECT ON docs_with_pub_status TO CdrGuest
GO
GRANT SELECT ON primary_pub_job TO CdrGuest
GO
GRANT SELECT ON primary_pub_doc TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON batch_job TO CdrPublishing
GO
GRANT SELECT ON batch_job TO CdrGuest
GO
GRANT SELECT, UPDATE, INSERT ON batch_job_parm TO CdrPublishing
GO
GRANT SELECT ON batch_job_parm TO CdrGuest
GO
GRANT SELECT ON filter_set TO CdrGuest
GO
GRANT SELECT ON filter_set_member TO CdrGuest
GO
