/*
 * $Id: tables.sql,v 1.118 2007-05-12 04:35:49 bkline Exp $
 *
 * DBMS tables for the ICIC Central Database Repository
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.117  2007/05/10 21:23:29  bkline
 * Updated comment for pub_proc_status.
 *
 * Revision 1.116  2007/05/02 21:04:04  bkline
 * Added new columns force_push and cg_new to pub_proc_cg table.
 *
 * Revision 1.115  2007/04/04 19:09:27  bkline
 * Added submitted_by column to gp_import_set table.
 *
 * Revision 1.114  2007/04/04 18:59:17  bkline
 * Added table for GENETICSPROFESSIONAL document set imports.
 *
 * Revision 1.113  2007/03/22 13:47:38  bkline
 * Added 'phase' column to ctgov_import table.
 *
 * Revision 1.112  2006/09/29 03:19:30  ameyer
 * Added chk_type column to link_type table.
 *
 * Revision 1.111  2006/09/11 13:12:00  bkline
 * Added last_status column to pub_proc_nlm table.
 *
 * Revision 1.110  2006/09/08 03:46:10  ameyer
 * Added query_term_pub table and indexes.
 *
 * Revision 1.109  2006/07/12 18:35:44  bkline
 * Added table pub_proc_nlm.
 *
 * Revision 1.108  2006/05/17 02:16:26  ameyer
 * Updated external_map table.  Added column mappable and updated comments.
 *
 * Revision 1.107  2006/04/20 20:54:11  bkline
 * Bumped up size of external_map.value column to 356 characters (from 256).
 *
 * Revision 1.106  2005/08/16 14:29:11  bkline
 * Renamed the column names for the external_map_type table.
 *
 * Revision 1.105  2005/08/15 18:16:45  bkline
 * Added external_map_type table.
 *
 * Revision 1.104  2005/06/21 13:02:12  bkline
 * Added 'bogus' column to external_map table.
 *
 * Revision 1.103  2005/03/04 22:33:08  bkline
 * Added external_map_rule table.
 *
 * Revision 1.102  2005/03/04 22:18:29  bkline
 * Changed import_doc.dropped from CHAR to DATETIME column.
 *
 * Revision 1.101  2005/01/22 16:46:11  bkline
 * Added status column to import_job table.
 *
 * Revision 1.100  2005/01/22 16:04:15  bkline
 * Added ctgov_export table.
 *
 * Revision 1.99  2004/11/02 22:42:20  ameyer
 * Changes for new blob handling.
 *
 * Revision 1.98  2004/10/07 20:43:31  bkline
 * Added generic tables to support import of documents from external sources
 * (based on the more special-purpose tables for CT.Gov imports).
 *
 * Revision 1.97  2004/08/10 15:08:55  bkline
 * Added comment for new auth_action column.
 *
 * Revision 1.96  2004/08/10 15:04:29  bkline
 * Added auth_action column to external_map_usage table.
 *
 * Revision 1.95  2004/04/30 01:04:25  ameyer
 * Added table filter_profile.
 * Revised a comment clarifying non-use of url field in link_net.
 *
 * Revision 1.94  2004/01/14 13:13:30  bkline
 * Added index on dt column of ctgov_download_stats table.
 *
 * Revision 1.93  2004/01/14 13:11:01  bkline
 * Added id column to ctgov_download_stats table.
 *
 * Revision 1.92  2004/01/13 21:45:39  bkline
 * Added `updated' column to ctgov_download_stats table.
 *
 * Revision 1.91  2004/01/13 21:41:19  bkline
 * Created table ctgov_download_stats.
 *
 * Revision 1.90  2004/01/12 21:19:20  bkline
 * Added column 'dropped' to ctgov_import.
 *
 * Revision 1.89  2003/12/06 15:54:43  bkline
 * Added identity column as new primary key for external_map.
 *
 * Revision 1.88  2003/11/10 13:42:58  bkline
 * Added ctgov_import_job and ctgov_import_event tables.
 *
 * Revision 1.87  2003/10/29 16:32:32  bkline
 * Backed out some NOT NULL restrictions on ctgov_import to accomodate
 * new requirements.
 *
 * Revision 1.86  2003/10/23 14:32:33  bkline
 * Added tables to support import of documents from ClinicalTrials.gov.
 *
 * Revision 1.85  2003/09/29 17:26:23  bkline
 * Renamed catgory column of external_map_usage table to id.
 *
 * Revision 1.84  2003/09/29 15:25:13  bkline
 * Modified external_id tables (now external_map and external_map_usage)
 * to be put into service for mapping NLM names for persons and
 * organizations when importing documents from ClinicalTrials.gov.
 *
 * Revision 1.83  2003/08/22 16:34:12  bkline
 * Fixed definition of published_doc view.
 *
 * Revision 1.82  2003/06/25 13:34:40  bkline
 * Fixed typo in modification to docs_with_pub_status (missing comma).
 *
 * Revision 1.81  2003/06/13 20:09:40  bkline
 * Added doc_title to docs_with_pub_status view.
 *
 * Revision 1.80  2003/02/10 15:37:34  pzhang
 * Added completed column back to primary_pub_job to avoid
 * possible backward compatibility problem.
 *
 * Revision 1.79  2003/02/07 21:03:13  pzhang
 * Deleted filtering removed in primary_pub_doc table because we need
 * to show removed docs in Cancer.gov job.
 *
 * Revision 1.78  2003/02/07 20:10:11  pzhang
 * Selected started instead of completed, added output_dir,
 * and filtered removed and failure in primary_pub_doc table.
 *
 * Revision 1.77  2003/01/30 23:46:38  ameyer
 * Made the name column unique.
 *
 * Revision 1.76  2003/01/28 18:46:01  ameyer
 * Added sys_value table for system wide control values.
 *
 * Revision 1.75  2002/12/19 17:15:27  pzhang
 * Added doc_status in doc_info.
 *
 * Revision 1.74  2002/11/14 20:18:08  pzhang
 * Made num in pub_proc_cg_work as required to support CG's
 * request for doc version.
 *
 * Revision 1.73  2002/11/11 17:18:36  bkline
 * Made filter_set.name UNIQUE.
 *
 * Revision 1.72  2002/11/11 16:09:08  bkline
 * Added filter_set and filter_set_member tables.
 *
 * Revision 1.71  2002/11/01 05:10:52  ameyer
 * Made remailer_ids.recipient nullable.  Not used in directories.
 *
 * Revision 1.70  2002/09/12 20:03:39  bkline
 * Added first_pub and first_pub_knowable columns to all_docs table.
 *
 * Revision 1.69  2002/08/29 12:16:45  bkline
 * Modified doc_info view.
 *
 * Revision 1.68  2002/08/23 01:10:27  ameyer
 * Added indexes to link_net.
 *
 * Revision 1.67  2002/08/12 20:30:46  bkline
 * Added command_log table.
 *
 * Revision 1.66  2002/08/07 18:03:08  pzhang
 * Added 'subdir' column to pub_proc_doc.
 *
 * Revision 1.65  2002/07/30 19:43:27  pzhang
 * Added pub_proc_cg_work table.
 *
 * Revision 1.64  2002/07/30 18:36:24  ameyer
 * Modified batch_job table slightly for wider, varchar job name.
 *
 * Revision 1.63  2002/07/19 20:43:20  bkline
 * Added pub_proc_cg table; added 'removed' column to pub_proc_doc.
 *
 * Revision 1.62  2002/07/19 00:43:32  ameyer
 * Changed status to string - more self documenting.
 *
 * Revision 1.61  2002/07/18 23:34:45  ameyer
 * Added batch_job and batch_job_parm for non-publishing batch processing.
 *
 * Revision 1.60  2002/07/05 15:05:33  bkline
 * New views for primary pub jobs.
 *
 * Revision 1.59  2002/07/03 13:06:43  bkline
 * Added primary_pub_job view; adjusted formatting.
 *
 * Revision 1.58  2002/07/03 12:16:38  bkline
 * Added new views for reports.
 *
 * Revision 1.57  2002/06/11 15:54:44  bkline
 * Added missing 'GO'.
 *
 * Revision 1.56  2002/06/08 03:17:04  ameyer
 * Added GO after remailer_ids.
 *
 * Revision 1.55  2002/06/04 18:50:31  ameyer
 * Added remailer_ids to support remailing.
 *
 * Revision 1.54  2002/06/03 12:54:10  bkline
 * Added index on document type to document table.
 *
 * Revision 1.53  2002/05/14 16:55:01  ameyer
 * Doubled column width from 32 to 64 chars in link_xml and link_net element
 * names to accomodate possible long XML element names.
 *
 * Revision 1.52  2002/04/10 13:39:40  bkline
 * Added failed_login_attempts view.
 *
 * Revision 1.51  2002/04/02 19:02:02  bkline
 * Added failure column to pub_proc_doc (and published_doc view).
 *
 * Revision 1.50  2002/02/02 01:39:51  bkline
 * Fixed typo in pub_proc table creation.
 *
 * Revision 1.49  2002/01/31 20:24:07  mruben
 * added no_output column to pub_proc
 *
 * Revision 1.48  2002/01/06 15:30:15  bkline
 * Removed statements to drop and recreate database.
 *
 * Revision 1.47  2001/12/24 00:59:36  bkline
 * Added commands to drop and re-add database.
 *
 * Revision 1.46  2001/12/23 14:55:12  bkline
 * Moved GRANT statements out to separate file.
 *
 * Revision 1.45  2001/12/22 01:38:34  bkline
 * Temporarily disabled foreign key constraint on link_net.target_doc.
 *
 * Revision 1.44  2001/12/06 03:00:36  bkline
 * Synchronized with actual database tables.
 *
 * Revision 1.43  2001/11/02 20:43:10  bkline
 * Added 'active' column to doc_type table.
 *
 * Revision 1.42  2001/10/22 11:54:33  bkline
 * Added email column to put_proc.
 *
 * Revision 1.41  2001/10/19 14:19:56  bkline
 * Added 'external' column to pub_proc table.
 *
 * Revision 1.40  2001/10/11 17:32:28  bkline
 * Added ready_for_review table.  Replaced pub_event and published_doc
 * tables with views.
 *
 * Revision 1.39  2001/09/28 17:00:43  bkline
 * Added messages column to pub_proc_doc tables; changed pub_system column
 * of pub_proc table to foreign key into all_docs table.
 *
 * Revision 1.38  2001/09/21 13:44:08  bkline
 * Changed all "REFERENCES document" to "REFERENCES all_doc".
 *
 * Revision 1.37  2001/09/20 18:08:24  ameyer
 * Changed title_filter to integer foreign key referencing all_docs(id).
 *
 * Revision 1.36  2001/09/05 17:53:16  bkline
 * Added publication tracking tables.
 *
 * Revision 1.35  2001/08/22 22:02:56  bkline
 * Added issue-tracking tables; removed approved column from all_docs.
 *
 * Revision 1.34  2001/08/07 21:09:59  ameyer
 * Added last_frag_id to all_docs.
 *
 * Revision 1.33  2001/07/28 11:58:38  bkline
 * Added node_loc column to query_term table.
 *
 * Revision 1.32  2001/07/26 23:09:07  ameyer
 * Updated description of xml_schema column to show change from schema
 * in place to schema referenced in the document table/view.
 *
 * Revision 1.31  2001/07/15 22:45:22  bkline
 * Added int_val to query_term table.
 *
 * Revision 1.30  2001/07/15 22:43:38  bkline
 * Added all_docs with document view.  Added term_kids and term_children
 * views.
 *
 * Revision 1.29  2001/06/21 17:59:28  ameyer
 * Added title_filter to doc_type.
 *
 * Revision 1.28  2001/06/05 17:47:22  ameyer
 * Added active_status table to control status values in documents.
 * Modified doc_version replacing approved and active_status with publishable.
 *
 * Revision 1.27  2001/05/15 16:05:06  ameyer
 * Fixed wrong constraints on link tables.
 *
 * Revision 1.26  2001/04/13 00:31:20  ameyer
 * Retored link_prop_type table.
 * Modified link_properties (renamed from link_prop) to use it.
 *
 * Revision 1.25  2001/04/08 19:13:13  bkline
 * Added term_parents view.
 *
 * Revision 1.24  2001/03/21 22:55:16  mruben
 * corrected comment
 *
 * Revision 1.23  2001/02/20 15:44:24  bkline
 * Added css to doc_type table; fixed default for schema_date.
 *
 * Revision 1.22  2001/02/16 01:59:07  ameyer
 * Dropped link_prop_type table
 * Changed property in link_prop to char string, and added comment
 * This streamlines access without any violation of normalization rules.
 *
 * Revision 1.21  2001/02/16 00:53:35  mruben
 * changes to save more in version control
 *
 * Revision 1.20  2001/01/02 23:30:23  ameyer
 * Changed length of link_prop[value] from 64 to 1024 max chars.
 *
 * Revision 1.19  2000/12/08 03:45:28  ameyer
 * Added active_status to document table, with new index and two new views.
 *
 * Revision 1.18  2000/11/30 23:21:39  ameyer
 * Added schema_date to doc_type table.
 *
 * Revision 1.17  2000/10/27 11:09:14  bkline
 * Added DDL for query_term, query_term_rule, and query_term_def tables.
 *
 * Revision 1.16  2000/10/17 17:55:42  mruben
 * added/modified tables for version control
 *
 * Revision 1.15  2000/09/12 22:49:11  ameyer
 * Added link_target table and revised some link related comments.
 * Eliminated validation info from link_net table.
 *
 * Revision 1.14  2000/06/09 00:01:09  ameyer
 * Added table debug_log.
 * Regularized some comment formatting for tables I defined.
 *
 * Revision 1.13  2000/05/18 15:12:33  bkline
 * Dropped url table and changed constraint on link_net.val_status from
 * "CHECK ..." to "REFERENCES doc_status".
 *
 * Revision 1.12  2000/05/11 21:09:37  ameyer
 * Eliminated creator/created, modifier/modified columns.
 * The data is already in the audit trail.
 *
 * Revision 1.11  2000/05/08 13:31:49  nanci
 * changed comments above document table and changed name to doc_attr
 *
 * Revision 1.10  2000/04/21 22:16:11  bkline
 * Added UNIQUE constraint to name columns.
 *
 * Revision 1.9  2000/04/15 22:59:59  bkline
 * Made session.name UNIQUE.
 *
 * Revision 1.8  2000/04/14 15:55:14  bkline
 * Altered some character columns to use wide characters.  Added default for
 * versioning column of doc_type ('Y').  Added 'ended' column to session
 * table.
 *
 * Revision 1.7  2000/04/13 22:09:05  bkline
 * Cleaned up DDL to make it runnable.
 *
 * Revision 1.6  2000/04/11 22:46:25  ameyer
 * Minor changes to link related tables.
 *
 * Revision 1.5  2000/02/04 01:53:11  ameyer
 * Added link module related tables.
 *
 * Revision 1.4  1999/12/16 21:53:40  bkline
 * Added 'approved' column to document table.
 *
 * Revision 1.3  1999/12/14 23:06:58  bobk
 * After meeting with Alan and Mike.
 *
 * Revision 1.2  1999/11/23  20:31:11  bobk
 * Added column-level comments.
 *
 * Revision 1.1  1999/11/23  19:20:33  bobk
 * Initial revision
 */

