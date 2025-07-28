/*
 * DBMS tables for the ICIC Central Database Repository
 *
 * This script contains the definitions for the tables and views
 * contained in the two CDR database, as well as documentation for
 * those tables and views and each of the columns which they contain.
 *
 * The two tables which form the core of the repository are the
 * all_docs and the all_doc_versions tables (used through the
 * corresponding views document and doc_version). The xml columns
 * in these tables contain the serialization for all of the CDR
 * XML documents. Everything else in the system exists to support
 * these documents. The other tables include:
 *  - tables to record user accounts, session, groups, and permissions
 *  - tables to control different document types
 *  - tables to support workflow (document locking, auditing, etc.)
 *  - tables to hold multimedia and other blobs associated with documents
 *  - tables to manage inter-document linking
 *  - tables to support document search/selection
 *  - tables to map internal identifiers to identifiers in external systems
 *  - tables to manage publication and other batch jobs
 *  - tables to manage import from and export to external systems (obsolete)
 *
 * This script assumes the cdr and cdr_archived_versions tables have been
 * freshly created, with no user tables.
 *
 * BZIssue::4925
 * BZIssue::4924 - Modify Summary Date Last Modified Report
 * BZIssue::5294 (OCECDR-3595) - new ctrp_id column for ctgov_import table
 */

USE cdr_archived_versions
GO

/*
 * This is the table which holds the older versions of CDR documents.
 * It is separated out in order to keep the size of the main cdr
 * database low enough that we can avoid some of the messier bugs
 * in the Windows file system that we used to encounter in backing
 * up and copying everything.  See notes on all_doc_versions table
 * and doc_version in the cdr database view below.
 *
 *           id  identifies the document which this version represents
 *          num  sequential number of the version of the document; numbering
 *               starts with 1
 *          xml  for structured documents, this is the data for the document;
 *               for unstructured documents this contains tagged textual
 *               information associated with the document (for example, the
 *               standard caption for an illustration)
 */
CREATE TABLE doc_version_xml
             (id INTEGER      NOT NULL,
             num INTEGER      NOT NULL,
             xml NTEXT        NOT NULL,
     PRIMARY KEY (id, num))
GO

USE cdr
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
             (id INTEGER      IDENTITY PRIMARY KEY,
            name VARCHAR(32)  NOT NULL UNIQUE,
doctype_specific CHAR(1)          NULL DEFAULT 'N',
         comment NVARCHAR(255   ) NULL)
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
         (id CHAR        PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment NVARCHAR(255)   NULL)
GO

