/*
 * $Id: tables.sql,v 1.8 2000-04-14 15:55:14 bkline Exp $
 *
 * DBMS tables for the ICIC Central Database Repository
 *
 * $Log: not supported by cvs2svn $
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
 * Common data elements (requirement 2.1.6).  All data tags beginning with
 * 'Z' must appear in this table, effectively creating a reserved namespace
 * for controlled data elements used throughout the system, to ensure that
 * the instances of the elements can be found.
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
        (tag VARCHAR(32) PRIMARY KEY,
 description VARCHAR(255) NOT NULL,
         dtd NVARCHAR(255) NULL)

/* 
 * Valid categories for control values.
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
     comment VARCHAR(255) NULL)

/* 
 * Named and typed values used to control various run-time settings.
 *
 *          grp  identifies which control group this value belongs to
 *         name  mnemonic label for value; e.g., US
 *          val  value associated with this name; e.g., United States
 *      comment  optional text description of the use of this value
 */
CREATE TABLE ctl
        (grp INTEGER NOT NULL REFERENCES ctl_grp,
        name VARCHAR(32) NOT NULL,
         val NVARCHAR(255) NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (grp, name))

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
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
    password VARCHAR(32) NOT NULL,
     created DATETIME NOT NULL,
    fullname VARCHAR(100) NULL,
      office VARCHAR(100) NULL,
       email VARCHAR(128) NULL,
       phone VARCHAR(20) NULL,
     expired DATETIME NULL,
     comment VARCHAR(255) NULL)

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
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
         usr INTEGER NOT NULL REFERENCES usr,
   initiated DATETIME NOT NULL,
    last_act DATETIME NOT NULL,
       ended DATETIME NULL,
     comment VARCHAR(255) NULL)

/* 
 * Tells whether the document type is in XML, or in some other format, such
 * as Microsoft Word 95, JPEG, HTML, PNG, etc.
 *
 *           id  automatically generated primary key for the format table
 *         name  the name of the format displayed to the user; e.g., HTML
 *      comment  optional free-text description of this format
 */
CREATE TABLE format
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
     comment VARCHAR(255) NULL)
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
 *   xml_schema  XML schema which identifies requirements of the elements
 *               of documents of this type; required even for "unstrucutred"
 *               document types
 *      comment  optional free-text description of additional characteristics
 *               of documents of this type
 */
CREATE TABLE doc_type
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
      format INTEGER NOT NULL REFERENCES format,
     created DATETIME NOT NULL,
  versioning CHAR NOT NULL DEFAULT 'Y',
         dtd NTEXT NOT NULL,
  xml_schema NTEXT NOT NULL,
     comment VARCHAR(255) NULL)

/* 
 * Functions (a.k.a "actions") which can be performed on documents.
 *
 *           id  automatically generated primary key for the action table
 *         name  display name used to identify the action for the user;
 *               e.g., ADD DOCUMENT
 *      comment  optional free-text description of additional characteristics
 *               for this function
 */
CREATE TABLE action
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
     comment VARCHAR(255) NULL)

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
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
     comment VARCHAR(255) NULL)

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
         (id CHAR PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
     comment VARCHAR(255) NULL)

/* 
 * Unit of storage managed by the Central Database Repository.  There will 
 * be optionally one foreign key reference to this table from either the 
 * url table or the doc_blob table, but not both.
 *
 *           id  automatically generated primary key for the document table
 *      created  date/time the document was first added to the repository
 *      creator  identification of user account responsible for adding the
 *               document to the repository
 *   val_status  foreign key reference into the doc_status table
 *     val_date  date validation processing was last performed on document
 *     approved  approved for publication/release ('Y' or 'N')
 *     doc_type  foreign key reference into the doc_type table
 *        title  required string containing title for document; TBD is
 *               whether titles will be required to be unique, if only by
 *               artificially adding some distinguishing suffix when necessary
 *     modified  date/time the document's data was last changed
 *     modifier  user responsible for last change to document
 *          xml  for structured documents, this is the data for the document;
 *               for unstructured documents this contains tagged textual
 *               information associated with the document (for example, the
 *               standard caption for an illustration)
 *      comment  optional free-text description of additional characteristics
 *               of the document
 */