/*
 * Do this part by hand.  It *has* to succeed!
 * USE master
 * GO
 * DROP DATABASE cdr
 * GO
 * CREATE DATABASE cdr
 * GO
 */

USE cdr
GO

/* 
 * Common data elements (requirement 2.1.6).  All data tags beginning with
 * 'Z' must appear in this table, effectively creating a reserved namespace
 * for controlled data elements used throughout the system, to ensure that
 * the instances of the elements can be found.
 *
 * NOTE: Not currently used.
 *
 * Consider placing this functionality in a text-format control file. - AM
 *
 *          tag  XML tag used for all elements used for the functionality
 *               represented in the description column
 *  description  free text description of the functionality represented by
 *               this common data element
 *          dtd  optional specification for the DTD which must be used for
 *               this element
 */
CREATE TABLE common
        (tag VARCHAR(32)  PRIMARY KEY,
 description VARCHAR(255) NOT NULL,
         dtd NVARCHAR(255)    NULL)
GO

/* 
 * Valid categories for control values.
 *
 * NOTE: Not currently used.
 *
 *           id  primary key for the control group table
 *         name  label used to identify all control values in a given group;
 *               e.g., CNTRY
 *      comment  optional text description of the purpose of the group; e.g.
 *               ISO country code values
 */
CREATE TABLE ctl_grp
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255)    NULL)
GO

/* 
 * Named and typed values used to control various run-time settings.
 *
 * NOTE: Not currently used.
 *
 *          grp  identifies which control group this value belongs to
 *         name  mnemonic label for value; e.g., US
 *          val  value associated with this name; e.g., United States
 *      comment  optional text description of the use of this value
 */
CREATE TABLE ctl
        (grp INTEGER     NOT NULL REFERENCES ctl_grp,
        name VARCHAR(32) NOT NULL,
         val NVARCHAR(255)   NULL,
     comment VARCHAR(255)    NULL,
 PRIMARY KEY (grp, name))
GO

/* 
 * Users authorized to work with the CDR.  Note that connection information is
 * not stored in the database, but is instead tracked by the server
 * application directly at runtime.
 *
 *           id  automatically generated primary key for the usr table
 *         name  unique logon name used by this user
 *     password  string used to authenticate this user's identity
 *      created  date and time the user's account was added to the system
 *     fullname  optional string giving the user's full name
 *       office  optional identification of the office in which the user works
 *        email  optional email address; used for automated notification of
 *               system events
 *        phone  option telephone number used to contact this user
 *      expired  optional date/time this account was (or will be) deactivated
 *      comment  optional free-text description of additional characteristics
 *               for this user account
 */
CREATE TABLE usr
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
    password VARCHAR(32) NOT NULL,
     created DATETIME    NOT NULL,
    fullname VARCHAR(100)    NULL,
      office VARCHAR(100)    NULL,
       email VARCHAR(128)    NULL,
       phone VARCHAR(20)     NULL,
     expired DATETIME        NULL,
     comment VARCHAR(255)    NULL)
GO

/*
 * Sessions created on behalf of individual users.
 *
 *           id  automatically generated primary key for the table
 *         name  newly generated string which identifies the session for
 *               purposes of the external client interface.  Returned to 
 *               the client as SessionId
 *          usr  identifies the user on whose behalf the session has been
 *               created
 *    initiated  date/time the session was created
 *     last_act  date/time of the most recent activity for the session
 *      comment  optional free-text notes about the session
 */
CREATE TABLE session
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
         usr INTEGER     NOT NULL REFERENCES usr,
   initiated DATETIME    NOT NULL,
    last_act DATETIME    NOT NULL,
       ended DATETIME        NULL,
     comment VARCHAR(255)    NULL)
GO

/* 
 * Tells whether the document type is in XML, or in some other format, such
 * as Microsoft Word 95, JPEG, HTML, PNG, etc.
 *
 *           id  automatically generated primary key for the format table
 *         name  the name of the format displayed to the user; e.g., HTML
 *      comment  optional free-text description of this format
 */
CREATE TABLE format
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255)    NULL)
GO

/* 
 * Every document stored in the repository must have a type represented by a
 * row in this table.
 *
 *           id  automatically generated primary key for the doc_type table;
 *               since the id is unique throughout the repository, this 
 *               column satisfies requirement 2.3.2
 *         name  display name for this document type; e.g. PROTOCOL
 *       format  identifies whether document is in XML, or some other format,
 *               such as Microsoft Word 95, JPEG, HTML, PNG, etc.
 *      created  date/time when the document type was created
 *   versioning  flag indicating whether version control is to be applied to
 *               documents of this type (see requirement 2.4.2)
 *          dtd  XML document type definition used to validate documents of
 *               this type; even document types which are "unstructured" from
 *               the perspective of the repository system (for example, PNG
 *               ILLUSTRATION) have a DTD for the XML information carried for
 *               the document.
 *   xml_schema  CDR doc id of the document containing the XML schema 
 *               which identifies requirements of the elements of documents
 *               of this type; required even for "unstructured"
 *               document types.
 *  schema_date  Date/time when schema was last modified.
 *          css  stylesheet for use by the client for editing documents of 
 *               this type.
 * title_filter  Identifier (document id) of XSLT filter for generating
 *               a document title from this document type.
 *       active  'Y' for document types currently in use; otherwise 'N'.
 *      comment  optional free-text description of additional characteristics
 *               of documents of this type.
 *
 * NOTE: We can't declare some of the FOREIGN KEY constraints until later in
 *       the script, when we have defined the all_docs table.
 */