/*
 * Version control.  This is now implemented with a base table whose xml
 * column can be set to NULL, in which case the document for the version cat
 * be retrieved from a separate database with a single table to store the
 * older versions of documents.  New versions have their xml stored
 * directly in the base table.  Every year (or six months if we need to do it
 * more frequently) the xml is moved to the separate database and the xml
 * column in the base table is set to NULL.  The doc_version view now
 * presents the data in a way that is transparent to the software in the
 * rest of the system, which has no knowledge of the database for the
 * archived XML.
 *
 *           id  identifies the document which this version represents
 *          num  sequential number of the version of the document; numbering
 *               starts with 1
 *           dt  date/time the document was checked in - by GETDATE()
 *               in checkin transaction
 *               don't know if it's used for anything at all
 *   updated_dt  date/time identical to what's in the the audit_trail
 *               this is the important date needed to match a version to
 *               an action recorded in the audit_trail or wherever.
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
CREATE TABLE all_doc_versions
             (id INTEGER       NOT NULL,
             num INTEGER       NOT NULL,
              dt DATETIME      NOT NULL,
      updated_dt DATETIME      NOT NULL,
             usr INTEGER       NOT NULL,
      val_status CHAR          NOT NULL,
        val_date DATETIME          NULL,
     publishable CHAR          NOT NULL,
        doc_type INTEGER       NOT NULL,
           title NVARCHAR(255) NOT NULL,
             xml NTEXT             NULL,
         comment NVARCHAR(255)     NULL,
     PRIMARY KEY (id, num))
GO
GRANT INSERT, UPDATE ON all_doc_versions TO CdrPublishing
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
 *               WARNING:
 *                  val_date is the date last validated, not the date last
 *                  stored.  It is legal and common to store an unvalidated
 *                  current working document which causes val_status to be set
 *                  to 'U' but leaves val_date alone.
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
 *               have the value 'N' for the first_pub_knowable column,
 *               in which case the publication subsystem must never
 *               insert a non-NULL value into the first_pub column.
 */
      CREATE TABLE all_docs
               (id INTEGER IDENTITY PRIMARY KEY,
        val_status CHAR          NOT NULL DEFAULT 'U',
          val_date DATETIME          NULL,
          doc_type INTEGER       NOT NULL,
             title NVARCHAR(255) NOT NULL,
               xml NTEXT         NOT NULL,
           comment NVARCHAR(255)     NULL,
     active_status CHAR          NOT NULL DEFAULT 'A',
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

/**
 * Replacement for command_log table (when CDR service was eliminated)
 */
CREATE TABLE api_request
 (request_id INTEGER IDENTITY PRIMARY KEY,
  process_id INTEGER,
   thread_id INTEGER,
    received DATETIME,
     request NTEXT)
CREATE INDEX api_request_received ON api_request(received)
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
   (document INTEGER      NOT NULL,
          dt DATETIME     NOT NULL,
         usr INTEGER      NOT NULL,
      action INTEGER      NOT NULL,
     program VARCHAR(32)      NULL,
     comment NVARCHAR(255)    NULL,
 PRIMARY KEY (document, dt))
GO

/*
 * Additional action associated with a row in the audit_trail table.
 * The immediate use for this table is to record explicitly when the
 * all_docs.active_status value changes between 'A' (active) and 'I'
 * (inactive).  We initially considered implementing support for that
 * requirement by simply inserting an additional row in the audit_trail
 * table, but there were views and other software which relied on
 * the primary key for that table restricting the combination of
 * document ID and DATETIME for the audited action to a single row.
 *
 *     document  identifies the document on which this action was performed
 *           dt  date/time the action occurred
 *       action  identification of the action which was performed
 */
CREATE TABLE audit_trail_added_action
   (document INTEGER      NOT NULL,
          dt DATETIME     NOT NULL,
      action INTEGER      NOT NULL,
 CONSTRAINT audit_added_pk PRIMARY KEY (document, dt, action))
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
         (id INTEGER IDENTITY PRIMARY KEY,
        name NVARCHAR(512) NOT NULL,
     command NVARCHAR(512) NOT NULL,
  process_id INTEGER           NULL,
     started DATETIME      NOT NULL,
   status_dt DATETIME      NOT NULL,
      status VARCHAR(16)   NOT NULL,
       email VARCHAR(256)      NULL,
    progress NVARCHAR(MAX)     NULL)
GO
GRANT INSERT, UPDATE ON batch_job TO CdrPublishing
GO

/*
 * Parameters for a batch job.
 *
 *          job  batch_job id for which these are parameters.
 *         name  Name of parameter, application dependent.
 *        value  Value as a string, application dependent.
 */
CREATE TABLE batch_job_parm
        (job INTEGER      NOT NULL,
        name NVARCHAR(32) NOT NULL,
       value NTEXT        NOT NULL)
GO
GRANT INSERT, UPDATE ON batch_job_parm TO CdrPublishing
GO

/*
 * Record of a document's having been checked out.  Retained even after it has
 * been checked back in.  Note that queued requests to check a document out
 * are not represented in the database.  These are instead tracked directly by
 * the CDR process at runtime.  We may reconsider this decision if the
 * multi-threaded, single-process architecture is replaced with multiple
 * server processes (or, more likely, we may switch to using shared memory).
 *
 *           id  CDR ID of the document checked out
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
         (id INTEGER   NOT NULL,
      dt_out DATETIME  NOT NULL,
         usr INTEGER   NOT NULL,
       dt_in DATETIME      NULL,
     version INTEGER       NULL,
     comment NVARCHAR(255) NULL,
 PRIMARY KEY (id, dt_out))
GO
GRANT INSERT, UPDATE ON checkout TO CdrPublishing
GO

/*
 * Records events which occurred in a CDR client session.  Used, for
 * example, to record when users save documents locally instead of in
 * the CDR.
 *
 *     event_id  primary key for table
 *   event_time  when was the event recorded
 *   event_desc  text description of the event
 *      session  foreign key reference into the session table
 */
CREATE TABLE client_log
   (event_id INTEGER       IDENTITY PRIMARY KEY,
  event_time DATETIME      NOT NULL,
  event_desc NVARCHAR(255) NOT NULL,
     session INTEGER           NULL)
GO

/*
 * Select information for each new trial found on ClinicalTrials.gov.
 * Used for report of recent CT.gov trials (OCECDR-3877).
 *
 *         nct_id  unique ID for the trial document (primary key)
 *    trial_title  brief title (if present) otherwise official title
 *    trial_phase  phase(s) of the trial
 * first_received  when the trial first appeared in NLM's database
 */
  CREATE TABLE ctgov_trial
       (nct_id VARCHAR(11) NOT NULL PRIMARY KEY,
   trial_title NVARCHAR(1024) NOT NULL,
   trial_phase NVARCHAR(20) NULL,
first_received DATETIME NOT NULL)
GO

/*
 * ID other than the NCT ID for a clinical trial document in NLM's
 * database. Used for report of recent CT.gov trials (OCECDR-3877).
 *
 *    nct_id  unique ID for NLM's trial document (link to ctgov_trial
 *            table)
 *  position  keeps the IDs in the order in which they appear in NLM's
 *            clinical_trial document
 *  other_id  the value of the ID
 */
CREATE TABLE ctgov_trial_other_id
     (nct_id VARCHAR(11)    NOT NULL,
    position INTEGER        NOT NULL,
    other_id NVARCHAR(1024) NOT NULL,
 PRIMARY KEY (nct_id, position))
GO

/*
 * Sponsors/collaborators found in NLM clinical trial documents.
 * Used for report of recent CT.gov trials (OCECDR-3877).
 *
 *    nct_id  unique ID for NLM's trial document (link to ctgov_trial
 *            table)
 *  position  keeps the sponsors in the order in which they appear
 *            in NLM's clinical_trial document
 *   sponsor  name of the sponsor/collaborator
 */
CREATE TABLE ctgov_trial_sponsor
     (nct_id VARCHAR(11)    NOT NULL,
    position INTEGER        NOT NULL,
     sponsor NVARCHAR(1024) NOT NULL,
 PRIMARY KEY (nct_id, position))
GO

/*
 * Named and typed values used to control various run-time settings.
 *
 * Software in the CdrServer enforces a constraint that only one grp+name
 * combination is allowed to be active (i.e., inactivated is null) at a time.
 * If there is more than one, the software will throw an exception.  An
 * attempt to add one via the CdrServer software will inactivate any existing
 * active row with the same grp+name.
 *
 *           id  unique row ID.
 *          grp  identifies group this name=value belongs to, e.g. "dbconnect"
 *         name  mnemonic label for value; e.g., "timeout"
 *          val  value associated with this name; e.g., "3000"
 *      comment  optional text description of the use of this value,
 *                e.g., "Database connection timeout, in milliseconds"
 *      created  Datetime this row was created
 *  inactivated  Datetime row made inactive, not used on controlling ops
 */
CREATE TABLE ctl
         (id INTEGER         IDENTITY PRIMARY KEY,
         grp NVARCHAR(255)   NOT NULL,
        name NVARCHAR(255)   NOT NULL,
         val NVARCHAR(255)   NOT NULL,
     comment NVARCHAR(255)       NULL,
     created DATETIME        NOT NULL,
 inactivated DATETIME            NULL)
GO

/*
 * When we notify a PDQ data partner that new PDQ data is available, we
 * record that notification here. With this information, if the notification
 * job fails part-way through, we can answer the questions "which contacts
 * have we already notified for this job?".
 *
 *   email_addr  where we sent the notification
 *   notif_date  when we sent it
 */
CREATE TABLE data_partner_notification
 (email_addr NVARCHAR(64) NOT NULL,
  notif_date DATETIME     NOT NULL)
GO

/*
 * Keep track of the CDR DLL commands run for a CDR session. Used for
 * troubleshooting when XMetaL crashes or locks up. See OCECDR-4229.
 *
 *     log_id  primary key, generated by the database
 *  log_saved  when the log was posted to the database; usually (but
 *             not necessarily) at the end of the session
 *   cdr_user  account name of the user running XMetaL
 * session_id  unique string identifying the CDR session
 *   log_data  contents of the trace log
 */
CREATE TABLE dll_trace_log
     (log_id INTEGER IDENTITY PRIMARY KEY,
   log_saved DATETIME NOT NULL,
    cdr_user VARCHAR(64),
  session_id VARCHAR(64),
    log_data NTEXT NOT NULL)
GO

/*
 * Keep track of the CDR DLL commands run for a CDR session. Used for
 * troubleshooting when XMetaL crashes or locks up. See OCECDR-4229.
 * Replacement version for the previous table. Intended for a world in
 * which we no longer have a DLL.
 *
 *     log_id  primary key, generated by the database
 *  log_saved  when the log was posted to the database; usually (but
 *             not necessarily) at the end of the session
 *   cdr_user  account name of the user running XMetaL
 * session_id  unique string identifying the CDR session
 *   log_data  contents of the trace log
 */
CREATE TABLE client_trace_log
     (log_id INTEGER IDENTITY PRIMARY KEY,
   log_saved DATETIME NOT NULL,
    cdr_user VARCHAR(64),
  session_id VARCHAR(64),
    log_data NVARCHAR(MAX) COLLATE Latin1_General_100_CI_AI_SC_UTF8 NOT NULL)
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
       (doc_id INTEGER PRIMARY KEY,
       blob_id INTEGER)
GO

/*
 * Indexes for access by the blob ID.
 */
CREATE INDEX doc_blob_blob_idx ON doc_blob_usage(blob_id)
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
     comment NVARCHAR(255)   NULL)
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
      format INTEGER     NOT NULL,
     created DATETIME    NOT NULL,
  versioning CHAR        NOT NULL DEFAULT 'Y',
  xml_schema INTEGER         NULL,
 schema_date DATETIME    NOT NULL DEFAULT GETDATE(),
