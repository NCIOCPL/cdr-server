/*
 * $Id$
 *
 * DBMS tables for the ICIC Central Database Repository
 *
 * BZIssue::4925
 * BZIssue::4924 - Modify Summary Date Last Modified Report
 */

/*
 * Do this part by hand.  It *has* to succeed!
 * USE master
 * GO
 * DROP DATABASE cdr
 * GO
 * CREATE DATABASE cdr
 * GO
 * DROP DATABASE cdr_archived_versions
 * GO
 * CREATE DATABASE cdr_archived_versions
 * GO
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
 * View of the doc_version table containing only publishable versions.
 * Command decision made by the CDR team to enforce check for val_status
 *   as part of publishability.
 * There _should_ be no invalid publishable docs, but we found some
 *   citations for which the latest publishable version is actually
 *   not valid.
 */
CREATE VIEW publishable_version AS
     SELECT * 
       FROM doc_version 
      WHERE publishable = 'Y'
        AND val_status = 'V'
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
   (document INTEGER      NOT NULL REFERENCES all_docs,
          dt DATETIME     NOT NULL,
      action INTEGER      NOT NULL REFERENCES action,
 CONSTRAINT audit_added_pk PRIMARY KEY (document, dt, action),
 CONSTRAINT audit_added_fk FOREIGN KEY (document, dt) REFERENCES audit_trail)
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
             xml NTEXT            NULL,
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


/*
 * Control PermaTargId attributes used to uniquely identify sections
 *  of a document with an unchanging ID that non-CDR web pages can use
 *  to "deep link" to that section, i.e., to link to an anchor inside
 *  the document, not just to the top level.
 * We track these here to insure that PermaTargIds are unique across
 *  the entire database and are never re-used.
 * PermaTargIds should never be used for linking from one CDR document to
 *  another.  Use cdr:id for that.  These are only for links from non-CDR
 *  documents to CDR documents.  The validation criteria for the two kinds
 *  of IDs are different.
 *
 *       targ_id    PermaTargID attribute, unique.
 *        doc_id    ID of the document containing the PermaTargId.
 *     dt_active    Datetime targ_id created.
 *    dt_deleted    Datetime targ_id marked as deleted, never re-use
 *                  If null, permatarg is active.
 */
CREATE TABLE link_permatarg (
         targ_id    INTEGER IDENTITY PRIMARY KEY,
          doc_id    INTEGER NOT NULL REFERENCES all_docs,
       dt_active    DATETIME NOT NULL DEFAULT GETDATE(),
      dt_deleted    DATETIME NULL
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
 * View of documents which have been successfully pushed during or
 * since the latest successful full load push job.
 */
CREATE VIEW pushed_doc
         AS SELECT d.doc_id, d.doc_version, d.pub_proc, e.pub_subset,
                   e.completed
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
                               AND pub_subset =
                                      'Push_Documents_To_Cancer.Gov_Full-Load')
GO

/*
 * View of documents which have been successfully removed from Cancer.gov
 * since the latest successful full load push job.
 */
CREATE VIEW removed_doc
         AS SELECT d.doc_id, d.doc_version, d.pub_proc, e.pub_subset,
                   e.completed
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
                              AND pub_subset =
                                      'Push_Documents_To_Cancer.Gov_Full-Load')
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
  force_push CHAR    NOT NULL DEFAULT 'N',
      cg_new CHAR    NOT NULL DEFAULT 'N')
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
 *         nlm_id  uniquely identifies the document within CT.gov
 *          title  official title, if present; otherwise brief title
 *            xml  unmodified XML document as downloaded from NLM
 *     downloaded  date/time of download
 *    disposition  foreign key into ctgov_disposition table
 *             dt  date/time of most recent disposition change
 *       verified  date/time document was last verified at NLM
 *        changed  date/time document was last changed at NLM
 *         cdr_id  document ID in CDR (if imported)
 *        dropped  flag indicating that NLM is no longer exporting this trial
 * reason_dropped  used to record results of research into why NLM stopped
 *                 sending a trial; when this column has been populated
 *                 the CT.gov download report will suppress reporting of
 *                 the drop of the trial
 *          force  flag for trials we want even if NLM doesn't code them
 *                 in such a way that they would be picked up by our
 *                 basic query
 *        comment  user comments, if any
 *          phase  captures the phase of the trial for reporting
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
 reason_dropped VARCHAR(512)   NULL,
       force CHAR          NOT NULL DEFAULT 'N',
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
 *  transferred  'Y' if the trial is being converted from an InScopeProtocol
                 to a CTGovProtocol; otherwise 'N'
 */