CREATE TABLE doc_type
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
      format INTEGER     NOT NULL REFERENCES format,
     created DATETIME    NOT NULL,
  versioning CHAR        NOT NULL DEFAULT 'Y',
  xml_schema INTEGER         NULL,
 schema_date DATETIME    NOT NULL DEFAULT GETDATE(),
title_filter INT             NULL,
      active CHAR        NOT NULL DEFAULT 'Y',
     comment VARCHAR(255)    NULL)
GO

/* 
 * Functions (a.k.a "actions") which can be performed on documents.
 *
 *               id  automatically generated primary key for the action table
 *             name  display name used to identify the action for the user;
 *                   e.g., ADD DOCUMENT
 * doctype_specific  whether permissions are assigned separately for each
 *                   document type for this action
 *          comment  optional free-text description of additional 
 *                   characteristics for this function
 */
    CREATE TABLE action
             (id INTEGER IDENTITY PRIMARY KEY,
            name VARCHAR(32)  NOT NULL UNIQUE,
doctype_specific CHAR(1)      DEFAULT 'N',
         comment VARCHAR(255) NULL)
GO

/* 
 * Groups used to assign permissions; users can belong to more than one. 
 *
 *           id  automatically generated primary key for the grp table
 *         name  display name used to identify the group to the user; for
 *               example, PDQ ADMIN
 *      comment  optional free-text description of the characteristics of
 *               this group; for example, "Responsible for adding users,
 *               document types, and validation rules; membership of this
 *               group should be restricted to two or three individuals."
 */
CREATE TABLE grp
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255)    NULL)
GO

/* 
 * Set of legal status codes for documents in the repository. 
 *
 *           id  primary key for the doc_status table; e.g., P
 *         name  display name shown to the user for this document status;
 *               e.g., PUBLISHED
 *      comment  optional free-text description of the characteristics of this
 *               document status, such as the conditions under which it should
 *               be applied, or the actions triggered by (or prevented) by the
 *               status
 */
CREATE TABLE doc_status
         (id CHAR        PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255)    NULL)
GO

/*
 * Set of legal values for the active_status column in the document table.
 *
 *           id  primary key for the active_status table; e.g., A
 *         name  display name shown to the user for this document status;
 *               e.g., "Active", "Publishable", "Deleted"
 *      comment  optional free-text description of the characteristics of this
 *               status code, such as the conditions under which it should
 *               be applied, or the actions triggered by (or prevented) by the
 *               status
 */
CREATE TABLE active_status
         (id CHAR PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255) NULL)
GO

/* 
 * Unit of storage managed by the Central Database Repository.  One row
 * contains one XML document - though that document may be a small 
 * metadata description of a larger multimedia or other blob.
 *
 * The all_docs tables contains all rows for all documents, regardless
 * of their status.  The "document" view excludes those all_docs rows
 * that have been marked as deleted (active_status of 'D').
 *
 *           id  automatically generated primary key for the document table
 *   val_status  foreign key reference into the doc_status table
 *     val_date  date validation processing was last performed on document
 *     doc_type  foreign key reference into the doc_type table
 *        title  required string containing title for document; Titles will
 *               not be required to be unique
 *          xml  for structured documents, this is the data for the document;
 *               for unstructured documents this contains tagged textual
 *               information associated with the document (for example, the
 *               standard caption for an illustration)
 *      comment  optional free-text description of additional characteristics
 *               of the document
 *active_status  'A' indicates an active document.  'D' = deleted.  Other
 *               values could be added in the future.
 * last_frag_id  if a document can contain elements with cdr:id attributes,
 *               we auto generate them in the form '_1', '_2', etc. for all
 *               elements which don't already have these attributes.  This
 *               stores the last used fragment id.  Next one used is this + 1.
 *     first_pub date document was first published (if at all); will be NULL
 *               for any document which has never yet been published; will
 *               also be NULL for any document which was imported from an
 *               external system (e.g., PDQ) for which we cannot ever know
 *               when first publication took place; these documents will
 *               have the value 'N' for the 
 * first_pub_knowable
 *               column, in which case the publication subsystem must never
 *               insert a non-NULL value into the first_pub column.
 */
      CREATE TABLE all_docs
               (id INTEGER IDENTITY PRIMARY KEY,
        val_status CHAR          NOT NULL DEFAULT 'U' REFERENCES doc_status,
          val_date DATETIME          NULL,
          doc_type INTEGER       NOT NULL REFERENCES doc_type,
             title NVARCHAR(255) NOT NULL,
               xml NTEXT         NOT NULL,
           comment NVARCHAR(255)     NULL,
     active_status CHAR          NOT NULL DEFAULT 'A' REFERENCES active_status,
      last_frag_id INTEGER       NOT NULL DEFAULT 0,
         first_pub DATETIME          NULL,
first_pub_knowable CHAR          NOT NULL DEFAULT 'Y')
GO

/*
 * Selection by document type optimization.
 */
CREATE INDEX doc_doc_type_idx ON all_docs(doc_type, active_status)
GO

/*
 * Index needed to make title searches for documents more efficient.
 */
CREATE INDEX doc_title_idx ON all_docs(title)
GO

/*
 * Index needed to support a view of the document table which brings all
 * active documents together, or all deleted documents.
 */
CREATE INDEX doc_status_idx ON all_docs(active_status, id)
GO

/*
 * Couldn't do this until now, because the all_docs table hadn't been
 * defined yet.
 */
ALTER TABLE doc_type 
        ADD CONSTRAINT fk_doc_type__xml_schema 
FOREIGN KEY (xml_schema)
 REFERENCES all_docs
GO
ALTER TABLE doc_type 
        ADD CONSTRAINT fk_doc_type__title_filter
FOREIGN KEY (title_filter)
 REFERENCES all_docs
GO

CREATE VIEW document AS SELECT * FROM all_docs WHERE active_status != 'D'
GO

/*
 * View of the document table containing only active documents.
 */
CREATE VIEW active_doc AS SELECT * FROM document WHERE active_status = 'A'
GO

/*
 * View of the document table containing only deleted documents.
 */
CREATE VIEW deleted_doc AS SELECT * FROM document WHERE active_status = 'D'
GO

/*
 * Flag for documents which have been marked as ready for pre-publication
 * review.
 *
 *       doc_id  foreign key into all_docs table.
 */
CREATE TABLE ready_for_review
     (doc_id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs)
GO

/* 
 * Record of a document's having been checked out.  Retained even after it has
 * been checked back in.  Note that queued requests to check a document out
 * are not represented in the database.  These are instead tracked directly by
 * the CDR process at runtime.  We may reconsider this decision if the
 * multi-threaded, single-process architecture is replaced with multiple
 * server processes (or, more likely, we may switch to using shared memory).
 *
 *           id  automatically generated primary key for the checkout table
 *       dt_out  date/time document was checked out
 *          usr  identifies the user account to which the document has been
 *               checked out
 *        dt_in  date/time document was checked back in
 *      version  version number checked in.  Null if abandoned
 *      comment  optional free-text notes about the checked-out status of the
 *               document, or the purpose for which the document was checked
 *               out
 */
CREATE TABLE checkout
         (id INTEGER  NOT NULL REFERENCES all_docs,
      dt_out DATETIME NOT NULL,
         usr INTEGER  NOT NULL REFERENCES usr,
       dt_in DATETIME     NULL,
     version INTEGER      NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (id, dt_out))
GO

/* 
 * Non-XML data for document.
 *
 * Blobs and documents are partly independent.  A new blob, with a new
 * id, may be associated with an existing document.  The new blob_id
 * replaces the previous blob_id.  The old blob will still be associated
 * with older versions of the XML document.  If there were no stored
 * versions, then the new blob replaces the old one and the blob_id
 * stays the same.
 *
 * However, a blob may only be associated with one current working
 * document - its XML metada.  And a current working document may 
 * only be associated with one blob.  Other documents may reference
 * a blob by linking to its associated XML metadata.
 *
 *           id  unique id of this blob, referenced by
 *                doc_blob_usage.blob_id and version_blob_usage.blob_id.
 *         data  binary image of the document's unstructured data
 */
CREATE TABLE doc_blob
         (id INTEGER IDENTITY PRIMARY KEY,
        data IMAGE   NOT NULL)
GO

/*
 * Associates a blob with the document that describes it.
 *
 * A blob can only be associated with one document, though it may be
 * referenced by many.  A reference from a document to a blob consists
 * of a linking element inside the document, linking to the doc_id in
 * this table for the blob.
 *
 *      doc_id  id of the metadata document for this blob.
 *     blob_id  id of the blob in the doc_blob table.
 */
CREATE TABLE doc_blob_usage
       (doc_id INTEGER UNIQUE REFERENCES all_docs,
       blob_id INTEGER UNIQUE REFERENCES doc_blob)
GO
/*
 * Indexes for access by either identifier.
 * Table may be small enough that this isn't helpful, but just in
 * case it grows.
 */
CREATE INDEX doc_blob_doc_idx ON doc_blob_usage(doc_id)
CREATE INDEX doc_blob_blob_idx ON doc_blob_usage(blob_id)
GO

/*
 * Associates a blob with a version of a document that describes it.
 * When a document associated with a blob (i.e., a blob metadata document)
 * is versioned, we create an entry in this table indicating that the
 * version references this particular blob.
 * If the blob changes, a new doc_blob will be created and the doc_blob_usage
 * table will refer to the new blob.  But the version_blob_usage entry
 * will remain unchanged.  The old version still references the old blob.
 *
 *      doc_id  id of the metadata document for this blob.
 * doc_version  version number.
 *     blob_id  id of the blob in the doc_blob table.
 */
CREATE TABLE version_blob_usage
       (doc_id INTEGER REFERENCES all_docs,
   doc_version INTEGER,
       blob_id INTEGER REFERENCES doc_blob)
GO
/*
 * Indexes for access.
 */