title_filter INTEGER         NULL,
      active CHAR        NOT NULL DEFAULT 'Y',
     comment NVARCHAR(255)   NULL)
GO

/*
 * Associates a version label with a specific version of a single document.
 *
 *        label  foreign key into the version_label table
 *     document  identifies the document which this version represents
 *          num  sequential number of the version of the document
 */
CREATE TABLE doc_version_label
      (label INTEGER NOT NULL,
    document INTEGER NOT NULL,
         num INTEGER NOT NULL,
 PRIMARY KEY (label, document))
GO

/*
 * Information used by publishing export in a separate process.
 *
 *   job_id  foreign key into the pub_proc table
 *  spec_id  unique integrer for a given job_id
 *  filters  sequence of names of filters or filter sets
 *   subdir  where the exported documents should be written
 */
CREATE TABLE export_spec
     (job_id INTEGER NOT NULL,
     spec_id INTEGER NOT NULL,
     filters NTEXT NOT NULL,
      subdir NVARCHAR(32) NULL,
 PRIMARY KEY (job_id, spec_id))
GO
GRANT INSERT, UPDATE, DELETE ON export_spec TO CdrPublishing
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
       usage INTEGER       NOT NULL,
       value NVARCHAR(356) NOT NULL,
      doc_id INTEGER           NULL,
         usr INTEGER           NULL,
    last_mod DATETIME      NOT NULL,
       bogus CHAR          NOT NULL DEFAULT 'N',
    mappable CHAR          NOT NULL DEFAULT 'Y',
CONSTRAINT external_map_unique UNIQUE (usage, value))
GO

/*
 * SQL "LIKE" patterns for CTGov Facility values that we should not
 * try to map.  If a new facility value string is encountered and
 * it matches one of these patterns, we create an external_map
 * entry for it, as usual, but we mark it as mappable = 'N'
 *
 *           id  primary key for this table (auto-generated)
 *      pattern  pattern.  Should include SQL LIKE wildcards in order
 *               to be useful.
 */
CREATE TABLE external_map_nomap_pattern
         (id INTEGER       IDENTITY PRIMARY KEY,
     pattern NVARCHAR(356) NOT NULL)
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
      map_id INTEGER       NOT NULL,
     element VARCHAR(64)   NOT NULL,
       value NVARCHAR(255) NOT NULL)
GO

/*
 * Table used to determine whether a mapping is to a document of the
 * correct type.
 *
 *     usage  foreign key into the external_map_usage table
 *  doc_type  foreign key into the doc_type table
 */
CREATE TABLE external_map_type
      (usage INTEGER NOT NULL,
    doc_type INTEGER NOT NULL,
PRIMARY KEY (usage, doc_type))
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
        name NVARCHAR(32) NOT NULL UNIQUE,
 auth_action INTEGER      NOT NULL,
     comment NVARCHAR(255)    NULL)
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
        name NVARCHAR(80)     NOT NULL UNIQUE,
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
 (filter_set INTEGER NOT NULL,
    position INTEGER NOT NULL,
      filter INTEGER     NULL,
      subset INTEGER     NULL,
  CONSTRAINT filter_set_member_pk PRIMARY KEY(filter_set, position))
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
     comment NVARCHAR(255)   NULL)
GO

/*
 * Current state of a glossary translation job.
 *
 *       doc_id  foreign key into the all_docs table
 *     state_id  foreign key into the glossary_translation_state table
 *   state_date  when the state was assigned
 *  assigned_to  foreign key into the usr table
 *     comments  optional notes/instructions, etc.
 */
CREATE TABLE glossary_translation_job
     (doc_id INTEGER  NOT NULL PRIMARY KEY,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL)
GO

/*
 * State history for glossary translation workflow.
 *
 *   history_id  primary key for this table
 *       doc_id  foreign key into the all_docs table
 *     state_id  foreign key into the glossary_translation_state table
 *   state_date  when the state was assigned
 *  assigned_to  foreign key into the usr table
 *     comments  optional notes/instructions, etc.
 */
CREATE TABLE glossary_translation_job_history
 (history_id INTEGER  IDENTITY PRIMARY KEY,
      doc_id INTEGER  NOT NULL,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL)
GO

/*
 * Valid value control table for glossary translation workflow states.
 *
 *     value_id  primary key for this table
 *   value_name  required display name for this state
 *    value_pos  required integer for ordering the states
 */
CREATE TABLE glossary_translation_state
   (value_id INTEGER       IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER       NOT NULL)
GO

/*
 * One-row table holding serialized data for the glossifier service.
 * Seeded with INSERT INTO glossifier VALUES(1, GETDATE(), ''). Refreshed
 * by nightly job running under CDR scheduler.
 *
 * TODO: soon to be retired.
 *
 *        pk  primary key, set manually to 1
 * refreshed  when the data was last refreshed
 *     terms  serialized glossary term data
 */
CREATE TABLE glossifier
         (pk INTEGER  PRIMARY KEY,
   refreshed DATETIME NOT NULL,
       terms TEXT     NOT NULL)
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
         (id INTEGER      IDENTITY PRIMARY KEY,
        name NVARCHAR(32) NOT NULL UNIQUE,
     comment NVARCHAR(255)    NULL)
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
        (grp INTEGER   NOT NULL,
      action INTEGER   NOT NULL,
    doc_type INTEGER   NOT NULL,
     comment NVARCHAR(255) NULL,
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
        (grp INTEGER   NOT NULL,
         usr INTEGER   NOT NULL,
     comment NVARCHAR(255) NULL,
 PRIMARY KEY (grp, usr))
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
         doc_id INTEGER     NOT NULL,
       fragment VARCHAR(64) NOT NULL,
    PRIMARY KEY (doc_id, fragment)
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
   link_type INTEGER     NOT NULL,
  source_doc INTEGER     NOT NULL,
 source_elem VARCHAR(64) NOT NULL,
  target_doc INTEGER         NULL,
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
     comment NVARCHAR(255)    NULL
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
      link_id INTEGER       NOT NULL,
  property_id INTEGER       NOT NULL,
        value NVARCHAR(1024)    NULL,
      comment NVARCHAR(256)     NULL
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
    source_link_type  INTEGER NOT NULL,
     target_doc_type  INTEGER NOT NULL
)
GO

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
    chk_type CHAR                NOT NULL DEFAULT 'C',
     comment NVARCHAR(255)           NULL
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
      doc_type INTEGER      NOT NULL,
       element VARCHAR(64)  NOT NULL,
       link_id INTEGER      NOT NULL,
   PRIMARY KEY (doc_type, element)
)
GO

/*
 * Used to communicate between processes for the media manifest file.
 *
 *    job_id  ID for the publishing job (but no foreign key contraint)
 *    doc_id  ID for the CDR Media document (but no foreign key constraint)
 * blob_date  when the Media document's BLOB was stored in the CDR
 * file_name  base file name (not full path) for the media file
 *     title  string for the CDR Media document's title
 */
CREATE TABLE media_manifest (
      job_id INTEGER      NOT NULL,
      doc_id INTEGER      NOT NULL,
   blob_date DATETIME     NOT NULL,
    filename NVARCHAR(32) NOT NULL,
       title NTEXT        NOT NULL,
 PRIMARY KEY (job_id, doc_id)
)
GO
GRANT INSERT, UPDATE ON media_manifest TO CdrPublishing
GO