CREATE TABLE document
         (id INTEGER IDENTITY PRIMARY KEY,
     created DATETIME NOT NULL,
     creator INTEGER NOT NULL REFERENCES usr,
  val_status CHAR NOT NULL DEFAULT 'U' REFERENCES doc_status,
    val_date DATETIME NULL,
    approved CHAR NOT NULL DEFAULT 'N',
    doc_type INTEGER NOT NULL REFERENCES doc_type,
       title VARCHAR(255) NOT NULL,
    modified DATETIME NULL,
    modifier INTEGER NULL REFERENCES usr,
         xml NTEXT NOT NULL,
     comment VARCHAR(255) NULL)

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
 *      comment  optional free-text notes about the checked-out status of the
 *               document, or the purpose for which the document was checked
 *               out
 */
CREATE TABLE checkout
         (id INTEGER NOT NULL REFERENCES document,
      dt_out DATETIME NOT NULL,
         usr INTEGER NOT NULL REFERENCES usr,
       dt_in DATETIME NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (id, dt_out))

/* 
 * Location of document data stored outside the database.
 *
 *           id  identification of the document to which this data belongs
 *          url  location of the document's data
 */
CREATE TABLE url
         (id INTEGER NOT NULL REFERENCES document,
         url VARCHAR(128),
 PRIMARY KEY (id))

/* 
 * Non-XML data for document. 
 *
 *           id  identification of the document to which this data belongs
 *         data  binary image of the document's unstructured data
 */
CREATE TABLE doc_blob
         (id INTEGER NOT NULL REFERENCES document,
        data IMAGE NOT NULL,
 PRIMARY KEY (id))

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
 *           id  identification of the document to which this attribute
 *               pertains
 *         name  name of the type of this document attribute; for example,
 *               'AUDIENCE'
 *          num  occurrence number for this instance of this attribute type
 *               for this document; convention starts numbering with 1
 *          val  value for this attribute instance; e.g., 'Technical'
 *      comment  optional free-text notes concerning this attribute value
 */
CREATE TABLE attr
         (id INTEGER NOT NULL REFERENCES document,
        name VARCHAR(32) NOT NULL,
         num INTEGER NOT NULL,
         val NVARCHAR(255) NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (id, name, num))

/* 
 * Support for external document ID categories; e.g., MEDLINE. 
 *
 *     category  automatically generated primary key for the id_category table
 *         name  display name for the category of identifier; e.g. 'MEDLINE'
 *      comment  optional free-text explanation of the usage/characteristics
 *               of this category of external identifier
 */
CREATE TABLE id_category
   (category INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL,
     comment VARCHAR(255) NULL)

/* 
 * Document identifiers other than our own unique IDs. 
 *
 *     document  identifies the document to which this external identifier
 *               applies
 *       id_cat  category of this external identifier
 *           id  value of the external identifier; e.g., '99446448'
 */
CREATE TABLE external_id
   (document INTEGER NOT NULL REFERENCES document,
      id_cat INTEGER NOT NULL REFERENCES id_category,
          id VARCHAR(32) NOT NULL,
 PRIMARY KEY (id_cat, id))

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
   (document INTEGER NOT NULL REFERENCES document,
          dt DATETIME NOT NULL,
         usr INTEGER NOT NULL REFERENCES usr,
      action INTEGER NOT NULL REFERENCES action,
     program VARCHAR(32) NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (document, dt))

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
 *           dt  date/time the modifications were stored in the repository
 *         data  compressed version of the deltas between this version of the
 *               document and the previous version
 *      comment  optional free-form text description of the characteristics of 
 *               this version of the document
 */
CREATE TABLE doc_version
   (document INTEGER NOT NULL REFERENCES document,
         num INTEGER NOT NULL,
          dt DATETIME NOT NULL,
        data IMAGE NULL,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (document, num))

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
         (id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) NOT NULL UNIQUE,
     comment VARCHAR(255) NULL)

/*
 * Associates a version label with a specific version of a single document.
 *
 *        label  foreign key into the version_label table
 *     document  identifies the document which this version represents
 *          num  sequential number of the version of the document
 */
CREATE TABLE doc_version_label

      (label INTEGER NOT NULL REFERENCES version_label,
    document INTEGER NOT NULL REFERENCES document,
         num INTEGER NOT NULL,
 PRIMARY KEY (label, document))
  
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
        (grp INTEGER NOT NULL REFERENCES grp,
      action INTEGER NOT NULL REFERENCES action,
    doc_type INTEGER NOT NULL REFERENCES doc_type,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (grp, action, doc_type))

/* 
 * Membership of groups.
 *
 *          grp  group to which this user account has been assigned
 *          usr  identification of user account assigned to this group
 *      comment  optional free-text notes about this user's membership
 *               in this group
 */