CREATE INDEX ver_blob_doc_idx ON version_blob_usage(doc_id, doc_version)
CREATE INDEX ver_blob_blob_idx ON version_blob_usage(blob_id)
GO

/* 
 * Names document attributes associated with a particular document. 
 * Requirement 2.3.1.  This can store any values which need to be used for
 * filtering selections without the need for searching through the full
 * documents.  Could also be used to help support ORDER BY extractions.  Note
 * that this table supports multiple occurrences of attributes of a given
 * type for each document.
 *
 * Either use this table, or a parallel table which carries the values of
 * records used for validating referential integrity (for example, thesaurus
 * records).
 *
 * NOTE: Not currently used.
 *
 *           id  identification of the document to which this attribute
 *               pertains
 *         name  name of the type of this document attribute; for example,
 *               'AUDIENCE'
 *          num  occurrence number for this instance of this attribute type
 *               for this document; convention starts numbering with 1
 *          val  value for this attribute instance; e.g., 'Technical'
 *      comment  optional free-text notes concerning this attribute value
 */
CREATE TABLE doc_attr
         (id INTEGER NOT NULL REFERENCES all_docs,
        name VARCHAR(32) NOT NULL,
         num INTEGER NOT NULL,
         val NVARCHAR(255) NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (id, name, num))
GO

/* 
 * Names of sets of external values which correspond to CDR documents.
 *
 *           id  automatically generated primary key for the table
 *         name  display name for the category of identifier; e.g.
 *               'CTGov Sponsor'
 *  auth_action  used to control authorization for editing rows in
 *               the external_map table; only a user who is a member
 *               of at least one group with the permission identified
 *               by this column for the usage of a row in the external_map
 *               table are allowed to modify that row; this is enforced
 *               by the CGI script, not within the CDR server
 *      comment  optional free-text explanation of the usage/characteristics
 *               of this category of external identifier
 */
CREATE TABLE external_map_usage
         (id INTEGER      IDENTITY PRIMARY KEY,
        name VARCHAR(32)  NOT NULL UNIQUE,
 auth_action INTEGER      NOT NULL REFERENCES action,
     comment NVARCHAR(255)    NULL)
GO

/* 
 * Map of values used by external systems to identify entities (such as
 * persons or organizations) which correspond to CDR documents.  This
 * table provides virtual lookup tables for mapping (for example) the
 * strings used by ClinicalTrials.gov at NLM to identify agencies or
 * sponsors.
 *
 *           id  primary key for this table
 *        usage  category of this external identifier
 *        value  value of the external identifier
 *       doc_id  identifies the document which this external value matches
 *          usr  identifies user who created or modified this entry
 *     last_mod  date/time the entry was created or modified
 *        bogus  'Y' = a known invalid value found in input data
 *     mappable  'N' = value is not an actual field value, often it's
 *                a comment explaining why no value is available.
 */
CREATE TABLE external_map
         (id INTEGER       IDENTITY PRIMARY KEY,
       usage INTEGER       NOT NULL REFERENCES external_map_usage,
       value NVARCHAR(356) NOT NULL,
      doc_id INTEGER           NULL REFERENCES all_docs,
         usr INTEGER           NULL REFERENCES usr,
    last_mod DATETIME      NOT NULL,
       bogus CHAR          NOT NULL DEFAULT 'N',
    mappable CHAR          NOT NULL DEFUALT 'Y',
CONSTRAINT external_map_unique UNIQUE (usage, value))
GO

/*
 * Custom processing rules for mapping of external values.
 *
 *           id  primary key for this table (auto-generated)
 *       map_id  foreign key into the external_map table
 *      element  element to be populated with custom value
 *        value  value to be placed in element
 */
CREATE TABLE external_map_rule
         (id INTEGER       IDENTITY PRIMARY KEY,
      map_id INTEGER       NOT NULL REFERENCES external_map,
     element VARCHAR(64)   NOT NULL,
       value NVARCHAR(255) NOT NULL)
GO

/* 
 * Actions performed on documents which modified the data.  Requirement 4.1.3.
 *
 *     document  identifies the document on which this action was performed
 *           dt  date/time the modified version of the document was stored 
 *               in the repository
 *          usr  user account responsible for the changes
 *       action  identification of the action which was performed
 *      program  optional identification of the program used to perform the 
 *               action
 *      comment  optional free-form text explanation of the changes
 */
CREATE TABLE audit_trail
   (document INTEGER      NOT NULL REFERENCES all_docs,
          dt DATETIME     NOT NULL,
         usr INTEGER      NOT NULL REFERENCES usr,
      action INTEGER      NOT NULL REFERENCES action,
     program VARCHAR(32)      NULL,
     comment VARCHAR(255)     NULL,
 PRIMARY KEY (document, dt))
GO

/*
 * Information logged for debugging.
 *
 *         id  Sequential id shows sequence of log writes.
 *     thread  Separate id for each thread, allows us to find messages
 *             generated by the same thread in the general stream of messages.
 *   recorded  Date time the message is logged.
 *     source  String passed by function requesting logging - should identify
 *             the function, code fragment, or other useful debugging info.
 *        msg  Logged message.
 *
 * Maximum SQL Server row size of 8000 allows up to 3945 wide chars for msg.
 * We're using 3800 in case we have to add something else later.
 */
CREATE TABLE debug_log
         (id INTEGER        IDENTITY PRIMARY KEY,
      thread INTEGER        NOT NULL,
    recorded DATETIME       NOT NULL,
      source NVARCHAR(64)   NOT NULL,
         msg NVARCHAR(3800) NOT NULL)
GO
CREATE INDEX debug_log_recorded_idx ON debug_log(recorded)
GO

/* 
 * Version control.  XXX - will we use version control for documents whose
 * data is stored externally (e.g., graphics)?  Also, should we add a column
 * for older versions which have been archived from the on-line portion of the
 * system, providing location information for the version's data?
 * Alan has suggested that we may only want to store version information for
 * versions which have been "published" - assuming we can come up with a
 * satisfactory definition for that term in this context.
 *
 *     document  identifies the document which this version represents
 *          num  sequential number of the version of the document; numbering
 *               starts with 1
 *           dt  date/time the document was checked in
 *          usr  identifies the user account that checked in the document
 *   val_status  copied from document table
 *     val_date  copied from document table
 *   updated_dt  date/time the document last updated
 *     doc_type  foreign key reference into the doc_type table
 *  publishable  'Y'=this version can be published, 'N'=can't publish
 *        title  required string containing title for document; Titles will
 *               not be required to be unique
 *          xml  for structured documents, this is the data for the document;
 *               for unstructured documents this contains tagged textual
 *               information associated with the document (for example, the
 *               standard caption for an illustration)
 *      comment  optional free-text description of additional characteristics
 *               of the document
 */
CREATE TABLE doc_version
             (id INTEGER      NOT NULL REFERENCES all_docs,
             num INTEGER      NOT NULL,
              dt DATETIME     NOT NULL,
      updated_dt DATETIME     NOT NULL,
             usr INTEGER      NOT NULL REFERENCES usr,
      val_status CHAR         NOT NULL REFERENCES doc_status,
        val_date DATETIME         NULL,
     publishable CHAR         NOT NULL,
        doc_type INTEGER      NOT NULL REFERENCES doc_type,
           title VARCHAR(255) NOT NULL,
             xml NTEXT        NOT NULL,
         comment VARCHAR(255)     NULL,
     PRIMARY KEY (id, num))
GO

/*
 * Marks a version for later retrieval by name.  Note that a single version of
 * a given document can be marked with more than one label (or with none at
 * all).
 *
 *           id  automatically generated primary key for the version_label
 *               table
 *        label  identification of a logical version; used to mark the common 
 *               version for a collection of documents in order to be able to
 *               retrieve that version of those documents at a later point in
 *               time, without knowing the individual version number for each
 *               document
 *      comment  optional free-text description of the purpose of this version
 *               label
 */
CREATE TABLE version_label
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255)    NULL)
GO

/*
 * Associates a version label with a specific version of a single document.
 *
 *        label  foreign key into the version_label table
 *     document  identifies the document which this version represents
 *          num  sequential number of the version of the document
 */
CREATE TABLE doc_version_label

      (label INTEGER NOT NULL REFERENCES version_label,
    document INTEGER NOT NULL REFERENCES all_docs,
         num INTEGER NOT NULL,
 PRIMARY KEY (label, document))
GO
  
/* 
 * Permission for a document-type specific action, assigned to a group.
 *
 *          grp  group to which this permission is assigned
 *       action  identification of action which is permitted
 *     doc_type  identification of document type for which the action is 
 *               allowed
 *      comment  optional free-text annotation of any notes about the
 *               assignment of this permission to the group
 */
CREATE TABLE grp_action
        (grp INTEGER  NOT NULL REFERENCES grp,
      action INTEGER  NOT NULL REFERENCES action,
    doc_type INTEGER  NOT NULL REFERENCES doc_type,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (grp, action, doc_type))
GO

/* 
 * Membership of groups.
 *
 *          grp  group to which this user account has been assigned
 *          usr  identification of user account assigned to this group
 *      comment  optional free-text notes about this user's membership
 *               in this group
 */
CREATE TABLE grp_usr
        (grp INTEGER  NOT NULL REFERENCES grp,
         usr INTEGER  NOT NULL REFERENCES usr,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (grp, usr))
GO


/*************************************************************
 *      Link related tables
 *************************************************************/

/*
 * Valid link types.
 * Provides a unique identifier for a single link type.
 * Most types will appear in one specific field of one doctype, but
 *  it is possible for the same type to be used in more than one place.
 * Each unique type has defining properties, see table link_properties.
 *
 *     id       Unique id used in other tables.
 *     name     Human readable name
 *     chk_type Which table to check links in:
 *                'C' = Current active docs in document table
 *                'P' = Publishable active docs in doc_version
 *                'V' = Any doc in doc_version
 *     comment  Optional free text notes
 */
CREATE TABLE link_type (
          id INTEGER      IDENTITY PRIMARY KEY,
        name VARCHAR(32)  UNIQUE NOT NULL,
    chk_type CHAR         NOT NULL DEFAULT 'C',
     comment VARCHAR(255) NULL
)
GO