/*
 * Current state of a media document translation job.
 *
 *       doc_id  foreign key into the all_docs table
 *     state_id  foreign key into the media_translation_state table
 *   state_date  when the state was assigned
 *  assigned_to  foreign key into the usr table
 *     comments  optional notes/instructions, etc.
 */
CREATE TABLE media_translation_job
 (english_id INTEGER  NOT NULL PRIMARY KEY,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL)
GO

/*
 * State history for media document translation workflow.
 *
 *   history_id  primary key for this table
 *       doc_id  foreign key into the all_docs table
 *     state_id  foreign key into the media_translation_state table
 *   state_date  when the state was assigned
 *  assigned_to  foreign key into the usr table
 *     comments  optional notes/instructions, etc.
 */
CREATE TABLE media_translation_job_history
 (history_id INTEGER  IDENTITY PRIMARY KEY,
  english_id INTEGER  NOT NULL,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL)
GO

/*
 * Valid value control table for media translation workflow states.
 *
 *     value_id  primary key for this table
 *   value_name  required display name for this state
 *    value_pos  required integer for ordering the states
 */
CREATE TABLE media_translation_state
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)
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
  pub_system INTEGER      NOT NULL,
  pub_subset VARCHAR(255) NOT NULL,
         usr INTEGER      NOT NULL,
  output_dir VARCHAR(255) NOT NULL,
     started DATETIME     NOT NULL,
   completed DATETIME         NULL,
      status VARCHAR(32)  NOT NULL,
    messages NTEXT            NULL,
       email VARCHAR(255)     NULL,
  "external" CHAR(1)          NULL DEFAULT 'N',
   no_output CHAR(1)      NOT NULL DEFAULT 'N')
GO
GRANT INSERT, UPDATE ON pub_proc TO CdrPublishing
GO
CREATE NONCLUSTERED INDEX pub_proc_status_completed_idx
ON pub_proc (status, completed)
INCLUDE (pub_system, output_dir, started)
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
         (id INTEGER NOT NULL PRIMARY KEY,
    pub_proc INTEGER NOT NULL,
         xml NTEXT   NOT NULL,
  force_push CHAR    NOT NULL DEFAULT 'N',
      cg_new CHAR    NOT NULL DEFAULT 'N')
GO
GRANT INSERT, UPDATE, DELETE ON pub_proc_cg TO CdrPublishing
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
         (id INTEGER     NOT NULL PRIMARY KEY,
         num INTEGER     NOT NULL,
  vendor_job INTEGER     NOT NULL,
      cg_job INTEGER     NOT NULL,
    doc_type VARCHAR(32) NOT NULL,
         xml NTEXT           NULL)
GO
GRANT INSERT, UPDATE, DELETE ON pub_proc_cg_work TO CdrPublishing
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
   (pub_proc INTEGER      NOT NULL,
      doc_id INTEGER      NOT NULL,
 doc_version INTEGER      NOT NULL,
     failure CHAR             NULL,
    messages NTEXT            NULL,
     removed CHAR(1)          NULL DEFAULT 'N',
      subdir VARCHAR(32)      NULL DEFAULT '',
  CONSTRAINT pub_proc_doc_pk  PRIMARY KEY(pub_proc, doc_id))
GO
GRANT INSERT, UPDATE ON pub_proc_doc TO CdrPublishing
GO
CREATE INDEX pub_proc_doc_failure_index ON pub_proc_doc(failure)

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
    pub_proc INTEGER      NOT NULL,
   parm_name NVARCHAR(32) NOT NULL,
  parm_value NTEXT            NULL,
  CONSTRAINT pub_proc_parm_pk PRIMARY KEY(pub_proc, id))
GO
GRANT INSERT, UPDATE ON pub_proc_parm TO CdrPublishing
GO

/*
 * Stored database queries.
 *
 *    name  required string identifying the query, used as the primary key
 *   value  the SQL query string
 */
CREATE TABLE query
       (name NVARCHAR(160) NOT NULL PRIMARY KEY,
       value NVARCHAR(MAX)     NULL)
GO
GRANT INSERT, UPDATE, DELETE, REFERENCES ON query TO CdrGuest
GO

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
     (doc_id INTEGER       NOT NULL,
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
 * Identifies elements which are to be indexed for querying.
 *
 *         path  hierarchical representation of element to be indexed;
 *               for example, 'Protocol/ProtSponsor'.
 *    term_rule  foreign key into query_term_rule table.
 */
CREATE TABLE query_term_def
       (path VARCHAR(512) NOT NULL,
   term_rule INTEGER          NULL)
GO

/*
 * Contains searchable element data for the last publishable version
 * of each document.
 *
 * The structure and meaning of the fields is identical to query_term, q.v.
 */
CREATE TABLE query_term_pub
     (doc_id INTEGER       NOT NULL,
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
         (id INTEGER        IDENTITY PRIMARY KEY,
        name NVARCHAR(32)   NOT NULL,
    rule_def NVARCHAR(2000) NOT NULL)
GO

/*
 * Flag for documents which have been marked as ready for pre-publication
 * review.
 *
 *       doc_id  foreign key into all_docs table.
 */
CREATE TABLE ready_for_review
     (doc_id INTEGER NOT NULL PRIMARY KEY)
GO

/*
 * Job run by the CDR scheduler.
 *
 *        id  primary key for the table
 *      name  required string for the job's name
 *   enabled  Boolean flag indicating whether the job should be run
 * job_class  required string in the form module_name.class_name
 *      opts  required JSON string for the job's parameter options
 *  schedule  optional string for cron-like schedule information
 */
CREATE TABLE scheduled_job (
          id UNIQUEIDENTIFIER NOT NULL PRIMARY KEY DEFAULT NEWID(),
        name NVARCHAR(512)    NOT NULL,
     enabled BIT              NOT NULL,
   job_class NVARCHAR(128)    NOT NULL,
        opts VARCHAR(1024)    NOT NULL,
    schedule VARCHAR(256)         NULL
)
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
         usr INTEGER     NOT NULL,
   initiated DATETIME    NOT NULL,
    last_act DATETIME    NOT NULL,
       ended DATETIME        NULL,
  ip_address VARCHAR(64)     NULL,
     comment NVARCHAR(255)   NULL)
GO

/**
 * Replacement for debug_log (when CDR Service was eliminated).
 *
 *  entry_id  primary key for this table
 * thread_id  ID of the thread in which this activity occurred
 *  recorded  when the activity was logged
 *   message  required description of the activity
 */
CREATE TABLE session_log
   (entry_id INTEGER  IDENTITY PRIMARY KEY,
   thread_id BIGINT   NOT NULL,
    recorded DATETIME NOT NULL,
     message NTEXT    NOT NULL)
CREATE INDEX session_log_recorded ON session_log(recorded, entry_id)
GO

/*
 * Valid values table for types of changes to summary documents.
 * Used by the translation queue management software. Valid value
 * lookup tables constructed with this structure (same column names
 * and types) can be managed using the admin web interface.
 *
 *   value_id   primary key, automatically generated
 * value_name   display name for the value
 *  value_pos   unique integer controlling the position of the value
 *              in picklists and reports
 */
CREATE TABLE summary_change_type
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)
GO