CREATE TABLE ctgov_import_event
        (job INTEGER     NOT NULL REFERENCES ctgov_import_job,
      nlm_id VARCHAR(16) NOT NULL REFERENCES ctgov_import,
      locked CHAR(1)     NOT NULL DEFAULT 'N',
         new CHAR(1)         NULL,
needs_review CHAR(1)         NULL,
 pub_version CHAR(1)         NULL,
 transferred CHAR            NULL,
  CONSTRAINT ctgov_import_event_pk PRIMARY KEY(job, nlm_id))
GO

/*
 * Remembers statistics from download jobs for trials from ClinicalTrials.gov.
 *
 *           id  primary key
 *           dt  date/time of download job
 * total_trials  number of cancer trials that meet query criteria
 *   new_trials  new active and approved-not yet active trials
 *  transferred  Count of trials for which ownership tranferred from PDQ
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
 transferred INTEGER  NOT NULL,
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
     session INTEGER           NULL REFERENCES session)
GO

/*
 * Table to store the set of parameters submitted to QC reports.
 * This table us used and populated by the program QcReports.py.
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
 * View which finds the last time a CDR document was saved.
 */
CREATE VIEW doc_last_save
         AS
     SELECT doc_id, 
            MAX(save_date) last_save_date
       FROM doc_save_action
   GROUP BY doc_id
GO

/*
 * Outcome for each CTRP trial document during a download job.
 *
 *     disp_id  primary key, automatically generated
 *   disp_name  string representing the disposition
 *     comment  optional elaboration on the semantics of the value
 */
CREATE TABLE ctrp_download_disposition
    (disp_id INTEGER     IDENTITY PRIMARY KEY,
   disp_name VARCHAR(32) NOT NULL UNIQUE,
     comment NTEXT           NULL)

/*
 * Outcome for each CTRP trial document during an import job.
 *
 *     disp_id  primary key, automatically generated
 *   disp_name  string representing the disposition
 *     comment  optional elaboration on the semantics of the value
 */
CREATE TABLE ctrp_import_disposition
    (disp_id INTEGER     IDENTITY PRIMARY KEY,
   disp_name VARCHAR(32) NOT NULL UNIQUE,
     comment NTEXT           NULL)

/* Populate the CTRP disposition tables */
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('new')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('changed')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('unchanged')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('failure')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('out of scope')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('needs nct id')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('rejected')
INSERT INTO ctrp_download_disposition (disp_name) VALUES ('unmatched')
INSERT INTO ctrp_import_disposition (disp_name) VALUES ('not yet reviewed')
INSERT INTO ctrp_import_disposition  (disp_name) VALUES ('import requested')
INSERT INTO ctrp_import_disposition (disp_name) VALUES ('imported')
INSERT INTO ctrp_import_disposition (disp_name) VALUES ('rejected')

/*
 * One row in the this table for each download of a set of trial
 * documents from CTRP.
 *
 *       job_id  primary key, automatically generated
 *   downloaded  date/time the set was retrieved from CTRP
 * job_filename  name of the file for the set in the form
 *               CTRP-TRIALS-YYYY-MM-DD.zip
 *      job_url  url used to retrieve the archive file for the set from CTRP
 */
CREATE TABLE ctrp_download_job
     (job_id INTEGER     IDENTITY PRIMARY KEY,
  downloaded DATETIME    NOT NULL,
job_filename VARCHAR(80) NOT NULL,
     job_url VARCHAR(256)    NULL)