/*
 * Link source control.
 * Checked by link validation software to determine whether 
 *  a particular field is allowed to contain a particular link type.
 * A particular field may only contain one link type.  Otherwise
 *  the presence of a field with an href would not be enough to
 *  determine the type of link represented.
 *
 *     doc_type  Foreign key references doc_type table.
 *     element   Element in doc_type which may contain one of these links.
 *     link_id   Foreign key references link_type table.
 */
CREATE TABLE link_xml (
      doc_type INTEGER      NOT NULL REFERENCES doc_type,
       element VARCHAR(64)  NOT NULL,
       link_id INTEGER      NOT NULL REFERENCES link_type,
   PRIMARY KEY (doc_type, element)
)
GO

/*
 * Link target control
 * Checked by link validation software to determine whether
 *  a particular link refers to a document of the correct type.
 * Note that a single link type may refer to more than one target
 *  document type.
 *
 *    source_link_type  Type of link to be checked.
 *    target_doc_type   Allowed doc_type of target.  May be more than 1.
 */
CREATE TABLE link_target (
    source_link_type  INTEGER NOT NULL REFERENCES link_type,
     target_doc_type  INTEGER NOT NULL REFERENCES doc_type
)
GO

/*
 * Valid link property types.
 * Lists all link custom validation properties known to the system.
 * Each entry in this table essentially identifies a custom validation
 *  routine which may be invoked during link validation.
 * Entries may only be created by a programmer who has written code
 *  to support the custom validation identified here.  Entries must
 *  not be made by users.
 * See CdrLinkProcs.cpp for how custom link properties are validated.
 *     id       Unique id used in other tables.
 *     name     Human readable name
 *     comment  Optional free text notes
 */
CREATE TABLE link_prop_type (
          id INTEGER      IDENTITY PRIMARY KEY,
        name VARCHAR(32)  NOT NULL UNIQUE,
     comment VARCHAR(255)     NULL
)
GO

/*
 * Properties of links.
 * Checked by link validation software to determine whether 
 *  any given link or link set has the specified properties.
 *
 *     link_id   Identifier for a type of link.
 *     property  Identifier for a property type.
 *     value     A valid value of this property for this link type.
 *     comment   Free text comments.
 *
 * Example:
 *   Link target doc must contain certain field/value pairs.
 *      (we have a whole megillah for checking this.)
 */
CREATE TABLE link_properties (
      link_id INTEGER       NOT NULL REFERENCES link_type,
  property_id INTEGER       NOT NULL REFERENCES link_prop_type,
        value VARCHAR(1024)     NULL,
      comment VARCHAR(256)      NULL
)
GO

/*
 * Link network.
 * A database modelling all of the links found in the xml of all CDR
 *  documents.  It allows us to find all documents linked to any given
 *  document, either as source or target, without having to examine 
 *  the documents themselves.
 * If target doc format is not CDR xml, then url is used and target_doc
 *  and target_frag are null
 *
 *     link_type      An identifier for the type of link, e.g., author, etc.
 *     source_doc     Doc id containing a link element.
 *     source_elem    Element in source doc containing a link.
 *     target_doc     Doc id linked to by source (null if url used)
 *     target_frag    Fragment id linked to, if any.
 *     url            Alternative to target id + fragment for non CDR targets.
 *                    Note: url is not currently of any value because we
 *                          are not currently storing external references
 *                          in the link_net table.  But we've left it in
 *                          in case we ever do.
 *
 * XXX NOTE: foreign key constraint on target_doc column was
 *           disabled because we had been inserting links to doc 0.
 *           That is now changed and the constraint could be re-imposed.
 */
CREATE TABLE link_net (
          link_type INTEGER     NOT NULL REFERENCES link_type,
         source_doc INTEGER     NOT NULL REFERENCES all_docs,
        source_elem VARCHAR(64) NOT NULL,
         target_doc INTEGER         NULL /* REFERENCES all_docs */ ,
        target_frag VARCHAR(32)     NULL,
                url VARCHAR(256)    NULL
)
GO
CREATE INDEX link_net_source_idx ON link_net(source_doc)
GO
CREATE INDEX link_net_target_idx ON link_net(target_doc)
GO
CREATE INDEX link_net_targ_frag_idx ON link_net(target_doc, target_frag)
GO


/*
 * Document fragment link targets found in the system.
 * If a document has any "id" attributes, an entry is made
 *  for each doc_id + fragment.
 * This table allows us to quickly determine whether an attempt to link
 *  to a fragment within a document will succeed, without having to
 *  retrieve, parse, and search the target document
 *
 *     doc_id    Id of doc containing fragment.
 *     fragment  Value of id attribute in element.
 */
CREATE TABLE link_fragment (
         doc_id INTEGER     NOT NULL REFERENCES all_docs,
       fragment VARCHAR(64) NOT NULL,
    PRIMARY KEY (doc_id, fragment)
)
GO

/*************************************************************
 *      End link related tables
 *************************************************************/

/*
 * Contains searchable element data.  Populated for elements identified in
 * query_term_def table.
 *
 *       doc_id  ID of document being indexed.
 *         path  location of element in document.
 *        value  searchable element data.
 *      int_val  integer for first string of decimal digits in value.
 *     node_loc  string of 4-character hex numbers representing position
 *               of elements in path; doc-level node is assumed to be one;
 *               first node after document node is represented by the first
 *               four hex digits, the next node by the next four digits, etc.
 *               This column is used to determine which rows in the query_term
 *               table represent sibling under the same parent.
 */
CREATE TABLE query_term
     (doc_id INTEGER       NOT NULL REFERENCES all_docs,
        path VARCHAR(512)  NOT NULL,
       value VARCHAR(800)  NOT NULL,
     int_val INTEGER           NULL,
    node_loc VARCHAR(160)  NOT NULL)
GO
CREATE INDEX ix_query_term1 ON query_term(value, doc_id)
GO
CREATE INDEX ix_query_term2 ON query_term(path, doc_id)
GO
CREATE INDEX ix_query_term3 ON query_term(doc_id, node_loc)
GO
CREATE INDEX ix_query_term4 ON query_term(int_val, doc_id)
GO

/*
 * Contains searchable element data for the last publishable version
 * of each document.
 *
 * The structure and meaning of the fields is identical to query_term, q.v.
 */
CREATE TABLE query_term_pub
     (doc_id INTEGER       NOT NULL REFERENCES all_docs,
        path VARCHAR(512)  NOT NULL,
       value VARCHAR(800)  NOT NULL,
     int_val INTEGER           NULL,
    node_loc VARCHAR(160)  NOT NULL)
GO
CREATE INDEX ix_query_term_pub1 ON query_term_pub(value, doc_id)
GO
CREATE INDEX ix_query_term_pub2 ON query_term_pub(path, doc_id)
GO
CREATE INDEX ix_query_term_pub3 ON query_term_pub(doc_id, node_loc)
GO
CREATE INDEX ix_query_term_pub4 ON query_term_pub(int_val, doc_id)
GO

/*
 * Allows for future customization of the query support mechanism, using more
 * sophisticated indexing logic than simply the text content of a single
 * element.  Syntax TBD.
 *
 *       doc_id  ID of document being indexed.
 *           id  primary key for the indexing rule.
 *         name  name of the indexing rule (for display purposes).
 *     rule_def  specification of the logic to be used for indexing this
 *               element.
 */
CREATE TABLE query_term_rule
         (id INTEGER       IDENTITY PRIMARY KEY,
        name VARCHAR(32)   NOT NULL,
    rule_def VARCHAR(2000) NOT NULL)
GO

/*
 * Identifies elements which are to be indexed for querying.
 *
 *         path  hierarchical representation of element to be indexed;
 *               for example, 'Protocol/ProtSponsor'.
 *    term_rule  foreign key into query_term_rule table.
 */
CREATE TABLE query_term_def
       (path VARCHAR(512) NOT NULL,
   term_rule INTEGER          NULL REFERENCES query_term_rule)
GO

/*
 * Contains child-parent document ID pairs for Term document hierarchical
 * relationships.
 */
CREATE VIEW term_parents
         AS SELECT DISTINCT doc_id AS child,
                            CAST(SUBSTRING(value, 4, 10) AS INT) as parent
                       FROM query_term
                      WHERE path = '/Term/TermParent/@cdr:ref'
GO

/*
 * More efficient view of previous view.
 */
CREATE VIEW term_kids
         AS SELECT DISTINCT q.int_val AS parent,
                            q.doc_id  AS child,
                            d.title
                       FROM query_term q,
                            document d
                      WHERE d.id = q.doc_id
                        AND q.path = /* '/Term/TermParent/@cdr:ref' */
                            '/Term/TermRelationship/ParentTerm/TermId/@cdr:ref'
GO

/*
 * This version tells which nodes are leaves.
 */
CREATE VIEW term_children
         AS SELECT DISTINCT q1.int_val AS parent,
                            q1.doc_id  AS child,
                            COUNT(DISTINCT q2.doc_id) AS num_grandchildren,
                            d.title
                       FROM query_term q1,
                            query_term q2,
                            document d
                      WHERE d.id = q1.doc_id
                        AND q1.path = '/Term/TermParent/@cdr:ref'
                        AND q2.path = '/Term/TermParent/@cdr:ref'
                        AND q2.int_val =* q1.doc_id
                   GROUP BY q1.int_val, q1.doc_id, d.title
GO

/*
 * Table controlling values of dev_task.status.
 */
CREATE TABLE dev_task_status
     (status VARCHAR(20) NOT NULL PRIMARY KEY)
GO

/*
 * Table for tracking CDR development tasks.
 */
CREATE TABLE dev_task
         (id INTEGER     IDENTITY PRIMARY KEY,
 description TEXT        NOT NULL,
 assigned_to VARCHAR(30)     NULL,
      status VARCHAR(20) NOT NULL REFERENCES dev_task_status,
 status_date DATETIME    NOT NULL DEFAULT GETDATE(),
    category VARCHAR(32)     NULL,
est_complete DATETIME        NULL,
       notes TEXT            NULL)
GO

/*
 * Table for valid values in issue.priority column.
 */
CREATE TABLE issue_priority
    (priority VARCHAR(20) PRIMARY KEY)
GO

/*
 * Table for valid values in user columns of issue table.
 */
CREATE TABLE issue_user
       (name VARCHAR(30) PRIMARY KEY)
GO