/*
 * Tracking information for the translation of an English CDR Summary
 * document into Spanish.
 *
 *   english_id  primary key (thus unique) and foreign key into all_docs
 *               table
 *     state_id  foreign key into the lookup table for translation job status
 *   state_date  when the job entered the current state
 *  change_type  foreign key into the lookup table for summary change type
 *  assigned_to  foreign key into the table for CDR users
 *     comments  optional instructions (e.g., specific version to translate)
 *     notified  record of when the assigned user was last notified about
 *               the change of the job's state; might not need this any more
 *               (requirements have been wandering all around, and are still
 *               not quite fixed in stone).
 */
CREATE TABLE summary_translation_job
 (english_id INTEGER  NOT NULL PRIMARY KEY,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 change_type INTEGER  NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL,
    notified DATETIME     NULL)
GO

/*
 * All states for the translation of the PDQ English summary documents.
 * Probably wouldn't have done this in a separate table, but the users'
 * originally requirements were for the system to discard prior states
 * for a summary translation's workflow, retaining only the latest state.
 * Then at the last minute they were puzzled that the reports didn't
 * show them any history. :-)
 *
 *   history_id  primary key, automatically generated
 *   english_id  foreign key into all_docs table
 *     state_id  foreign key into the lookup table for translation job status
 *   state_date  when the job entered the current state
 *  change_type  foreign key into the lookup table for summary change type
 *  assigned_to  foreign key into the table for CDR users
 *     comments  optional instructions (e.g., specific version to translate)
 *     notified  record of when the assigned user was last notified about
 *               the change of the job's state; might not need this any more
 *               (requirements have been wandering all around, and are still
 *               not quite fixed in stone).
 */
CREATE TABLE summary_translation_job_history
 (history_id INTEGER  IDENTITY PRIMARY KEY,
  english_id INTEGER  NOT NULL,
    state_id INTEGER  NOT NULL,
  state_date DATETIME NOT NULL,
 change_type INTEGER  NOT NULL,
 assigned_to INTEGER  NOT NULL,
    comments NTEXT        NULL,
    notified DATETIME     NULL)
GO

/*
 * Valid values table for the status of CDR document translation jobs.
 * Used by the translation queue management software. Valid value
 * lookup tables constructed with this structure (same column names
 * and types) can be managed using the admin web interface.
 *
 *   value_id   primary key, automatically generated
 * value_name   display name for the value
 *  value_pos   unique integer controlling the position of the value
 *              in picklists and reports
 */
CREATE TABLE summary_translation_state
   (value_id INTEGER IDENTITY PRIMARY KEY,
  value_name NVARCHAR(128) NOT NULL UNIQUE,
   value_pos INTEGER NOT NULL)
GO
/*
 * Files to be attached to translation job notificiation messages.
 *
 *  attachment_id  primary key, automatically generated
 *     english_id  foreign key into all_docs table
 *     file_bytes  binary content for file
 *      file_name  name of the original file
 *     registered  when the file was connected with the translation job
 */
  CREATE TABLE summary_translation_job_attachment
(attachment_id INTEGER IDENTITY PRIMARY KEY,
    english_id INTEGER NOT NULL,
    file_bytes VARBINARY(MAX) NOT NULL,
     file_name NVARCHAR(256) NOT NULL,
    registered DATETIME2)
GO

/*
 * Audio pronunciation file for a CDR glossary document.
 *
 *              id  primary key for this table
 *      zipfile_id  foreign key into the term_audio_zipfile table
 *   review_status  R=rejected, A=approved, U=unreviewed
 *          cdr_id  foreign key into the all_docs table
 *       term_name  required string for the name being pronounced
 *        language  'English' or 'Spanish'
 *   pronunciation  optional string for the phonetic representation
 *        mp3_name  string for the path in the zipfile for the mp3 file
 *     reader_note  optional comments from the creator of the mp3 file
 *   reviewer_note  optional comments from the reviewer of the mp3 file
 *     reviewer_id  foreign key into the usr table
 *     review_date  when the review was performed
 */
 CREATE TABLE term_audio_mp3
          (id INTEGER       IDENTITY PRIMARY KEY,
   zipfile_id INTEGER       NOT NULL,
review_status CHAR              NULL DEFAULT 'U',
       cdr_id INTEGER       NOT NULL,
    term_name nvarchar(256) NOT NULL,
     language VARCHAR(10)   NOT NULL,
pronunciation VARCHAR(256)      NULL,
     mp3_name VARCHAR(256)      NULL,
  reader_note NVARCHAR(2048)    NULL,
reviewer_note NVARCHAR(2048)    NULL,
  reviewer_id INTEGER           NULL,
  review_date DATETIME          NULL)
GO

/*
 * Compressed archive of audio pronunciation files for glossary documents.
 *
 *       id  primary key for this table
 * filename  required string for the base file name (no path)
 * filedate  when the file was loaded onto the CDR server
 * complete  'Y' if all of the pronunciations have been reviewed
 */
CREATE TABLE term_audio_zipfile
         (id INTEGER      IDENTITY PRIMARY KEY,
    filename VARCHAR(128) NOT NULL,
    filedate DATETIME     NOT NULL,
    complete CHAR             NULL DEFAULT 'N')
GO

/*
 * Table to store the set of parameters submitted to QC reports.
 * This table is used and populated by the program QcReports.py.
 * The QC reports are being converted into MS-Word from IE but
 * MS-Word limits the URL to be passed to 256 characters.  We
 * store the set of parameters in this table and only pass the
 * record-ID to run the QC report for MS-Word.
 *
 *        id primary key for table
 *  shortURL currently unused
 *   longURL the list of parameters passed to the report module
 *       usr the user ID running the report/storing the record
 *           currently unused
 *   created date and time when row has been created
 *  lastused currently unused
 */
CREATE TABLE url_parm_set
         (id INTEGER       IDENTITY PRIMARY KEY,
    shortURL VARCHAR(150)      NULL,
     longURL VARCHAR(2000) NOT NULL,
         usr INTEGER           NULL,
     created DATETIME      NOT NULL DEFAULT GETDATE(),
    lastused DATETIME          NULL)
GO
GRANT INSERT ON url_parm_set TO CdrGuest
GO

/*
 * Users authorized to work with the CDR.  Note that connection information is
 * not stored in the database, but is instead tracked by the server
 * application directly at runtime.
 *
 *           id  automatically generated primary key for the usr table
 *         name  unique logon name used by this user
 *     password  plain text string used to authenticate this user's identity
 *     hashedpw  use this instead of password if hashing available
 * login_failed  count of consecutive failures, reset to 0 on success
 *      created  date and time the user's account was added to the system
 *     fullname  optional string giving the user's full name
 *       office  optional identification of the office in which the user works
 *        email  optional email address; used for automated notification of
 *               system events
 *        phone  option telephone number used to contact this user
 *      expired  optional date/time this account was (or will be) deactivated
 *      comment  optional free-text description of additional characteristics
 *               for this user account
 *
 * Note:
 *   There used to be a column for passwords that has been removed for
 *   tightened security purposes.
 *     Plain text string used to authenticate this user's identity
 *     password VARCHAR(32) NOT NULL
 */