CREATE TABLE grp_usr
        (grp INTEGER NOT NULL REFERENCES grp,
         usr INTEGER NOT NULL REFERENCES usr,
     comment VARCHAR(255) NULL,
 PRIMARY KEY (grp, usr))


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
 *     id       unique id used in other tables.
 *     name     human readable name
 *     comment  optional free text notes
 */
CREATE TABLE link_type (
          id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) UNIQUE,
     comment VARCHAR(255) NULL,
)

/*
 * Link source control.
 * Checked by link validation software to determine whether 
 *  a particular field is allowed to contain a particular link type.
 * A particular field may only contain one link type.  Otherwise
 *  the presence of a field with an href would not be enough to
 *  determine the type of link represented.
 *
 *     doc_type foreign key references doc_type table
 *     element  element in doc_type which may contain one of these links.
 *     link_id  foreign key references link_type table.
 */
CREATE TABLE link_xml (
      doc_type INTEGER NOT NULL REFERENCES doc_type,
       element VARCHAR(32) NOT NULL UNIQUE,
       link_id INTEGER NOT NULL REFERENCES link_type,
   PRIMARY KEY (doc_type, element)
)

/*
 * Types of properties of links.
 * A list of properties which may appear in the link_prop table.
 *
 *     id      unique identifier of property.
 *     name    human readable property name.
 *     comment free text comments.
 */
CREATE TABLE link_prop_type (
          id INTEGER IDENTITY PRIMARY KEY,
        name VARCHAR(32) UNIQUE,
     comment VARCHAR(256) NULL
)

/*
 * Properties of links.
 * Checked by link validation software to determine whether 
 *  any given link or link set has the specified properties.
 *
 *     link_id  identifier for a type of link.
 *     property identifier for a property type
 *     value    a valid value of this property for this link type
 *
 * Some example properties are:
 *   Allowed source document type
 *   Allowed source element type
 *   Allowed target document type
 *   Link type is transitive
 *   Link type is reflexive
 *   etc.
 */
CREATE TABLE link_prop (
      link_id INTEGER NOT NULL REFERENCES link_type,
     property INTEGER NOT NULL REFERENCES link_prop_type,
        value VARCHAR(64),
  PRIMARY KEY (link_id, property)
)

/*
 * Link network.
 * A database modelling all of the links found in the xml of all CDR
 *  documents.  It allows us to find all documents linked to any given
 *  document, either as source or target, without having to examine 
 *  the documents themselves.
 * If target_fmt is not CDR xml, then url is used and target_doc,
 *  target_doctype and target_frag are all null
 *
 *     link_type      an identifier for the type of link, e.g., author, etc.
 *     val_status     P(assed validation) F(ailed) or N(ot validated).
 *     source_doc     doc id containing a link element.
 *     source_doctype doc type of source_doc
 *     source_elem    element in source doc containing a link.
 *     target_doc     doc id linked to by source (null if url used)
 *     target_doctype doc type of target_doc
 *     target_fmt     format (xml, html, etc.) of target_doc
 *     target_frag    fragment id linked to, if any.
 *     url            alternative to target id + fragment for non CDR targets.
 *     val_time       date/time err_count set.
 */
CREATE TABLE link_net (
          link_type INTEGER NOT NULL REFERENCES link_type,
         val_status CHAR NOT NULL CHECK (val_status IN ('P', 'F', 'N')),
         source_doc INTEGER NOT NULL REFERENCES document,
     source_doctype INTEGER NOT NULL REFERENCES doc_type,
        source_elem VARCHAR(32) NOT NULL,
         target_doc INTEGER NULL REFERENCES document,
     target_doctype INTEGER REFERENCES doc_type NULL,
         target_fmt INTEGER NOT NULL REFERENCES format,
        target_frag VARCHAR(32) NULL,
                url VARCHAR(256) NULL,
           val_time DATETIME
)

/*
 * Link targets found in the system.
 * An entry is created here for each document added to the link database.
 * If the document has any "id" attributes, additional entries are made
 *  for each doc_id + element + fragment.
 * This table allows us to quickly determine whether an attempt to link
 *  to a document or fragment within a document will succeed.
 *
 *     doc_id    id of doc containing fragment
 *     elem      element tag for element containing fragment.
 *     fragment  value of id attribute in element.
 */
CREATE TABLE link_fragment (
         doc_id INTEGER NOT NULL REFERENCES document,
           elem VARCHAR(32),
       fragment VARCHAR(32),
    PRIMARY KEY (doc_id, elem)
)

/*************************************************************
 *      End link related tables
 *************************************************************/