/*
 * Table for tracking development issues for the CDR project (bugs, change
 * requests, etc.).
 *
 *           id  primary key for the issue.
 *       logged  date/time the issue was logged.
 *    logged_by  name of the project member who logged the issue.
 *     priority  level of urgency with which issue should be addressed.
 *  description  free text description of the problem or request.
 *     assigned  date/time the issue was assigned to a project team member.
 *  assigned_to  name of project member to whom the issue was assigned.
 *     resolved  date/time the issue was closed.
 *  resolved_by  name of project member who resolved the issue.
 *        notes  additional information about the issue, including information
 *               about how the issue was resolved.
 */
CREATE TABLE issue
         (id INTEGER IDENTITY PRIMARY KEY,
      logged DATETIME    NOT NULL,
   logged_by VARCHAR(30) NOT NULL REFERENCES issue_user,
    priority VARCHAR(20) NOT NULL REFERENCES issue_priority,
 description TEXT        NOT NULL,
    assigned DATETIME        NULL,
 assigned_to VARCHAR(30)     NULL REFERENCES issue_user,
    resolved DATETIME        NULL,
 resolved_by VARCHAR(30)     NULL REFERENCES issue_user,
       notes TEXT            NULL)
GO

/*
 * Table used to track development tasks for CDR reports.
 *
 *  description  brief textual identification of report
 *         spec  location of specification document (if any)
 *       sample  location of sample report document (if any)
 *     dev_task  foreign key reference to dev_task table
 */
CREATE TABLE report_task
(description VARCHAR(255) NOT NULL PRIMARY KEY,
        spec VARCHAR(255)     NULL,
      sample VARCHAR(255)     NULL,
    dev_task INT              NULL REFERENCES dev_task)
GO

/*
 * Table used to track processing of publication events.
 *
 *           id  primary key for the publication event processing.
 *   pub_system  name of the system requesting the publishing event e.g. (UDB).
 *   pub_subset  name of the publishing subset used for the event.
 *          usr  ID of user requesting the publication event.
 *   output_dir  output directory (without .username.datetime.status).
 *      started  date/time processing commenced.
 *    completed  date/time processing was finished.
 *       status  see list of valid status values enumerated at the top
 *               of the class definition for Publish in cdrpub.py.
 *     messages  messages generated during processing.
 *     external  flag indicating whether the publishing event took place
 *               outside the CDR (such as historical publications imported
 *               from the PDQ Oracle tables); further details in the messages
 *               column when appropriate.
 */
CREATE TABLE pub_proc
         (id INTEGER IDENTITY PRIMARY KEY,
  pub_system INTEGER      NOT NULL REFERENCES all_docs,
  pub_subset VARCHAR(255) NOT NULL,
         usr INTEGER      NOT NULL REFERENCES usr,
  output_dir VARCHAR(255) NOT NULL,
     started DATETIME     NOT NULL,
   completed DATETIME         NULL,
      status VARCHAR(32)  NOT NULL,
    messages NTEXT            NULL,
       email VARCHAR(255)     NULL,
    external CHAR(1)          NULL DEFAULT 'N',
   no_output CHAR(1)      NOT NULL DEFAULT 'N')
GO

/*
 * Table used to record parameters used for processing a publication event.
 *
 *           id  used with pub_proc to form the primary key.
 *     pub_proc  foreign key into the pub_proc table.
 *    parm_name  name of the parameter.
 *   parm_value  value of the parameter.
 */
CREATE TABLE pub_proc_parm
         (id INTEGER      NOT NULL,
    pub_proc INTEGER      NOT NULL REFERENCES pub_proc,
   parm_name VARCHAR(32)  NOT NULL,
  parm_value NVARCHAR(255)    NULL,
  CONSTRAINT pub_proc_parm_pk      PRIMARY KEY(pub_proc, id))
GO

/*
 * Table used to record processing of a document for a publication event.
 *
 *     pub_proc  foreign key into the pub_proc table.
 *       doc_id  identification of document being processed.
 *  doc_version  part of foreign key into doc_version table.
 *      failure  set to 'Y' if document could not be published.
 *     messages  optional description of any failures.
 *      removed  flag indicating that instead of an export of a document,
 *               this row refers to an instruction sent to Cancer.Gov
 *               to remove the document from the web site.
 */
CREATE TABLE pub_proc_doc
   (pub_proc INTEGER      NOT NULL  REFERENCES pub_proc,
      doc_id INTEGER      NOT NULL,
 doc_version INTEGER      NOT NULL,
     failure CHAR             NULL,
    messages NTEXT            NULL,
     removed CHAR(1)          NULL DEFAULT 'N',
      subdir VARCHAR(32)      NULL DEFAULT '',
  CONSTRAINT pub_proc_doc_fk        PRIMARY KEY(pub_proc, doc_id, doc_version),
  CONSTRAINT pub_proc_doc_fk_docver FOREIGN KEY(doc_id, doc_version) 
                                    REFERENCES doc_version)
GO

/*
 * Collects information about documents to be remailed in a given job.
 * Entries in this table normally exist only until the job is
 * complete, after which they are deleted.  A system crash could
 * leave useless entries in the table which serve no function and
 * can be cleaned out if desired.
 *
 *          job  Publication job id, foreign key into the pub_proc table.
 *          doc  Document to be mailed
 *      tracker  Id of mailer tracking doc for original mailing for
 *               which this is a remailer.
 *               There may be more than one for one doc.
 *    recipient  Id of person or org document for recipient of the
 *               remailing.  There must be one per tracker.
 */
CREATE TABLE remailer_ids
        (job INTEGER      NOT NULL REFERENCES pub_proc,
         doc INTEGER      NOT NULL,
     tracker INTEGER      NOT NULL,
   recipient INTEGER      NULL,
  CONSTRAINT remailer_ids_doc_fk FOREIGN KEY (doc) 
                                 REFERENCES all_docs,
  CONSTRAINT remailer_ids_tracker_fk FOREIGN KEY (tracker)
                                     REFERENCES all_docs,
  CONSTRAINT remailer_ids_recipient_fk FOREIGN KEY (recipient)
                                       REFERENCES all_docs)
GO

/*
 * Holds all information for a batch job.
 * These are not publishing jobs, which have their own table, but
 * other types of batch jobs.
 *
 *           id  Batch job id.
 *         name  Human readable name of this job.
 *      command  Executable command line, may include @@vars, see cdrbatch.py.
 *               May also include command line args, but see batch_job_parm
 *               for more flexible way to pass args to Python progs.
 *   process_id  OS process identifier.
 *      started  DateTime job was queued.
 *         last  DateTime last status was set.
 *       status  Last known status of job.  See cdrbatch.py
 *        email  Send email here with final status.
 * progress_msg  Job can set this to allow status query.
 *               At end, job might set final info here.
 */
CREATE TABLE batch_job
         (id INTEGER  IDENTITY PRIMARY KEY,
        name VARCHAR (512) NOT NULL,
     command VARCHAR (512) NOT NULL,
  process_id INTEGER NULL,
     started DATETIME NOT NULL,
   status_dt DATETIME NOT NULL,
      status VARCHAR (16) NOT NULL,
       email VARCHAR (256) NULL,
    progress TEXT NULL)
GO

/*
 * Parameters for a batch job.
 *
 *          job  batch_job id for which these are parameters.
 *         name  Name of parameter, application dependent.
 *        value  Value as a string, application dependent.
 */
CREATE TABLE batch_job_parm
        (job INTEGER NOT NULL REFERENCES batch_job,
        name VARCHAR (32) NOT NULL,
       value VARCHAR (256) NOT NULL)
GO


/*
 * View for completed publication events.
 */
CREATE VIEW pub_event
         AS SELECT *
              FROM pub_proc
             WHERE completed IS NOT NULL
GO

/*
 * View of documents which have been successfully published.
 */
CREATE VIEW published_doc
         AS SELECT pub_proc_doc.*
              FROM pub_proc_doc, pub_event
             WHERE pub_event.status = 'Success'
               AND pub_event.id = pub_proc_doc.pub_proc
               AND (pub_proc_doc.failure IS NULL
                OR pub_proc_doc.failure <> 'Y')
GO

/*
 * View of documents control system behavior.
 */
CREATE VIEW control_docs
         AS SELECT document.*
              FROM document
              JOIN doc_type
                ON doc_type.id = document.doc_type
             WHERE doc_type.name IN ('css', 
                                     'Filter', 
                                     'PublishingSystem', 
                                     'schema')
GO

/*
 * Convenience view for determining the creation date of a document.
 */
CREATE VIEW creation_date
AS
    SELECT d.id, MIN(a.dt) AS dt
      FROM document d
      JOIN audit_trail a
        ON d.id = a.document
  GROUP BY d.id
GO

/*
 * Alternate definition of same view (different view column names).
 */
CREATE VIEW doc_created          
AS 
    SELECT document AS doc_id, MIN(dt) AS created
      FROM audit_trail           
  GROUP BY document
GO

/*
 * Convenience view for interesting document information.
 */
CREATE VIEW doc_header
AS
  SELECT d.id AS DocId,
         t.name AS DocType,
         d.title AS DocTitle
   FROM  document d,
         doc_type t
  WHERE  d.doc_type = t.id
GO

/*
 * View used to determine when it's appropriate to send update mailers.
 */
CREATE VIEW last_doc_publication
         AS SELECT published_doc.doc_id,
                   pub_event.pub_subset,
                   MAX(pub_event.completed) as dt
              FROM published_doc
              JOIN pub_event
                ON pub_event.id = published_doc.pub_proc
          GROUP BY pub_event.pub_subset, published_doc.doc_id
GO

/*
 * Terminology tree display support.
 */
CREATE VIEW orphan_terms 
AS 
    SELECT DISTINCT d.title            
               FROM document d,
                    query_term q           
              WHERE d.id = q.doc_id
                AND q.path = '/Term/TermPrimaryType'             
                AND q.value <> 'glossary term'  
                AND NOT EXISTS (SELECT *                    
                                  FROM query_term q2
                                 WHERE q2.doc_id = d.id                     
                                   AND q2.path = '/Term/TermParent/@cdr:ref')
GO

/*
 * More terminology tree display support.
 */