CREATE TABLE usr
         (id INTEGER     IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
    hashedpw VARBINARY(2048) NULL,
login_failed INTEGER     NOT NULL DEFAULT 0,
     created DATETIME    NOT NULL,
    fullname VARCHAR(100)    NULL,
      office VARCHAR(100)    NULL,
       email VARCHAR(128)    NULL,
       phone VARCHAR(20)     NULL,
     expired DATETIME        NULL,
     comment VARCHAR(255)    NULL)
GO

/*
 * Drug terms the user doesn't want on the refresh-from-EVS page.
 *
 * id  foreign key into the all_docs table
 */
CREATE TABLE unrefreshable_drug_term
         (id INTEGER PRIMARY KEY)
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
       (doc_id INTEGER,
   doc_version INTEGER,
       blob_id INTEGER)
GO
/*
 * Indexes for access.
 */
CREATE INDEX ver_blob_doc_idx ON version_blob_usage(doc_id, doc_version)
CREATE INDEX ver_blob_blob_idx ON version_blob_usage(blob_id)
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
     comment NVARCHAR(255)   NULL)
GO

/*
 * Information from ZIPInfo's ZIPList5 subscription service.
 *
 * See https://www.zipinfo.com/ for documentation of the fields.
 */
    CREATE TABLE zipcode
           (city VARCHAR(28) NOT NULL,
              st CHAR(2)     NOT NULL,
             zip CHAR(5)     NOT NULL,
       area_code CHAR(5)     NOT NULL,
     county_fips CHAR(5)     NOT NULL,
     county_name VARCHAR(25) NOT NULL,
       preferred CHAR(1)     NOT NULL CHECK (preferred IN ('P', 'A', 'N')),
   zip_code_type CHAR(1)     NOT NULL CHECK (zip_code_type IN ('P',
                                                               'U',
                                                               'M',
                                                               ' ')))
GO

/*
 * Backup for the zipcode table.
 */
    CREATE TABLE zipcode_backup
           (city VARCHAR(28) NOT NULL,
              st CHAR(2)     NOT NULL,
             zip CHAR(5)     NOT NULL,
       area_code CHAR(5)     NOT NULL,
     county_fips CHAR(5)     NOT NULL,
     county_name VARCHAR(25) NOT NULL,
       preferred CHAR(1)     NOT NULL CHECK (preferred IN ('P', 'A', 'N')),
   zip_code_type CHAR(1)     NOT NULL CHECK (zip_code_type IN ('P',
                                                               'U',
                                                               'M',
                                                               ' ')))
GO

/************************************************************************/
/* VIEWS WITH NO DEPENDENCIES ON OTHER VIEWS                            */
/************************************************************************/

/*
 * View of the document table containing only active documents.
 */
CREATE VIEW active_doc AS SELECT * FROM all_docs WHERE active_status = 'A'
GO

/*
 * View of the document table containing only deleted documents.
 */
CREATE VIEW deleted_doc AS SELECT * FROM all_docs WHERE active_status = 'D'
GO

/*
 * Most code in the CDR uses this view rather than the underlying table.
 */
CREATE VIEW document AS SELECT * FROM all_docs WHERE active_status != 'D'
GO
GRANT INSERT, UPDATE ON document TO CdrPublishing
GO

/*
 * View on the audit trail for actions which save a document.
 */
CREATE VIEW doc_save_action
         AS
     SELECT audit_trail.document doc_id,
            audit_trail.dt save_date,
            action.name save_action,
            audit_trail.usr save_user
       FROM audit_trail
       JOIN action
         ON action.id = audit_trail.action
      WHERE action.name IN ('ADD DOCUMENT', 'MODIFY DOCUMENT')
GO

/*
 * View which makes the siphoning off of older XML to a separate table
 * transparent to the rest of the system. See description of the
 * all_doc_versions table above.
 */
CREATE VIEW doc_version
AS
         SELECT v.id, v.num, v.dt, v.updated_dt, v.usr,
                v.val_status, v.val_date,
                v.publishable, v.doc_type, v.title,
                xml = CASE
                    WHEN v.xml IS NOT NULL THEN v.xml
                    ELSE a.xml
                END,
                comment
           FROM all_doc_versions v
LEFT OUTER JOIN cdr_archived_versions.dbo.doc_version_xml a
             ON a.id = v.id
            AND a.num = v.num
GO

/*
 * One row, the most recent one, for each actual term.
 *
 * Due to character set problems, the terms don't always exactly match,
 * but every combination of cdr_id + language should be unique.
 *
 *  max_date - the date of the last review for each unique term.
 *
 * Inner joining on this table will ensure that only the last review of a
 * term is examined.
 */
CREATE VIEW lastmp3
AS
     SELECT MAX(review_date) AS max_date, cdr_id, language
       FROM term_audio_mp3
   GROUP BY cdr_id, language
GO

/*
 * View of the usr table without the password information.
 */
CREATE VIEW open_usr AS
     SELECT id, name, created, fullname, office, email, phone, expired,
            comment
       FROM usr
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
 * Contains child-parent document ID pairs for Term document hierarchical
 * relationships.
 */
CREATE VIEW term_parents
AS
    SELECT DISTINCT doc_id AS child, int_val AS parent
               FROM query_term
              WHERE path = '/Term/TermRelationship/ParentTerm/TermId/@cdr:ref'
GO


/************************************************************************/
/* VIEWS WITH DEPENDENCIES ON OTHER VIEWS - FIRST PASS                  */
/************************************************************************/

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
    FROM document d
    JOIN doc_type t
      ON d.doc_type = t.id
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
 * Most recent publishable version for each document.
 */
CREATE VIEW latest_publishable_version
AS
      SELECT d.id,
             MAX(v.num) num
        FROM doc_version v
        JOIN document d
          ON d.id = v.id
         AND v.val_status = 'V'
         AND v.publishable = 'Y'
         AND d.active_status = 'A'
    GROUP BY d.id
GO

/*
 * Terminology tree display support.
 */
CREATE VIEW orphan_terms
AS
    SELECT DISTINCT d.title
               FROM document d
               JOIN query_term q
                 ON d.id = q.doc_id
              WHERE q.path = '/Term/TermPrimaryType'
                AND q.value <> 'glossary term'
                AND NOT EXISTS (SELECT *
                                  FROM query_term q2
                                 WHERE q2.doc_id = d.id
                                   AND q2.path = '/Term/TermRelationship'
                                               + 'ParentTerm/TermId/@cdr:ref')
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

/*
 * View of the doc_version view containing only publishable versions.
 * Command decision made by the CDR team to enforce check for val_status
 *   as part of publishability.
 * There _should_ be no invalid publishable docs, but we found some
 *   citations for which the latest publishable version is actually
 *   not valid. (Don't think it's possible for this to happen any more.)
 */
CREATE VIEW publishable_version AS
     SELECT *
       FROM doc_version
      WHERE publishable = 'Y'
        AND val_status = 'V'
GO

/*
 * View of documents which have been successfully published.
 */