/*
 * One row for each trial document found in each set of clinical trial
 * documents retrieved from CTRP.
 *
 *      job_id  foreign key into the ctrp_download_job table
 *     ctrp_id  unique identifier of the trial document in the CTRP system
 * disposition  foreign key into the ctrp_download_disposition table
 *     comment  optional notes on the trial's processing
 */
CREATE TABLE ctrp_download
     (job_id INTEGER     NOT NULL REFERENCES ctrp_download_job,
     ctrp_id VARCHAR(16) NOT NULL,
 disposition INTEGER     NOT NULL REFERENCES ctrp_download_disposition,
     comment NTEXT           NULL,
 PRIMARY KEY (job_id, ctrp_id))

/*
 * One row for each CTRP trial document whose site information we
 * need to import into the matching CTGovProtocol document.
 *
 *     ctrp_id  unique identifier of the trial document in the CTRP system
 * disposition  foreign key into the ctrp_import_disposition table
 *      cdr_id  foreign key into the CDR all_docs table; represents
 *              the document for the CTGovProtocol into which the site
 *              information from the CTRP document will be imported;
 *              we might want to prohibit NULLs in this column after
 *              the dust settles; over time the requirements for this
 *              import software have varied wildly, so constraints like
 *              this have been hard to nail down
 *      nct_id  unique identifier of the trial in NLM's clinicaltrials.gov
 *              system; not clear at this point how this will be used
 *              (another point at which the requirements have been
 *              extremely volatile), but I'm recording it
 *     doc_xml  the serialized representation of the document received
 *              from CTRP; will be NULL in cases where CIAT has provided
 *              us with the mapping between the CDR ID and the CTRP ID
 *              for a trial, but we haven't yet pulled the document
 *              down from CTRP in the download software; we may also
 *              have situations in which CIAT provides us with the
 *              mapping relationship, and we have retrieved the XML
 *              document from CTRP, but the CDR ID still represents
 *              an InScopeProtocol document instead of a CTGovProtocol
 *              document: in that case the import software will skip
 *              over the trial until the InScopeProtocol document has
 *              been converted to a CTGovProtocol document (after
 *              "ownership" of the document has been transferred)
 *     comment  optional notes on the processing of this trial
 */
CREATE TABLE ctrp_import
    (ctrp_id VARCHAR(16) NOT NULL PRIMARY KEY,
 disposition INTEGER     NOT NULL REFERENCES ctrp_import_disposition,
      cdr_id INTEGER         NULL REFERENCES all_docs,
      nct_id VARCHAR(16)     NULL,
     doc_xml NTEXT           NULL,
     comment NTEXT           NULL)

/*
 * One row in the this table for each job to sweep through the queue of
 * site information to be import from CTRP documents into the corresponding
 * CTGovProtocol documents and perform those imports.
 *
 *       job_id  primary key, automatically generated
 *     imported  date/time the import job was run
 */
CREATE TABLE ctrp_import_job
     (job_id INTEGER     IDENTITY PRIMARY KEY,
    imported DATETIME    NOT NULL)

/*
 * Record of each attempt to import site information from the trial's
 * CTRP document into the corresponding CTGovProtocol document.
 *
 *       job_id  foreign key into the ctrp_import_job table
 *      ctrp_id  foreign key into the ctrp_import table
 *       locked  flag (Y|N) indicating that the import failed because
 *               the CTGovProtocol document was checked out by one of
 *               the users
 * mapping_gaps  flag (Y|N) indicating that the import failed because
 *               we were unable to establish required links to CDR
 *               organization, country, or political subunit documents.
 */
CREATE TABLE ctrp_import_event
     (job_id INTEGER     NOT NULL REFERENCES ctrp_import_job,
     ctrp_id VARCHAR(16) NOT NULL REFERENCES ctrp_import,
      locked CHAR        NOT NULL DEFAULT 'N',
mapping_gaps CHAR        NOT NULL DEFAULT 'N',
 PRIMARY KEY (job_id, ctrp_id))