CREATE VIEW TermsWithParents 
AS 
    SELECT d.id, d.xml   
      FROM document d,
           doc_type t  
     WHERE d.doc_type = t.id    
       AND t.name     = 'Term'    
       AND d.xml LIKE '%<TermParent%'
GO

/*
 * Useful view for troubleshooting login failures or detecting intruders.
 */
CREATE VIEW failed_login_attempts
AS
    SELECT recorded,
           source,
           msg
      FROM debug_log
     WHERE source LIKE 'Failed logon attempt%'
GO

/*
 * Used by the document version history report.
 */
CREATE VIEW doc_info AS
             SELECT d.id        doc_id,
                    d.title     doc_title,
                    d.active_status doc_status, 
                    t.name      doc_type,
                    u1.fullname created_by,
                    a1.dt       created_date,
                    u2.fullname mod_by,
                    a2.dt       mod_date
               FROM document d
               JOIN doc_type t
                 ON t.id = d.doc_type
    LEFT OUTER JOIN audit_trail a1
                 ON a1.document = d.id
                AND a1.dt = (SELECT MIN(audit_trail.dt)
                               FROM audit_trail
                               JOIN action
                                 ON action.id = audit_trail.action
                              WHERE audit_trail.document = d.id
                                AND action.name = 'ADD DOCUMENT')
    LEFT OUTER JOIN usr u1
                 ON u1.id = a1.usr
    LEFT OUTER JOIN audit_trail a2
                 ON a2.document = d.id
                AND a2.dt = (SELECT MAX(audit_trail.dt)
                               FROM audit_trail
                               JOIN action
                                 ON action.id = audit_trail.action
                              WHERE audit_trail.document = d.id
                                AND action.name = 'MODIFY DOCUMENT')
    LEFT OUTER JOIN usr u2
                 ON u2.id = a2.usr
GO

/*
 * Used by New Documents with Publication Status report.
 */
CREATE VIEW docs_with_pub_status
AS
         SELECT d.id             doc_id,
                cu.name          cre_user,
                c.dt             cre_date,
                v.dt             ver_date,
                vu.name          ver_user,
                t.name           doc_type,
                d.title          doc_title,
                pv = CASE
                          WHEN v.publishable IS NOT NULL
                           AND v.publishable = 'Y'
                           AND d.active_status = 'A'
                               THEN 'Y'
                          ELSE      'N'
                     END,
                epv = (SELECT COUNT(*)
                         FROM doc_version
                        WHERE id = d.id
                          AND num < v.num
                          AND publishable = 'Y')
           FROM document d
           JOIN audit_trail c
             ON c.document = d.id
            AND c.dt = (SELECT MIN(audit_trail.dt)
                          FROM audit_trail
                          JOIN action
                            ON action.id = audit_trail.action
                         WHERE audit_trail.document = d.id
                           AND action.name = 'ADD DOCUMENT')
           JOIN usr cu
             ON cu.id = c.usr
           JOIN doc_type t
             ON t.id = d.doc_type
LEFT OUTER JOIN doc_version v
             ON v.id = d.id
            AND v.num = (SELECT MAX(num)
                           FROM doc_version
                          WHERE id = d.id)
LEFT OUTER JOIN usr vu
             ON vu.id = v.usr
GO

/*
 * Used for finding publishing jobs for licensees and Cancer.gov.
 */
CREATE VIEW primary_pub_job
AS
    SELECT pub_proc.*
      FROM pub_proc
      JOIN document
        ON document.id = pub_proc.pub_system
     WHERE document.title = 'Primary'
       AND pub_proc.status = 'Success'
       AND pub_proc.completed IS NOT NULL
GO

CREATE VIEW primary_pub_doc
AS
    SELECT pub_proc_doc.*, primary_pub_job.completed,
           primary_pub_job.started, primary_pub_job.output_dir
      FROM pub_proc_doc
      JOIN primary_pub_job
        ON pub_proc_doc.pub_proc = primary_pub_job.id
     WHERE pub_proc_doc.failure IS NULL
GO

/*
 * Table used to remember the set of documents which Cancer.Gov has.
 *
 *           id  primary key of document sent to Cancer.Gov.
 *     pub_proc  identifies job which sent the document to Cancer.Gov.
 *          xml  copy of the filtered document sent to Cancer.Gov.
 *   force_push  flag indicating that the document can be pushed
 *               to Cancer.gov even if it is identical with what
 *               we sent with the previous job.
 *       cg_new  indicates whether this document is missing from 
 *               the live Cancer.gov site or the Cancer.gov Preview
 *               stage; set to 'N' in the normal case; set to 'Y'
 *               for documents which were pushed to Cancer.gov
 *               but later failed processing; this allows the 
 *               module which groups published documents which
 *               must all fail together to recognize which documents
 *               aren't really available on Cancer.gov, even though
 *               they have rows in the pub_proc_cg table.
 */
CREATE TABLE pub_proc_cg
         (id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs,
    pub_proc INTEGER NOT NULL REFERENCES pub_proc,
         xml NTEXT   NOT NULL,
  force_push CHAR    NOT NULL,
      cg_new CHAR    NOT NULL)
GO

/*
 * Table used to remember trials we have sent to ClinicalTrials.gov
 * at NLM.
 *
 *                id  primary key of document sent to NLM.
 *               xml  copy of the filtered document sent to NLM.
 *         last_sent  date/time of the most recent export to NLM.
 * drop_notification  date/time we last told NLM the trial has been pulled
 *                    from Cancer.gov.
 *              
 */
     CREATE TABLE pub_proc_nlm
              (id INTEGER      NOT NULL PRIMARY KEY REFERENCES all_docs,
              xml NTEXT        NOT NULL,
        last_sent DATETIME     NOT NULL,
      last_status VARCHAR(255)     NULL,
drop_notification DATETIME         NULL)
GO

/*
 * Table used to hold working information on transactions to
 * Cancer.Gov. The information will be permanently stored to 
 * pub_proc_cg and pub_proc_doc after transactions to Cancer.Gov 
 * are completed. The information will be deleted after they are 
 * updated successfully to pub_proc_cg and pub_proc_doc.
 * 
 * This table rather than a temporary table is created to guarantee
 * the state of pub_proc_doc and pub_proc_cg can be kept in sync 
 * with Cancer.Gov.
 *
 *           id  primary key of document sent to Cancer.Gov.
 *          num  version of the document sent to Cancer.Gov.
 *       cg_job  job which sent the document to Cancer.Gov.
 *   vendor_job  job which produced the filtered documents.
 *     doc_type  document type name of the document.
 *          xml  copy of the filtered document sent to Cancer.Gov;
 *               NULL denotes deleted document.
 */
CREATE TABLE pub_proc_cg_work
         (id INTEGER NOT NULL PRIMARY KEY REFERENCES all_docs,
         num INTEGER NOT NULL,
  vendor_job INTEGER NOT NULL REFERENCES pub_proc,
      cg_job INTEGER NOT NULL REFERENCES pub_proc,
    doc_type VARCHAR(32) NOT NULL,
         xml NTEXT NULL)
GO

/*
 * Debugging table for tracking commands.
 *  
 *       thread  logging ID used to identify this thread since the process
 *               started.
 *     received  date/time the command set was received by the server.
 *      command  full XML string (UTF-8 encoded) for the CdrCommandSet.
 */
CREATE TABLE command_log
     (thread INTEGER NOT NULL,
    received DATETIME NOT NULL,
     command TEXT NOT NULL,
  CONSTRAINT command_log_pk PRIMARY KEY(thread, received))
CREATE INDEX command_log_time ON command_log(received)
GO

/*
 * Named collection of CDR XSL/T filters and nested filter sets.
 *
 *      command  full XML string (UTF-8 encoded) for the CdrCommandSet.
 *           id  automatically generated primary key.
 *         name  used by scripts to invoke the set of filters.
 *  description  brief description of the set, used to describe the
 *               filter set's use in a user interface (for example,
 *               in a popup help tip).
 *        notes  more extensive optional notes on the use of this
 *               filter set.
 */
CREATE TABLE filter_set
         (id INTEGER IDENTITY NOT NULL PRIMARY KEY,
        name VARCHAR(80)      NOT NULL UNIQUE,
 description NVARCHAR(256)    NOT NULL,
       notes NTEXT                NULL)
GO

/*
 * Member of a filter set.
 *
 *   filter_set  identifies which set this is a member of.
 *     position  identifies how to sequence this member within the set.
 *       filter  identifies a filter which is a member of the set.
 *       subset  identifies a nested filter set.
 */
CREATE TABLE filter_set_member
 (filter_set INTEGER NOT NULL REFERENCES filter_set,
    position INTEGER NOT NULL,
      filter INTEGER     NULL REFERENCES all_docs,
      subset INTEGER     NULL REFERENCES filter_set,
  CONSTRAINT filter_set_member_pk PRIMARY KEY(filter_set, position))
GO

/*
 * System wide control table, used for any arbitrary name value pairs
 * that the system needs to access.
 *
 *         name  The name associated with this value.
 *               By convention, applications should group related values by
 *               using related names, (arbitrary example below):
 *                  mailer/Person/maxmailers = ...
 *                  mailer/Person/interval = ...
 *                  mailer/Organization/maxmailers = ...
 *               Because name is unique, we limit len to below sys limit
 *               of 900 chars on index length.  Should be more than enough.
 *        value  String to retrieve for this name.
 *  last_change  Date-time when value last set.
 *          usr  ID of user making the last change.
 *      program  Optional name of program making the last change.
 *        notes  Optional notes documenting this system value.
 */
CREATE TABLE sys_value
       (name VARCHAR(800) NOT NULL UNIQUE,
       value VARCHAR(2000) NOT NULL,
         usr INT NOT NULL REFERENCES usr,
 last_change DATETIME NOT NULL,
     program varchar(64) NULL,
       notes NTEXT NULL)
GO

/*
 * Table of valid values for disposition status of documents imported 
 * from ClinicalTrials.gov.
 *
 *           id  uniquely identifies the status value
 *         name  string used to represent the value
 *      comment  optional user comments
 */
CREATE TABLE ctgov_disposition
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32)      NOT NULL UNIQUE,
     comment NTEXT                NULL)
GO