CREATE VIEW published_doc
         AS SELECT pub_proc_doc.*
              FROM pub_proc_doc
              JOIN pub_event
                ON pub_event.id = pub_proc_doc.pub_proc
             WHERE pub_event.status = 'Success'
               AND (pub_proc_doc.failure IS NULL
                OR pub_proc_doc.failure <> 'Y')
GO

/*
 * View of documents which have been successfully pushed during or
 * since the latest successful full load push job.
 */
CREATE VIEW pushed_doc
AS
    SELECT d.doc_id, d.doc_version, d.pub_proc, e.pub_subset, e.completed
      FROM pub_proc_doc d
      JOIN pub_event e
        ON e.id = d.pub_proc
     WHERE e.status = 'Success'
       AND e.pub_subset LIKE 'Push%'
       AND d.failure IS NULL
       AND d.removed = 'N'
       AND e.id >= (SELECT MAX(id)
                      FROM pub_event
                     WHERE e.status = 'Success'
                       AND pub_subset = 'Push_Documents_To_Cancer.'
                                      + 'Gov_Full-Load')
GO

/*
 * View of documents which have been successfully removed from Cancer.gov
 * since the latest successful full load push job.
 */
CREATE VIEW removed_doc
AS
    SELECT d.doc_id, d.doc_version, d.pub_proc, e.pub_subset, e.completed
      FROM pub_proc_doc d
      JOIN pub_event e
        ON e.id = d.pub_proc
     WHERE e.status = 'Success'
       AND e.pub_subset LIKE 'Push%'
       AND d.failure IS NULL
       AND d.removed = 'Y'
       AND e.id > (SELECT MAX(id)
                     FROM pub_event
                    WHERE e.status = 'Success'
                      AND pub_subset = 'Push_Documents_To_Cancer.Gov_Full-Load')
GO

/*
 * Alternate (more efficient) approach to term_parents view.
 */
CREATE VIEW term_kids
AS
    SELECT DISTINCT q.int_val AS parent, q.doc_id AS child, d.title
               FROM query_term q
               JOIN document d
                 ON d.id = q.doc_id
              WHERE q.path = '/Term/TermRelationship/ParentTerm/TermId/@cdr:ref'
GO

/*
 * This version tells which nodes are leaves.
 */
CREATE VIEW term_children
AS
    SELECT DISTINCT q1.int_val AS parent,
                    q1.doc_id  AS child,
                    COUNT(DISTINCT q2.doc_id) AS num_grandchildren,
                    d.title
               FROM document d
               JOIN query_term q1
                 ON q1.doc_id = d.id
    LEFT OUTER JOIN query_term q2
                 ON q1.doc_id = q2.int_val
              WHERE q1.path = '/Term/TermRelationship/ParentTerm'
                            + '/TermId/@cdr:ref'
                AND q2.path = '/Term/TermRelationship/ParentTerm'
                            + '/TermId/@cdr:ref'
           GROUP BY q1.int_val, q1.doc_id, d.title
GO

/*
 * More terminology tree display support.
 */
CREATE VIEW TermsWithParents
AS
    SELECT d.id, d.xml
      FROM document d
      JOIN doc_type t
        ON d.doc_type = t.id
     WHERE t.name = 'Term'
       AND d.xml LIKE '%<TermParent%'
GO


/************************************************************************/
/* VIEWS WITH DEPENDENCIES ON OTHER VIEWS - SECOND PASS                 */
/************************************************************************/

/*
 * View which finds the last time a CDR document was saved.
 */
CREATE VIEW doc_last_save
         AS
     SELECT doc_id,
            MAX(save_date) AS last_save_date
       FROM doc_save_action
   GROUP BY doc_id
GO

/*
 * View used by the Pronunciation Recordings Tracking report.
 */
CREATE VIEW last_doc_publication
         AS SELECT published_doc.doc_id,
                   pub_event.pub_subset,
                   MAX(pub_event.completed) AS dt
              FROM published_doc
              JOIN pub_event
                ON pub_event.id = published_doc.pub_proc
          GROUP BY pub_event.pub_subset, published_doc.doc_id
GO

/*
 * Builds on the primary_pub_job view to get the documents for each such job.
 */
CREATE VIEW primary_pub_doc
AS
    SELECT pub_proc_doc.*, primary_pub_job.completed,
           primary_pub_job.started, primary_pub_job.output_dir
      FROM pub_proc_doc
      JOIN primary_pub_job
        ON pub_proc_doc.pub_proc = primary_pub_job.id
     WHERE pub_proc_doc.failure IS NULL
GO


/************************************************************************/
/* FOREIGN KEY CONSTRAINTS                                              */
/************************************************************************/

ALTER TABLE all_doc_versions ADD FOREIGN KEY(doc_type) REFERENCES doc_type
GO
ALTER TABLE all_doc_versions ADD FOREIGN KEY(id) REFERENCES all_docs
GO
ALTER TABLE all_doc_versions ADD FOREIGN KEY(usr) REFERENCES usr
GO
ALTER TABLE all_doc_versions ADD FOREIGN KEY(val_status) REFERENCES doc_status
GO

ALTER TABLE all_docs ADD FOREIGN KEY(active_status) REFERENCES active_status
GO
ALTER TABLE all_docs ADD FOREIGN KEY(doc_type) REFERENCES doc_type
GO
ALTER TABLE all_docs ADD FOREIGN KEY(val_status) REFERENCES doc_status
GO

ALTER TABLE audit_trail ADD FOREIGN KEY(action) REFERENCES action
GO
ALTER TABLE audit_trail ADD FOREIGN KEY(document) REFERENCES all_docs
GO
ALTER TABLE audit_trail ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE audit_trail_added_action ADD FOREIGN KEY(action) REFERENCES action
GO
ALTER TABLE audit_trail_added_action ADD FOREIGN KEY(document)
 REFERENCES all_docs
GO
ALTER TABLE audit_trail_added_action ADD FOREIGN KEY(document, dt)
 REFERENCES audit_trail
GO

ALTER TABLE batch_job_parm ADD FOREIGN KEY(job) REFERENCES batch_job
GO

ALTER TABLE checkout ADD FOREIGN KEY(id) REFERENCES all_docs
GO
ALTER TABLE checkout ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE client_log ADD FOREIGN KEY(session) REFERENCES session
GO

ALTER TABLE ctgov_trial_other_id ADD FOREIGN KEY(nct_id) REFERENCES ctgov_trial
GO

ALTER TABLE ctgov_trial_sponsor ADD FOREIGN KEY(nct_id) REFERENCES ctgov_trial
GO

ALTER TABLE doc_blob_usage ADD FOREIGN KEY(blob_id) REFERENCES doc_blob
GO
ALTER TABLE doc_blob_usage ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO

ALTER TABLE doc_type ADD FOREIGN KEY(format) REFERENCES format
GO
ALTER TABLE doc_type ADD FOREIGN KEY(title_filter) REFERENCES all_docs
GO
ALTER TABLE doc_type ADD FOREIGN KEY(xml_schema) REFERENCES all_docs
GO

ALTER TABLE doc_version_label ADD FOREIGN KEY(label) REFERENCES version_label
GO
ALTER TABLE doc_version_label ADD FOREIGN KEY(document, num)
 REFERENCES all_doc_versions
GO