/*
 * Table for tracking documents downloaded from NLM's CancerTrials.gov site.
 *
 *       nlm_id  uniquely identifies the document within CT.gov
 *        title  official title, if present; otherwise brief title
 *          xml  unmodified XML document as downloaded from NLM
 *   downloaded  date/time of download
 *  disposition  foreign key into ctgov_disposition table
 *           dt  date/time of most recent disposition change
 *     verified  date/time document was last verified at NLM
 *      changed  date/time document was last changed at NLM
 *       cdr_id  document ID in CDR (if imported)
 *      dropped  flag indicating that NLM is no longer exporting this trial
 *      comment  user comments, if any
 */
CREATE TABLE ctgov_import
     (nlm_id VARCHAR(16)   NOT NULL PRIMARY KEY,
       title NVARCHAR(255)     NULL,
         xml NTEXT             NULL,
  downloaded DATETIME          NULL,
 disposition INTEGER       NOT NULL REFERENCES ctgov_disposition,
          dt DATETIME      NOT NULL,
    verified DATETIME          NULL,
     changed DATETIME          NULL,
      cdr_id INTEGER           NULL REFERENCES all_docs,
     dropped CHAR          NOT NULL DEFAULT 'N',
     comment NTEXT             NULL,
       phase VARCHAR(20)       NULL)
GO
CREATE INDEX ctgov_import_title ON ctgov_import(title)
GO
CREATE INDEX ctgov_import_down  ON ctgov_import(downloaded)
GO

/*
 * Record of each CTGovProtocol import job.
 *
 *           id  primary key for table
 *           dt  datetime job was run
 */
CREATE TABLE ctgov_import_job
         (id INTEGER IDENTITY PRIMARY KEY,
          dt DATETIME NOT NULL)
GO
CREATE INDEX ctgov_import_job_dt ON ctgov_import_job(dt)
GO

/*
 * Remembers what happened to each document processed by a given
 * CTGov import job.
 *
 *          job  foreign key into ctgov_import_job table
 *       nlm_id  foreign key into ctgov_import table
 *          new  'Y' if this is the first import of the document; otherwise 'N'
 * needs_review  'Y' if NLM made changes which might affect PDQ indexing;
 *               otherwise 'N'
 *  pub_version  'Y' if a publishable version was created; otherwise 'N'
 */
CREATE TABLE ctgov_import_event
        (job INTEGER     NOT NULL REFERENCES ctgov_import_job,
      nlm_id VARCHAR(16) NOT NULL REFERENCES ctgov_import,
      locked CHAR(1)     NOT NULL DEFAULT 'N',
         new CHAR(1)         NULL,
needs_review CHAR(1)         NULL,
 pub_version CHAR(1)         NULL,
  CONSTRAINT ctgov_import_event_pk PRIMARY KEY(job, nlm_id))
GO

/*
 * Remembers statistics from download jobs for trials from ClinicalTrials.gov.
 *
 *           id  primary key
 *           dt  date/time of download job
 * total_trials  number of cancer trials that meet query criteria
 *   new_trials  new active and approved-not yet active trials
 *      updated  Updates to previously downloaded trials
 *    unchanged  skipped trials since no changes from previous download
 *      pdq_cdr  skipped trials from PDQ/CDR
 *   duplicates  skipped duplicate trials
 * out_of_scope  Skipped out of scope trials
 *       closed  Skipped closed and completed trials
 */
CREATE TABLE ctgov_download_stats
         (id INTEGER  IDENTITY PRIMARY KEY,
          dt DATETIME NOT NULL,
total_trials INTEGER  NOT NULL,
  new_trials INTEGER  NOT NULL,
     updated INTEGER  NOT NULL,
   unchanged INTEGER  NOT NULL,
     pdq_cdr INTEGER  NOT NULL,
  duplicates INTEGER  NOT NULL,
out_of_scope INTEGER  NOT NULL,
      closed INTEGER  NOT NULL)
GO
CREATE INDEX ctgov_download_stats_dt ON ctgov_download_stats (dt)
GO

/*
 * Logs filter document IDs when filter profiling is turned on by
 * setting the environment variable CDR_FILTER_PROFILING before
 * starting the CdrServer.  See CdrFilter.cpp.
 *
 *      id  CDR document ID of a filter that was invoked
 *  millis  Number of milliseconds elapsed between start/end of filtering
 *      dt  Date/time of invocation
 *
 * Typical usage is:
 *   SELECT id AS FilterID, COUNT(millis) AS Count, AVG(millis) AS AvgMils, 
 *          MAX(millis) AS Max, MIN(millis) AS Min, STDEV(millis) AS StdDev
 *     FROM filter_profile
 *    WHERE dt BETWEEN (... AND ...)
 * GROUP BY id
 * ORDER BY id
 *
 * Rows may be deleted at any time without harming CDR operations.
 */
CREATE TABLE filter_profile
        (id INTEGER,
     millis INTEGER,
         dt DATETIME)
GO

/*
 * Records jobs used to push documents to Cancer.gov, so we can remember
 * whether and when we sent any protocols from the job to NLM.
 *
 *  pub_proc  Identifies job by ID
 *  exported  Records time CTGov export was run for this job.
 */
CREATE TABLE ctgov_export
   (pub_proc INTEGER PRIMARY KEY REFERENCES pub_proc,
    exported DATETIME NULL)
GO

/*
 * Table of valid values for disposition status of documents imported 
 * from an outside organization.
 *
 *           id  uniquely identifies the status value
 *         name  string used to represent the value; e.g. 'imported',
 *               'pending', 'unmatched'
 *      comment  optional user comments
 */
CREATE TABLE import_disposition
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32)      NOT NULL UNIQUE,
     comment NTEXT                NULL)
GO

/*
 * External organizations from which we import documents.
 *
 *           id  primary key, automatically generated
 *         name  name of the source (e.g., RSS)
 *      comment  optional description of the external organization
 */
CREATE TABLE import_source
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32)      NOT NULL UNIQUE,
     comment NTEXT                NULL)
GO

/*
 * Table for tracking documents downloaded from an external source.
 *
 *           id  primary key, automatically generated by the database
 *       source  foreign key linking to the external_source table
 *    source_id  uniquely identifies the document within the external
 *               organization
 *        title  optional title used to display the document's name
 *          xml  unmodified XML document as downloaded from the external 
 *               source
 *   downloaded  date/time of download
 *  disposition  foreign key into ctgov_disposition table
 *      disp_dt  date/time of most recent disposition change
 *       cdr_id  document ID in CDR (if imported)
 *      dropped  records when we detect that document is no longer exported
 *      comment  user comments, if any
 */
CREATE TABLE import_doc
         (id INTEGER       IDENTITY PRIMARY KEY,
      source INTEGER       NOT NULL REFERENCES import_source,
   source_id VARCHAR(64)   NOT NULL,
       title NVARCHAR(255)     NULL,
         xml NTEXT             NULL,
  downloaded DATETIME          NULL,
 disposition INTEGER       NOT NULL REFERENCES import_disposition,
     disp_dt DATETIME      NOT NULL,
    verified DATETIME          NULL,
     changed DATETIME          NULL,
      cdr_id INTEGER           NULL REFERENCES all_docs,
     dropped DATETIME          NULL,
     comment NTEXT             NULL,
CONSTRAINT import_doc_unique UNIQUE (source, source_id))
GO
CREATE INDEX import_doc_title ON import_doc(title)
GO
CREATE INDEX import_doc_down  ON import_doc(downloaded)
GO

/*
 * Record of each job importing a batch of external documents.
 *
 *           id  primary key for table
 *           dt  datetime job was run
 *       source  link to import_source table
 *       status  'Started', 'In progress', 'Failure', 'Success'
 */
CREATE TABLE import_job
         (id INTEGER     IDENTITY PRIMARY KEY,
          dt DATETIME    NOT NULL,
      source INTEGER     NOT NULL REFERENCES import_source,
      status VARCHAR(20) NOT NULL)
GO
CREATE INDEX import_job_dt ON import_job(dt)
GO
CREATE INDEX import_job_source ON import_job(source, status, dt)
GO

/*
 * Remembers what happened to each document processed by a given
 * import job.
 *
 *          job  foreign key into import_job table
 *          doc  foreign key into import_doc table
 *       locked  'Y' if we were unable to modify the document because it was 
 *               locked; otherwise 'N'
 *          new  'Y' if this is the first import of the document; otherwise 'N'
 * needs_review  'Y' if anomalies were detected which may require user review;
 *               otherwise 'N'
 *  pub_version  'Y' if a publishable version was created; otherwise 'N'
 */
CREATE TABLE import_event
        (job INTEGER     NOT NULL REFERENCES import_job,
         doc INTEGER     NOT NULL REFERENCES import_doc,
      locked CHAR(1)     NOT NULL DEFAULT 'N',
         new CHAR(1)     NOT NULL DEFAULT 'N',
needs_review CHAR(1)     NOT NULL DEFAULT 'N',
 pub_version CHAR(1)     NOT NULL DEFAULT 'N',
  CONSTRAINT import_event_pk PRIMARY KEY(job, doc))
GO

/*
 * Table used to determine whether a mapping is to a document of the
 * correct type.
 *
 *     usage  foreign key into the external_map_usage table
 *  doc_type  foreign key into the doc_type table
 */
CREATE TABLE external_map_type
      (usage INTEGER NOT NULL REFERENCES external_map_usage,
    doc_type INTEGER NOT NULL REFERENCES doc_type,
PRIMARY KEY (usage, doc_type))
GO

/*
 * Sets of GENETICSPROFESSIONAL documents to be imported from CIAT-maintained
 * external database.
 *
 *           id  primary key for table
 * submitted_by  foreign key into usr table for user submitting set
 *         path  location of zipfile containing gp document set
 *     uploaded  date/time the set was submitted by CIAT
 *       status  U (uploaded), P (processing), I (imported), or E (error)
 *     imported  date/time the set was imported into the repository
 *       errors  description of failure, if imported did not succeed
 */
CREATE TABLE gp_import_set
         (id INTEGER       IDENTITY PRIMARY KEY,
submitted_by INTEGER       NOT NULL REFERENCES usr,
        path VARCHAR(1024) NOT NULL,
    uploaded DATETIME      NOT NULL,
      status CHAR          NOT NULL DEFAULT 'U',
    imported DATETIME          NULL,
      errors NTEXT             NULL)
GO