ALTER TABLE external_map ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO
ALTER TABLE external_map ADD FOREIGN KEY(usage) REFERENCES external_map_usage
GO
ALTER TABLE external_map ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE external_map_rule ADD FOREIGN KEY(map_id) REFERENCES external_map
GO

ALTER TABLE external_map_type ADD FOREIGN KEY(doc_type) REFERENCES doc_type
GO
ALTER TABLE external_map_type ADD FOREIGN KEY(usage)
 REFERENCES external_map_usage
GO

ALTER TABLE external_map_usage ADD FOREIGN KEY(auth_action) REFERENCES action
GO

ALTER TABLE filter_set_member ADD FOREIGN KEY(filter) REFERENCES all_docs
GO
ALTER TABLE filter_set_member ADD FOREIGN KEY(filter_set) REFERENCES filter_set
GO
ALTER TABLE filter_set_member ADD FOREIGN KEY(subset) REFERENCES filter_set
GO

ALTER TABLE glossary_translation_job ADD FOREIGN KEY(assigned_to) REFERENCES usr
GO
ALTER TABLE glossary_translation_job ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO
ALTER TABLE glossary_translation_job ADD FOREIGN KEY(state_id)
 REFERENCES glossary_translation_state
GO

ALTER TABLE glossary_translation_job_history ADD FOREIGN KEY(assigned_to)
 REFERENCES usr
GO
ALTER TABLE glossary_translation_job_history ADD FOREIGN KEY(doc_id)
 REFERENCES all_docs
GO
ALTER TABLE glossary_translation_job_history ADD FOREIGN KEY(state_id)
 REFERENCES glossary_translation_state
GO

ALTER TABLE grp_action ADD FOREIGN KEY(action) REFERENCES action
GO
ALTER TABLE grp_action ADD FOREIGN KEY(doc_type) REFERENCES doc_type
GO
ALTER TABLE grp_action ADD FOREIGN KEY(grp) REFERENCES grp
GO

ALTER TABLE grp_usr ADD FOREIGN KEY(grp) REFERENCES grp
GO
ALTER TABLE grp_usr ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE link_fragment ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO

ALTER TABLE link_net ADD FOREIGN KEY(link_type) REFERENCES link_type
GO
ALTER TABLE link_net ADD FOREIGN KEY(source_doc) REFERENCES all_docs
GO
ALTER TABLE link_net ADD FOREIGN KEY(target_doc) REFERENCES all_docs
GO

ALTER TABLE link_properties ADD FOREIGN KEY(link_id) REFERENCES link_type
GO
ALTER TABLE link_properties ADD FOREIGN KEY(property_id)
 REFERENCES link_prop_type
GO

ALTER TABLE link_target ADD FOREIGN KEY(source_link_type) REFERENCES link_type
GO
ALTER TABLE link_target ADD FOREIGN KEY(target_doc_type) REFERENCES doc_type
GO

ALTER TABLE link_xml ADD FOREIGN KEY(doc_type) REFERENCES doc_type
GO
ALTER TABLE link_xml ADD FOREIGN KEY(link_id) REFERENCES link_type
GO

ALTER TABLE media_translation_job ADD FOREIGN KEY(assigned_to) REFERENCES usr
GO
ALTER TABLE media_translation_job ADD FOREIGN KEY(english_id)
 REFERENCES all_docs
GO
ALTER TABLE media_translation_job ADD FOREIGN KEY(state_id)
 REFERENCES media_translation_state
GO

ALTER TABLE media_translation_job_history ADD FOREIGN KEY(assigned_to)
 REFERENCES usr
GO
ALTER TABLE media_translation_job_history ADD FOREIGN KEY(english_id)
 REFERENCES all_docs
GO
ALTER TABLE media_translation_job_history ADD FOREIGN KEY(state_id)
 REFERENCES media_translation_state
GO

ALTER TABLE pub_proc ADD FOREIGN KEY(pub_system) REFERENCES all_docs
GO
ALTER TABLE pub_proc ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE pub_proc_cg ADD FOREIGN KEY(id) REFERENCES all_docs
GO
ALTER TABLE pub_proc_cg ADD FOREIGN KEY(pub_proc) REFERENCES pub_proc
GO

ALTER TABLE pub_proc_cg_work ADD FOREIGN KEY(cg_job) REFERENCES pub_proc
GO
ALTER TABLE pub_proc_cg_work ADD FOREIGN KEY(id) REFERENCES all_docs
GO
ALTER TABLE pub_proc_cg_work ADD FOREIGN KEY(vendor_job) REFERENCES pub_proc
GO

ALTER TABLE pub_proc_doc ADD FOREIGN KEY(doc_id, doc_version)
 REFERENCES all_doc_versions
GO
ALTER TABLE pub_proc_doc ADD FOREIGN KEY(pub_proc) REFERENCES pub_proc
GO

ALTER TABLE pub_proc_parm ADD FOREIGN KEY(pub_proc) REFERENCES pub_proc
GO

ALTER TABLE query_term ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO

ALTER TABLE query_term_def ADD FOREIGN KEY(term_rule) REFERENCES query_term_rule
GO

ALTER TABLE query_term_pub ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO

ALTER TABLE ready_for_review ADD FOREIGN KEY(doc_id) REFERENCES all_docs
GO

ALTER TABLE session ADD FOREIGN KEY(usr) REFERENCES usr
GO

ALTER TABLE summary_translation_job ADD FOREIGN KEY(assigned_to) REFERENCES usr
GO
ALTER TABLE summary_translation_job ADD FOREIGN KEY(english_id)
 REFERENCES all_docs
GO
ALTER TABLE summary_translation_job ADD FOREIGN KEY(change_type)
 REFERENCES summary_change_type
GO
ALTER TABLE summary_translation_job ADD FOREIGN KEY(state_id)
 REFERENCES summary_translation_state
GO

ALTER TABLE summary_translation_job_history ADD FOREIGN KEY(assigned_to)
 REFERENCES usr
GO
ALTER TABLE summary_translation_job_history ADD FOREIGN KEY(change_type)
 REFERENCES summary_change_type
GO
ALTER TABLE summary_translation_job_history ADD FOREIGN KEY(english_id)
 REFERENCES all_docs
GO
ALTER TABLE summary_translation_job_history ADD FOREIGN KEY(state_id)
 REFERENCES summary_translation_state
GO
ALTER TABLE summary_translation_job_attachment ADD FOREIGN KEY(english_id)
 REFERENCES all_docs
GO

ALTER TABLE term_audio_mp3 ADD FOREIGN KEY(cdr_id) REFERENCES all_docs
GO
ALTER TABLE term_audio_mp3 ADD FOREIGN KEY(reviewer_id) REFERENCES usr
GO
ALTER TABLE term_audio_mp3 ADD FOREIGN KEY(zipfile_id)
 REFERENCES term_audio_zipfile
GO

ALTER TABLE unrefreshable_drug_term ADD FOREIGN KEY(id) REFERENCES all_docs
GO

ALTER TABLE version_blob_usage ADD FOREIGN KEY(blob_id) REFERENCES doc_blob
GO
ALTER TABLE version_blob_usage ADD FOREIGN KEY(doc_id, doc_version)
 REFERENCES all_doc_versions
GO
