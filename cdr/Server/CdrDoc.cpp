/**
 * CdrDoc.cpp
 *
 * Implementation of adding or replacing a document in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.cpp,v 1.44 2002-08-01 18:44:22 bkline Exp $
 *
 */

// Eliminate annoying MS warnings about MS problems.
#pragma warning(disable : 4786)

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDbConnection.h"
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrVersion.h"
#include "CdrFilter.h"
#include "CdrDoc.h"
#include "CdrGetDoc.h"
#include "CdrLink.h"
#include "CdrValidateDoc.h"
#include "CdrXsd.h"

#include <set>

// XXXX For debugging
#include <iostream>
//#define CDR_TIMINGS 1 // Uncomment line to activate timing code.
#include "CdrTiming.h"

static cdr::String CdrPutDoc (cdr::Session&, const cdr::dom::Node&,
                              cdr::db::Connection&, bool);

static void auditDoc (cdr::db::Connection&, int, int, cdr::String,
                      cdr::String, cdr::String);

static void delQueryTerms (cdr::db::Connection&, int);

static cdr::String createFragIdTransform (cdr::db::Connection&, int);

static void rememberQueryTerm(int, cdr::String&, cdr::String&, wchar_t*,
                              cdr::QueryTermSet&);

static void checkForDuplicateTitle(cdr::db::Connection&, int, 
                                   const cdr::String&,
                                   const cdr::String&);

// String constants used as substitute titles
#define MALFORMED_DOC_TITLE L"!Malformed document - no title can be generated"
#define NO_TITLE_AVAILABLE  L"!No title available for this document"

// Constructor for a CdrDoc
// Extracts the various elements from the CdrDoc for database update
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    cdr::dom::Node& docDom
) : docDbConn (dbConn),
    Comment (true),
    BlobData (true),
    ValDate (true),
    parsed (false),
    malformed (false),
    Id (0),
    DocType (0),
    ActiveStatus (L"A"),
    Xml (L""),
    schemaDocId (0),
    lastFragmentId (0),
    revisedXml (L""),
    revFilterFailed (false),
    revFilterLevel (0),
    NeedsReview (false),
    titleFilterId (0),
    Title (L"")
{

    // Get doctype and id from CdrDoc attributes
    const cdr::dom::Element& docElement =
        static_cast<const cdr::dom::Element&>(docDom);
    TextDocType = docElement.getAttribute (L"Type");
    if (TextDocType.size() == 0)
        throw cdr::Exception (L"CdrDoc: Doctag missing 'Type' attribute");
    TextId = docElement.getAttribute (L"Id");

    // If Id was passed, document must exist in database.
    if (TextId.size() > 0) {
        Id = TextId.extractDocId ();

        // Does the document exist?
        // Find out and also get the last used auto generated fragment id
        std::string qry = "SELECT Id, last_frag_id FROM document WHERE Id = ?";
        cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
        select.setInt(1, Id);
        cdr::db::ResultSet rs = select.executeQuery();
        if (!rs.next())
            throw cdr::Exception(L"CdrDoc: Unable to find document " + TextId);
        lastFragmentId = rs.getInt (2);
    }
    else
        Id = 0;

    // Check DocType - must be one of recognized types
    // Get name of XSLT filter to generate title at the same time
    // And get document id of the schema for this doc type
    std::string qry = "SELECT id, title_filter, xml_schema "
                      "  FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
    select.setString (1, TextDocType);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"CdrDoc: DocType '" + TextDocType +
                             L"' is invalid");
    DocType       = rs.getInt(1);
    cdr::Int tfi  = rs.getInt(2);
    cdr::Int sdi  = rs.getInt(3);

    // There may not be schema or filter ids, so we need to check
    if (tfi.isNull())
        titleFilterId = 0;
    else
        titleFilterId = tfi;
    if (sdi.isNull())
        schemaDocId = 0;
    else
        schemaDocId = sdi;

    // Default values for control elements in the document table
    // All others are defaulted to NULL by cdr::String constructor
    ValStatus    = L"U";
    ActiveStatus = L"A";

    // Pull out the parts of the document from the dom
    cdr::dom::Node child = docDom.getFirstChild ();
    while (child != 0) {

        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {

            // Assign each element to object variables
            cdr::String name = child.getNodeName();
            if (name == L"CdrDocCtl") {
                cdr::dom::Node ctlNode = child.getFirstChild ();
                while (ctlNode != 0) {
                    if (ctlNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE){
                        cdr::String ctlTag = ctlNode.getNodeName();

                        // Capture each element.
                        // Many elements are not allowed to be modified
                        //   by client programs.  These are skipped.  Only
                        //   client modifiable elements are examined
                        //   and retained.
                        if (ctlTag == L"DocTitle")
                            // Title from retrieved doc is captured here
                            //   but will almost certainly be overridden
                            //   by createTitle XSLT script
                            // This title is only kept if no XSLT script
                            //   exists for this doc type
                            Title = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocComment")
                            Comment = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocActiveStatus") {
                            ActiveStatus = cdr::dom::getTextContent (ctlNode);
                            if (ActiveStatus != L"A" && ActiveStatus != L"I")
                                throw cdr::Exception(
                                   L"CdrDoc: Unrecognized DocActiveStatus '" +
                                   ActiveStatus + L"' - expecting 'A' or 'I'");
                        }
                        else if (ctlTag == L"DocNeedsReview") {
                            NeedsReview = cdr::ynCheck (
                                   cdr::dom::getTextContent (ctlNode),
                                   false, L"DocNeedsReview");
                        }

                        // Everything else is ignored
                    }

                    ctlNode = ctlNode.getNextSibling ();
                }
            }

            else if (name == L"CdrDocXml") {
                cdr::dom::Node ctlNode = child.getFirstChild ();
                while (ctlNode != 0) {
                    if (ctlNode.getNodeType() ==
                                    cdr::dom::Node::CDATA_SECTION_NODE) {

                        Xml = ctlNode.getNodeValue();
                        break;
                    }
                    ctlNode = ctlNode.getNextSibling ();
                }
            }

            else if (name == L"CdrDocBlob") {

                BlobData = cdr::Blob(cdr::dom::getTextContent(child));
            }

            else
                // Nothing else is allowed
                throw cdr::Exception(L"CdrDoc: Unrecognized element '" +
                                     name + L"' in CdrDoc");
        }

        child = child.getNextSibling ();
    }

    // Check for absolutely required fields
    if (Xml.size() == 0)
        throw cdr::Exception (L"CdrDoc: Missing required 'Xml' element");

    // Generate a title, or use an existing one, or create an error title
    createTitle();

} // CdrDoc ()


// Constructor for a CdrDoc
// Extracts the various elements from the database by document ID
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    int docId
) : docDbConn (dbConn), Id (docId) {

    // Text version of identifier is from passed id
    TextId = cdr::stringDocId (Id);

    // Get information from document and related tables
    std::string query = "          SELECT d.val_status,"
                        "                 d.val_date,"
                        "                 d.title,"
                        "                 d.active_status,"
                        "                 d.doc_type,"
                        "                 d.xml,"
                        "                 d.last_frag_id,"
                        "                 d.comment,"
                        "                 t.name,"
                        "                 t.title_filter,"
                        "                 t.xml_schema,"
                        "                 b.data,"
                        "                 r.doc_id"
                        "            FROM document d"
                        "            JOIN doc_type t"
                        "              ON t.id = d.doc_type"
                        " LEFT OUTER JOIN doc_blob b"
                        "              ON b.id = d.id"
                        " LEFT OUTER JOIN ready_for_review r"
                        "              ON r.doc_id = d.id"
                        "           WHERE d.id = ?";
    cdr::db::PreparedStatement select = docDbConn.prepareStatement(query);
    select.setInt(1, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"CdrDoc: Unable to load document " + TextId);

    // Copy info into this object
    ValStatus      = rs.getString (1);
    ValDate        = rs.getString (2);
    Title          = cdr::entConvert (rs.getString (3));
    ActiveStatus   = rs.getString (4);
    DocType        = rs.getInt (5);
    Xml            = rs.getString (6);
    lastFragmentId = rs.getInt (7);
    Comment        = cdr::entConvert (rs.getString (8));
    TextDocType    = rs.getString (9);
    cdr::Int tfi   = rs.getInt (10);
    cdr::Int sdi   = rs.getInt (11);
    BlobData       = rs.getBytes (12);
    cdr::Int rvRdy = rs.getInt (13);

    // There may not be schema or filter ids, default is 0
    if (tfi.isNull())
        titleFilterId = 0;
    else
        titleFilterId = tfi;
    if (sdi.isNull())
        schemaDocId = 0;
    else
        schemaDocId = sdi;

    // Default for ready_for_review table is false (document not so marked)
    if (rvRdy.isNull())
        NeedsReview = false;
    else
        NeedsReview = true;

    // We haven't filtered this for insertion, deletion markup
    revisedXml = L"";

    // We have not parsed the xml yet and don't know of anything wrong with it
    parsed    = false;
    malformed = false;

    // Haven't filtered it either
    revFilterFailed = false;
    revFilterLevel  = 0;
}


/**
 * Store a document.
 * Assumes all checking already done in CdrPutDoc()
 * Throws exception if failed.
 *
 */
void cdr::CdrDoc::Store ()
{
    std::string sqlStmt;


    // New record
    if (!Id) {
        sqlStmt =
            "INSERT INTO document ("
            "   val_status,"
            "   val_date,"
            "   active_status,"
            "   doc_type,"
            "   title,"
            "   xml,"
            "   comment,"
            "   last_frag_id"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    }

    // Existing record
    else {
        sqlStmt =
            "UPDATE document "
            "  SET val_status = ?,"
            "      val_date = ?,"
            "      active_status = ?,"
            "      doc_type = ?,"
            "      title = ?,"
            "      xml = ?,"
            "      comment = ?,"
            "      last_frag_id = ?"
            "  WHERE"
            "      id = ?";
    }

    // Fill in values
    cdr::db::PreparedStatement docStmt = docDbConn.prepareStatement (sqlStmt);
    docStmt.setString (1, ValStatus);
    docStmt.setString (2, ValDate);
    docStmt.setString (3, ActiveStatus);
    docStmt.setInt    (4, DocType);
    docStmt.setString (5, Title);
    docStmt.setString (6, Xml);
    docStmt.setString (7, Comment);
    docStmt.setInt    (8, lastFragmentId);

    // For existing records, fill in ID
    if (Id) {
        docStmt.setInt (9, Id);
    }

    // Do it
    docStmt.executeUpdate ();

    // If new record, find out what was the key
    if (!Id) {
        Id = docDbConn.getLastIdent ();

        // Convert id to wide characters
        char idbuf[32];
        sprintf (idbuf, "CDR%010d", Id);
        TextId = idbuf;
    }

    // Clear out any old blob data for the document.
    static const char* const delBlobQuery = "DELETE doc_blob WHERE id = ?";
    cdr::db::PreparedStatement delBlob =
        docDbConn.prepareStatement(delBlobQuery);
    delBlob.setInt(1, Id);
    delBlob.executeUpdate();

    // Store the blob data, if any.
    if (!BlobData.isNull()) {
        static const char* const insBlobQuery =
            "INSERT doc_blob(id, data) VALUES(?,?)";
        cdr::db::PreparedStatement insBlob =
            docDbConn.prepareStatement(insBlobQuery);
        insBlob.setInt(1, Id);
        insBlob.setBytes(2, BlobData);
        insBlob.executeUpdate();
    }

    // Update ready_for_review table as required
    // This table will probably be very small, and in memory all
    //   the time.  I don't bother trying to figure out if an
    //   update is required because the cost of trying is low
    //   and the always-do-it technique is robust.
    if (NeedsReview) {
        static const char *const revQuery =
            "INSERT INTO ready_for_review (doc_id) VALUES (?)";
        cdr::db::PreparedStatement revReady =
            docDbConn.prepareStatement (revQuery);
        revReady.setInt (1, Id);
        try {
            revReady.executeUpdate();
        }
        catch (...) {
            // Ignore failure, it means that the doc is already
            //   marked ready for review
            ;
        }
    }
    else {
        // Insure that if it was ready for review, it's not now
        static const char *const revQuery =
            "DELETE FROM ready_for_review WHERE doc_id = ?";
        cdr::db::PreparedStatement revReady =
            docDbConn.prepareStatement (revQuery);
        revReady.setInt (1, Id);
        try {
            revReady.executeUpdate();
        }
        catch (...) {
            // Ignore failure
            ;
        }
    }



} // Store

/**
 * Add a new document to the database.
 * A front end to CdrPutDoc.
 * Called by CdrCommand.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::addDoc (
    cdr::Session& session,
    const cdr::dom::Node& node,
    cdr::db::Connection& conn
) {
    // Call CdrPutDoc with flag indicating new document.
    // Return result.
    return CdrPutDoc (session, node, conn, 1);
}


/**
 * Update a document in the database by replacing it.
 * A front end to CdrPutDoc.
 * Called by CdrCommand.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::repDoc (
    cdr::Session& session,
    const cdr::dom::Node& node,
    cdr::db::Connection& conn
) {
    // Call CdrPutDoc with flag indicating new document.
    // Return result.
    return CdrPutDoc (session, node, conn, 0);
}


/**
 * Add or update a document in the CDR.
 * Called by CdrAddDoc and CdrRepDoc.
 *
 *  @param      session     Contains information about the current user.
 *  @param      node        Contains the XML DOM for the complete command.
 *                          The document is one node within this.
 *  @param      conn        Reference to the connection object for the
 *                          CDR database.
 *  @param      newrec      True=This is a new record, False=update rec.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */

static cdr::String CdrPutDoc (
    cdr::Session& session,
    const cdr::dom::Node& cmdNode,
    cdr::db::Connection& dbConn,
    bool newrec
) {
    MAKE_TIMER(putDocTimer);
    MAKE_TIMER(incrementalTimer);

    bool cmdCheckIn,            // True=Check in doc, else doc still locked
         cmdVersion,            // True=Create new version in version control
         cmdPublishVersion,     // True=New version ctl version is publishable
         cmdValidate,           // True=Perform validation, else just store
         cmdSchemaVal,          // True=Validate schema
         cmdLinkVal,            // True=Validate links
         cmdSetLinks,           // True=Update link tables
         cmdEcho;               // True=Client want modified doc echoed back
    cdr::String cmdReason;      // Reason to associate with new version
    cdr::String validationTypes;// Schema, Links or both, empty=both
    cdr::dom::Node child,       // Child node in command
                   docNode;     // Node containing CdrDoc
    cdr::dom::NamedNodeMap attrMap; // Map of attributes in DocCtl subelement
    cdr::dom::Node attr;        // Single attribute from map

    // Default values, may be replaced by elements in command
    cmdVersion        = false;
    cmdPublishVersion = false;
    cmdValidate       = true;
    cmdSchemaVal      = true;
    cmdLinkVal        = true;
    cmdCheckIn        = false;
    cmdEcho           = false;
    cmdSetLinks       = true;
    validationTypes   = L"";

    // Default reason is NULL created by cdr::String contructor
    cmdReason   = L"";

    // Parse command to get document and relevant parts
    child = cmdNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();

            // User must specify whether to check-in (unlock) the
            //   document or leave it locked
            if (name == L"CheckIn")
                cmdCheckIn = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"CheckIn");

            else if (name == L"Version") {
                cmdVersion = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"Version");
                // If versioning, must specify publishability of new version
                if (cmdVersion) {
                    attrMap = child.getAttributes();
                    if ((attr = attrMap.getNamedItem ("Publishable")) == NULL)
                        throw cdr::Exception (
                            L"Missing required 'Publishable' attribute "
                            L"in Version element");
                    cmdPublishVersion =
                        cdr::ynCheck (cdr::String (attr.getNodeValue()), false,
                            L"'Publishable' attribute of Version element");
                }
            }


            else if (name == L"Validate") {
                cmdValidate = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"Validate");

                // If user said Y(es), validate
                if (cmdValidate) {

                    // User may request specific types
                    attrMap = child.getAttributes();
                    if ((attr = attrMap.getNamedItem ("ValidationTypes"))
                             != NULL) {

                        // Default is do all validation, but user has requested
                        //   specific types.  Turn off defaults
                        cmdSchemaVal = false;
                        cmdLinkVal   = false;

                        // Turn back on the specifically requested ones
                        validationTypes = attr.getNodeValue();
                        if (validationTypes.find (L"Schema") !=
                                validationTypes.npos)
                            cmdSchemaVal = true;
                        if (validationTypes.find (L"Links") !=
                                validationTypes.npos)
                            cmdLinkVal = true;

                        // Sanity check
                        if (!cmdSchemaVal && !cmdLinkVal)
                            throw cdr::Exception (
                                    L"No known validation types were "
                                    L" requested in ValidationTypes='" +
                                    validationTypes + L"'");
                    }
                }
            }

            else if (name == L"SetLinks")
                // Update of link tables might be turned off to speed up
                //   certain kinds of batch loads - with link updates done
                //   later
                cmdSetLinks = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"SetLinks");

            else if (name == L"Echo")
                cmdEcho = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"Echo");

            else if (name == L"Reason")
                cmdReason = cdr::dom::getTextContent (child);

            else if (name == L"CdrDoc")

                // Save node for later use
                docNode = child;

            else {
                throw cdr::Exception (L"CdrDoc: Unrecognized element '" +
                                        name + L"' in command");
            }
        }

        child = child.getNextSibling ();
    }
    if (!cmdValidate) {
        cmdSchemaVal = false;
        cmdLinkVal   = false;
    }
    SHOW_ELAPSED("command parsed", incrementalTimer);

    // Make sure validation has been invoked if a publishable version
    // has been requested.  The DLL will enforce this, but we do it
    // here as well, since the DLL is our primary, but not our only client.
    if (cmdPublishVersion && (!cmdSchemaVal || !cmdLinkVal))
        throw cdr::Exception(L"CdrPubDoc: creation of a published version "
                             L"not allowed unless full document validation is "
                             L"invoked.");

    // Construct a doc object containing all the data
    // The constructor may look for additonal elements in the DocCtl
    //   not parsed out by the above logic
    if (docNode == 0)
        throw cdr::Exception(L"CdrPutDoc: No 'CdrDoc' element in transaction");
    cdr::CdrDoc doc (dbConn, docNode);
    SHOW_ELAPSED("CdrDoc constructed", incrementalTimer);

    // Check user authorizations
    cdr::String action = newrec ? L"ADD DOCUMENT" : L"MODIFY DOCUMENT";
    cdr::String doctype = doc.getTextDocType();
    if (!session.canDo (dbConn, action, doctype))
        throw cdr::Exception (L"CdrPutDoc: User '" + session.getUserName() +
                              L"' not authorized to '" + action +
                              L"' with docs of type '" + doctype + L"'");

    // Make sure user is authorized for the desired active status.
    if (!newrec) {
        static const char* const query = "SELECT active_status "
                                         "  FROM document      "
                                         " WHERE id = ?        ";
        cdr::db::PreparedStatement st = dbConn.prepareStatement(query);
        st.setInt(1, doc.getId());
        cdr::db::ResultSet rs = st.executeQuery();
        if (!rs.next())
            throw cdr::Exception(L"Failure retrieving document status "
                                 L"from database");
        cdr::String dbActStat = rs.getString(1);
        if (dbActStat != doc.getActiveStatus() 
                && !session.canDo(dbConn, L"PUBLISH DOCUMENT", doctype))
            if (dbActStat == L"A")
                throw cdr::Exception(L"User not authorized to block "
                                     L"publication for this document");
            else
                throw cdr::Exception(L"User not authorized to remove "
                                     L"publication block for this document");
    }
    else if (doc.getActiveStatus() != L"A"
            && !session.canDo(dbConn, L"PUBLISH DOCUMENT", doctype))
        throw cdr::Exception(L"User not authorized to block publication "
                             L"for this document");

    // Make sure user is authorized to create the first publishable version.
    if (cmdPublishVersion) {
        static const char* const query = "SELECT COUNT(*)         "
                                         "  FROM doc_version      "
                                         " WHERE id = ?           "
                                         "   AND publishable = 'Y'";
        cdr::db::PreparedStatement st = dbConn.prepareStatement(query);
        st.setInt(1, doc.getId());
        cdr::db::ResultSet rs = st.executeQuery();
        if (!rs.next())
            throw cdr::Exception(L"Failure determining publishable version "
                                 L"count for document");
        int nPublishableVersions = rs.getInt(1);
        if (!nPublishableVersions
                && !session.canDo(dbConn, L"PUBLISH DOCUMENT", doctype))
            throw cdr::Exception(L"User not authorized to create the first "
                                 L"publishable version for this document");
    }
    SHOW_ELAPSED("permissions checked", incrementalTimer);

    // Block creation of documents with duplicate titles for special document
    // types.
    if (doctype == L"Filter" || doctype == L"css"    || doctype == L"schema")
        checkForDuplicateTitle(dbConn, doc.getId(), doctype, doc.getTitle());

    // Does document id attribute match expected action type?
    if (doc.getId() && newrec)
        throw cdr::Exception (L"CdrPutDoc: Attempt to add doc with "
                              L"existing ID as new");
    else if (!doc.getId() && !newrec)
        throw cdr::Exception (L"CdrPutDoc: Attempt to replace doc "
                              L"without existing ID");

    // Run all the custom pre-processing routines.
    doc.preProcess(cmdValidate);
    SHOW_ELAPSED("preprocessing completed", incrementalTimer);

    // Start transaction - everything is serialized from here on
    // If any called function throws an exception, CdrServer
    //   will catch it and perform a rollback
    dbConn.setAutoCommit (false);

    // If new record, store it so we can get the new ID
    if (newrec) {
        doc.Store ();

        // If we need to store a version, the user must check it out first
        //   but he can't have done that for a new record, so we do it now
        // Note: Auto-check-out is also performed for new records for which
        //   check-in has not been explicitly requested.  This supports
        //   the case where a user is editing a new document, saves, and
        //   and continues editing.
        // To store a new record without checking it out, the client must
        //   explicitly request check-in.
        if (cmdVersion || !cmdCheckIn)
            cdr::checkOut (session, doc.getId(), dbConn,
                           L"Temp checkout by addDoc", false);
        SHOW_ELAPSED("new doc stored", incrementalTimer);
    }

    else {
        // Is document locked by this or another user?
        int         lockUserId;     // User id of person who checked out doc
        cdr::String lockDate;       // Date checked out
        cdr::String errMsg;         // For reporting lock errors

        if (cdr::isCheckedOut (doc.getId(), dbConn, &lockUserId, &lockDate)) {
            if (lockUserId != session.getUserId()) {


                // It's checked out to someone else
                // Tell user who it is and give date/time checked out
                std::string qry = "SELECT name, fullname "
                                  "  FROM usr "
                                  " WHERE id = ?";
                cdr::db::PreparedStatement stmt = dbConn.prepareStatement(qry);
                stmt.setInt(1, lockUserId);
                cdr::db::ResultSet rs = stmt.executeQuery();

                // Should never happen
                if (!rs.next())
                    throw cdr::Exception (L"Document is checked out but "
                                          L"user not identified!");

                // Report who has it, and when
                // Give brief user name and full name ifwe have it
                cdr::String lockUserName     = rs.getString (1);
                cdr::String lockUserFullName = rs.getString (2);
                if (lockUserFullName.size() > 0)
                    lockUserFullName = L" (" + lockUserFullName + L"";
                throw cdr::Exception (
                        L"User " + session.getUserName()
                        + L" cannot check-in document checked out by user "
                        + lockUserName + lockUserFullName
                        + L" at " + lockDate);
            }
        }

        else
            throw cdr::Exception (L"User " + session.getUserName()
                    + L" has not checked out this document.  "
                    L"Storing document not allowed");
        SHOW_ELAPSED("finished checking lock status", incrementalTimer);
    }

    // Perform validation if requested
    if (cmdValidate) {
        if (cmdLinkVal && !cmdSetLinks)
            // If not setting links, can't do any link validation because
            //   it automatically does a link_net update
            throw cdr::Exception (
                    L"Validation of links without SetLinks is currently "
                    L"unsupported.  Sorry.");

        // Perform validation
        // Will set valid_status and valid_date in the doc object
        // Will overwrite whatever is there
        cdr::execValidateDoc (doc,cdr::UpdateUnconditionally,validationTypes);

        // If the user was hoping for a publishable version, set him straight.
        if (cmdPublishVersion && doc.getValStatus() != L"V") {
            doc.getErrList().push_back(L"Non-publishable version will "
                                       L"be created.");
            cmdPublishVersion = false;
            cmdVersion = true;
        }
        SHOW_ELAPSED("validation completed", incrementalTimer);
    }

    if (cmdSetLinks && !cmdLinkVal) {
        // Update of link tables is part of unconditional link validation
        // But if not done there, we have to do it here unless it was
        //   specifically suppressed with the SetLinks command.
        // Call to parseAvailable is done first to check if the document
        //   was already parsed, or if not, parse it.  It also
        //   handles revision filtering before the parse.
        if (doc.parseAvailable())
            cdr::link::CdrSetLinks (doc.getDocumentElement(), dbConn,
                                    doc.getId(), doc.getTextDocType(),
                                    cdr::UpdateLinkTablesOnly,
                                    doc.getErrList());
        SHOW_ELAPSED("setting links", incrementalTimer);
    }

    // If we haven't already done so, store it
    if (!newrec) {
        doc.Store ();
        SHOW_ELAPSED("document replaced", incrementalTimer);
    }

    // Add support for queries.
    doc.updateQueryTerms();
    SHOW_ELAPSED("back from updateQueryTerms", incrementalTimer);

    // A <Comment> may have been specified to store in the document table,
    //   and a <Reason> to store in the audit_trail table.
    // If no Reason was specified, we'll use the Comment for auditing
    //   also.
    // XXX It's not clear that we should have separate comment columns
    //     in the all_docs and doc_version tables.  The theory was that
    //     these would bear the answer to the question "what did you do?"
    //     and the comment column in the audit_trail table would be for
    //     "why did you do it?" but no one has come up with a convincing
    //     use case.
    if (cmdReason == L"")
        cmdReason = doc.getComment();

    // Append audit info
    // Have to do this before checkIn so checkIn can use the exact same
    //   date-time, as taken from the audit trail.
    auditDoc (dbConn, doc.getId(), session.getUserId(), action,
              L"CdrPutDoc", cmdReason);
    SHOW_ELAPSED("action audited", incrementalTimer);

    // Checkin must be performed either to checkin or version the doc.
    // If versioning without long term checkin, then an immediate
    //   re-checkout is required.
    if (cmdCheckIn || cmdVersion) {

        // "abandon" parameter on checkIn tells version control to
        //   NOT create a new version.
        bool vcAbandon = true;
        if (cmdVersion)
            vcAbandon = false;

        // Perform checkIn and (possibly) versioning
        cdr::checkIn (session, doc.getId(), dbConn,
                      cmdPublishVersion ? L"Y" : L"N",
                      &cmdReason, vcAbandon);

        // If user wants to keep his lock on the document, we check it out
        //    again.
        if (!cmdCheckIn)
            cdr::checkOut (session, doc.getId(), dbConn,
                           L"Continue editing after versioning");
        SHOW_ELAPSED("versioning/checkin done", incrementalTimer);
    }

    // If we got here okay commit all updates
    dbConn.commit();
    dbConn.setAutoCommit (true);
    SHOW_ELAPSED("changes committed", incrementalTimer);

    // Return string with errors, etc.
    cdr::String rtag = newrec ? L"Add" : L"Rep";
    cdr::String resp = cdr::String (L"  <Cdr") + rtag + L"DocResp>\n"
                     + L"   <DocId>" + doc.getTextId() + L"</DocId>\n"
                     + doc.getErrString();
    if (cmdEcho)
        resp += cdr::getDocString(doc.getTextId(), dbConn, true, true) + L"\n";

    resp += L"  </Cdr" + rtag + L"DocResp>\n";
    SHOW_ELAPSED("command response ready", incrementalTimer);
    SHOW_ELAPSED("total elapsed time for CdrPutDoc", putDocTimer);
    return resp;

} // CdrPutDoc


/**
 * Delete a document.
 *
 * See CdrDoc.h for details.
 */

cdr::String cdr::delDoc (
    cdr::Session& session,
    const cdr::dom::Node& cmdNode,
    cdr::db::Connection& conn
) {
    cdr::String cmdReason,      // Reason to associate with new version
                docIdStr,       // Doc id in string form
                docTypeStr;     // Doc type in string form
    int         docId,          // Doc id as unadorned integer
                docType,        // Document type
                userId,         // User invoking this command
                outUserId,      // User with the doc checked out, if it is
                errCount;       // Number of link releated errors
    bool        autoCommitted,  // True=Not inside SQL transaction at start
                cmdValidate;    // True=Validate that nothing links to doc
    cdr::dom::Node child,       // Child node in command
                   docNode;     // Node containing CdrDoc
    cdr::ValidRule vRule;       // Validation request to CdrDelLinks
    cdr::StringList errList;    // List of errors from CdrDelLinks


    // Default values, may be replace by elements in command
    // Default reason is NULL created by cdr::String contructor
    cmdValidate = true;
    cmdReason   = L"";
    docId       = -1;

    // Parse incoming command
    child = cmdNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();

            if (name == L"DocId") {
                docIdStr = cdr::dom::getTextContent (child);
                docId    = docIdStr.extractDocId ();
            }

            else if (name == L"Validate")
                cmdValidate = cdr::ynCheck (cdr::dom::getTextContent (child),
                                           false, L"Validate");

            else if (name == L"Reason")
                cmdReason = cdr::dom::getTextContent (child);

            else
                throw cdr::Exception (L"CdrDelDoc: Unrecognized element '" +
                                        name + L"' in command");
        }

        child = child.getNextSibling ();
    }

    // Get document type, need it to check user authorization
    std::string tQry = "SELECT t.id, t.name "
                       "  FROM document d "
                       "  JOIN doc_type t "
                       "    ON d.doc_type = t.id "
                       " WHERE d.id = ?";
    cdr::db::PreparedStatement tSel = conn.prepareStatement (tQry);
    tSel.setInt (1, docId);
    cdr::db::ResultSet tRs = tSel.executeQuery();
    if (!tRs.next())
        throw cdr::Exception (L"delDoc: Unable to find document " + docIdStr);
    docType    = tRs.getInt (1);
    docTypeStr = tRs.getString (2);

    // Is user authorized to do this?
    if (!session.canDo (conn, L"DELETE DOCUMENT", docTypeStr))
        throw cdr::Exception (L"delDoc: User not authorized to delete docs "
                              L"of this type");

    // If no reason given, supply a default
    if (cmdReason.length() == 0)
        cmdReason = L"Document deleted.  No reason recorded.";

    // From now on, do everything or nothing
    // setAutoCommit() checks state first, so it won't end an existing
    //   transaction
    autoCommitted = conn.getAutoCommit();
    conn.setAutoCommit(false);

    // Has anyone got this document checked out?
    // If not, that's okay.  We don't require a check-out in order to
    //   delete a document - as long as no one else has it checked out.
    // However if it is checked out we'll check it in.  Won't allow
    //   a deleted document to stay checked out.
    userId = session.getUserId();
    if (isCheckedOut(docId, conn, &outUserId)) {
        if (outUserId != userId)
            throw cdr::Exception (L"Document " + docIdStr + L" is checked out"
                                  L" by another user");

        // If it's checked out, we'll check it in
        // No version is created in version control
        checkIn (session, docId, conn, L"I", &cmdReason, true, false);
    }

    // Update the link net to delete links from this document
    // If validation was requested, we tell CdrDelLinks to abort if
    //   there are any links _to_ this document
    vRule = cmdValidate ? UpdateIfValid : UpdateUnconditionally;

    errCount = cdr::link::CdrDelLinks (conn, docId, vRule, errList);

    if (errCount == 0 || vRule == UpdateUnconditionally) {

        // Proceed with deletion
        // Begin by deleting all query terms pointing to this doc
        delQueryTerms (conn, docId);

        // The document itself is not removed from the database, it
        //   is simply marked as deleted
        // Note that if document is already deleted, we silently do
        //   nothing
        std::string dQry = "UPDATE document "
                           "   SET active_status = 'D' "
                           " WHERE id = ?";
        cdr::db::PreparedStatement dSel = conn.prepareStatement (dQry);
        dSel.setInt (1, docId);
        cdr::db::ResultSet dRs = dSel.executeQuery();

        // Record what we've done
        auditDoc (conn, docId, userId, L"DELETE DOCUMENT",
                  L"CdrDelDoc", cmdReason);
    }

    // Restore autocommit status if required
    conn.setAutoCommit(autoCommitted);

    // Prepare response
    cdr::String resp = L"  <CdrDelDocResp>\n"
                       L"   <DocId>" + docIdStr + L"</DocId>\n";

    // Include any errors
    if (errCount > 0)
        resp += cdr::packErrors (errList);

    // Terminate and return response
    return (resp + L"  </CdrDelDocResp>\n");
} // delDoc


/**
 * Determine whether the DocumentElement (top node of the parse
 * tree) is available, make it so if not.
 */

bool cdr::CdrDoc::parseAvailable ()
{
    cdr::String data;

    // If we've already got a parse tree, one way or another, we're done
    if (parsed) {
        if (malformed)
            return false;
        return true;
    }

    // The XML to parse should be revision filtered
    // This is a design decision with pros and cons, but we've
    //   decided the pros win
    if (revisedXml.size() == 0)
        getRevisionFilteredXml();

    // Parse whatever is available
    if (revisedXml.size() > 0)
        data = revisedXml;
    else if (Xml.size() > 0)
        data = Xml;
    else {
        // Nothing to build a tree from
        errList.push_back (L"No XML in this document to parse");
        return false;
    }

    // Build a parse tree
    cdr::dom::Parser parser;
    try {
        // Successful or not, the parse will have been attempted
        parsed = true;

        // Attempt it
        parser.parse (data);
        docElem = parser.getDocument().getDocumentElement();
    }
    catch (const cdr::dom::XMLException& e) {
        // XML is unparsable, presumed to be malformed
        this->malFormed();

        // Save error message from parser
        errList.push_back (L"Parsing XML: " + cdr::String (e.getMessage()));
        return false;
    }
    catch (...) {
        this->malFormed();
        errList.push_back (L"Unknown error parsing XML");
        return false;
    }

    // If we're here, parse was successful, document is well formed
    // Flag should be false already but it's reasonable to confirm it here
    malformed = false;

    return true;

} // parseAvailable


/**
 * Return an XML string with revision markup filtered out as per the
 * standard techniques.
 */

cdr::String cdr::CdrDoc::getRevisionFilteredXml (
    int         revisionLevel,
    bool        getIfUnfiltered
) {
    // String to receive warnings, errors, etc. from XSLT
    cdr::String errorStr = L"";

    // Validate parameter
    if (revisionLevel < 1 || revisionLevel > 3)
        throw cdr::Exception (L"CdrDoc::getRevisedXml: Illegal revision filter "
                             + cdr::String::toString (revisionLevel)
                             + L" requested for document");

    // Need to filter revision markup if:
    //   Filter attempt has not already failed (!revFilterFailed)
    //   Filtering never attempted (revFilterLevel == 0)
    //   Filtering done but at level other than what we want (revisionLevel...)
    //   Any previous revision filtering discarded (...size() == 0)
    if (!revFilterFailed &&
        ((revisionLevel != revFilterLevel) || revisedXml.size() == 0)) {

        // Attempt to filter at the requested level
        cdr::FilterParmVector pv;        // Parameters passed to it
        pv.push_back (std::pair<cdr::String,cdr::String>
            (L"useLevel", cdr::String::toString (revisionLevel)));

        try {
            revisedXml = cdr::filterDocumentByScriptTitle (
                Xml, L"Revision Markup Filter", docDbConn, &errorStr, &pv);
        }
        catch (cdr::Exception& e) {
            // Don't try filtering this doc again
            revFilterFailed = true;

            // Save error msg for client
            errList.push_back (L"Filtering revision markup: " +
                               cdr::String (e.what ()));
        }

        // Append any warnings
        if (errorStr.size() > 0)
            errList.push_back (
                L"Revision Markup Filter returned these messages: " +
                errorStr);

        // If successful, save level info
        if (!revFilterFailed) {
            if (revisedXml.size() > 0)
                revFilterLevel = revisionLevel;
            else {
                // Didn't really succeed
                revFilterLevel  = 0;
                revFilterFailed = true;
                errList.push_back (
                        L"Filtering revision markup: Got 0 length result");
            }
        }
    }

    // If everything still looks good, return the filtered xml
    if (!revFilterFailed)
        return revisedXml;

    // Else return depends on whether caller asked for unfiltered data
    if (getIfUnfiltered)
        return Xml;

    // Nothing left to return
    return L"";
}


/*
 * Generate a title for the document by means of an XSLT filter.
 */

void cdr::CdrDoc::createTitle()
{
    // If a filter is available, use it
    if (titleFilterId > 0) {

        // If revision markup not yet filtered, filter it now.
        // This is needed because it might affect the title generation fields
        cdr::String xml = getRevisionFilteredXml(DEFAULT_REVISION_LEVEL, true);

        // Generate title
        cdr::String filterTitle = L"";
        cdr::String filterMsgs  = L"";
        try {
            filterTitle = cdr::filterDocumentByScriptId (
                    Xml, titleFilterId, docDbConn, &filterMsgs);
        }
        catch (cdr::Exception& e) {
            // Add an error to the doc object
            errList.push_back (L"Generating title: " + cdr::String (e.what()));
            filterTitle = L"";
        }

        // If any messages returned, make them available for later viewing
        if (filterMsgs.size() > 0)
            errList.push_back (
                L"Generating title, filter produced these messages: " +
                filterMsgs);

        // If we got a title, it replaces whatever is was passed in
        //   an update transaction.
        if (filterTitle.size() > 0)
            Title = filterTitle;
    }

    // If we still have no title because:
    //   No title filter defined and no title supplied by other means, or
    //   Title filter ran but generated no output, or
    //   Title filter generated terminal error
    // Then
    //   Generate a default error title.
    if (Title.size() == 0) {
        errList.push_back (
            L"No title available for document, using default error title");
        Title = NO_TITLE_AVAILABLE;
    }
}


/**
 * Mark a document as malformed
 */

void cdr::CdrDoc::malFormed()
{
    // Mark this objec in memory
    malformed = true;

    // Set validation status in case doc is stored
    ValStatus = L"M";

    // Title derives from the document, but we can't derive a title
    //   from a malformed document.
    // If there is a title, leave it alone, else see if there's a filter
    if (Title.size() == 0) {
        if (titleFilterId == 0)
            Title = NO_TITLE_AVAILABLE;
        else
            Title = MALFORMED_DOC_TITLE;
    }
}


/**
 * Log an action on a document to the audit_trail
 *
 * @param  dbConn       Database connection.
 * @param  docId        Document ID.
 * @param  user         Userid.
 * @param  action       An id from the action table, corresponding to one of:
 *                         "ADD DOCUMENT"
 *                         "MODIFY DOCUMENT"
 *                         "DELETE DOCUMENT"
 * @param  program      Optional program name or null string.
 * @param  comment      Optional free text comment or null string.
 */

static void auditDoc (
    cdr::db::Connection& dbConn,
    int                  docId,
    int                  userId,
    cdr::String          action,
    cdr::String          program,
    cdr::String          comment
) {
    int actionId = 0;

    // Convert action string to expected code
    std::string actQry = "SELECT id FROM action WHERE name = ?";
    cdr::db::PreparedStatement actStmt = dbConn.prepareStatement(actQry);
    actStmt.setString (1, action);
    cdr::db::ResultSet actRs = actStmt.executeQuery();
    if (!actRs.next())
        throw cdr::Exception (L"auditDoc: Bad action string '"
                              + action
                              + L"'  Can't happen.");
    actionId = actRs.getInt (1);

    // Insert
    std::string qry = "INSERT INTO audit_trail (document, dt, usr, action, "
                      "program, comment) VALUES (?, GETDATE(), ?, ?, ?, ?)";
    cdr::db::PreparedStatement stmt = dbConn.prepareStatement(qry);
    stmt.setInt (1, docId);
    stmt.setInt (2, userId);
    stmt.setInt (3, actionId);
    stmt.setString (4, program);
    stmt.setString (5, comment);
    cdr::db::ResultSet rs = stmt.executeQuery();
}


/**
 * External command interface to updating query terms for a doc.
 */
cdr::String cdr::reIndexDoc (
    cdr::Session&         session,
    const cdr::dom::Node& commandNode,
    cdr::db::Connection&  dbConnection
) {
    int         docId;      // Doc id from transaction
    cdr::String docIdStr,   // String form "CDR00..."
                name;       // Element name


    // Get document id from command
    cdr::dom::Node node = commandNode.getFirstChild();
    while (node != 0) {

        if (node.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            name = node.getNodeName();
            if (name != L"DocId")
                throw cdr::Exception (L"reIndexDoc: Element '" + name
                                    + L"' in CdrReindexDocs request is "
                                      L"currently unsupported");

            docIdStr = cdr::dom::getTextContent (node);
            docId    = docIdStr.extractDocId();
        }

        node = node.getNextSibling();
    }

    // Construct CdrDoc for this document
    cdr::CdrDoc doc(dbConnection, docId);

    // Update the index table
    doc.updateQueryTerms();

    // Failures are impossible - except for internal exceptions
    return (L"<CdrReindexResp/>");
}

/**
 * Replaces the rows in the query_term table for the current document.
 */
void cdr::CdrDoc::updateQueryTerms()
{
    MAKE_TIMER(queryTermsTimer);
    QueryTermSet oldTerms, newTerms;

    // Sanity check.
    if (Id == 0 || DocType == 0)
        return;

    // Find out which paths get indexed.
    cdr::db::Statement selStmt = docDbConn.createStatement();
    char selQuery[256];
    sprintf(selQuery, "SELECT path"
                      "  FROM query_term_def"
                      " WHERE path LIKE '/%s/%%'"
                      "    OR path LIKE '//%%'",
                      TextDocType.toUtf8().c_str());
    cdr::db::ResultSet rs = selStmt.executeQuery(selQuery);
    StringSet paths;
    while (rs.next()) {
        cdr::String path = rs.getString(1);
        if (paths.find(path) == paths.end()) {
            paths.insert(path);
        }
    }
    SHOW_ELAPSED("getting query_term defs", queryTermsTimer);

    // Gather existing query term rows.
    cdr::db::PreparedStatement oqtStmt = docDbConn.prepareStatement(
            "SELECT path, value, int_val, node_loc"
            "  FROM query_term                    "
            " WHERE doc_id = ?                    ");
    oqtStmt.setInt(1, Id);
    cdr::db::ResultSet oqtResultSet = oqtStmt.executeQuery();
    while (oqtResultSet.next()) {
        cdr::String path = oqtResultSet.getString(1);
        cdr::String value = oqtResultSet.getString(2);
        cdr::Int    int_val = oqtResultSet.getInt(3);
        cdr::String node_loc = oqtResultSet.getString(4);
        QueryTerm qt(Id, path, value, int_val, node_loc);
        oldTerms.insert(qt);
    }
    SHOW_ELAPSED("gathered old query terms", queryTermsTimer);

    // Find out which rows need to be in the query_term table now.
    if (!paths.empty()) {

        // Find or create a parse of the document
        if (parseAvailable()) {
            cdr::dom::Node node = getDocumentElement();

            // Create a buffer for ordinal position paths
            // We construct hex strings here to represent the ordinal
            //   position of each element in the tree leading to a node
            // This enables us to find occurrences of a node which
            //   have a common ancestor at any desired level, e.g., to
            //   find element A and B only when they are siblings of
            //   each other, as opposed A as a subelement of one parent
            //   and B as a subelement of another.
            wchar_t ordPosPath[MAX_INDEX_ELEMENT_DEPTH * INDEX_POS_WIDTH + 1];
            ordPosPath[0] = (wchar_t) '\0';

            // Find query terms for the entire document tree
            // Indexing of ordinal position paths will begin at the level
            //   below the document node.  Since we start at the document
            //   node, we pass a depth of -1 here.
            collectQueryTerms(cdr::String(L""), node.getNodeName(), node,
                              paths, ordPosPath, -1, newTerms);
            SHOW_ELAPSED("finished gathering new query terms", queryTermsTimer);
        }
    }

    // Determine which rows need to be deleted, which need to be added.
    QueryTermSet wantedTerms, unwantedTerms;
    QueryTermSet::const_iterator i;
    QueryTermSet* termsToInsert = &wantedTerms;
    for (i = newTerms.begin(); i != newTerms.end(); ++i)
        if (oldTerms.find(*i) == oldTerms.end())
            wantedTerms.insert(*i);
    for (i = oldTerms.begin(); i != oldTerms.end(); ++i)
        if (newTerms.find(*i) == newTerms.end())
            unwantedTerms.insert(*i);

    // Sometimes it's just faster to wipe out the rows and start fresh.
    if (unwantedTerms.size() > 1000 && 
        unwantedTerms.size() > newTerms.size() / 2) {
        delQueryTerms(docDbConn, Id);
        termsToInsert = &newTerms;
        SHOW_ELAPSED("finished wholesale term deletion", queryTermsTimer);
    }
    else {

        // Wipe out the unwanted rows one by one.
        cdr::db::PreparedStatement delStmt = docDbConn.prepareStatement(
                " DELETE FROM query_term   "
                "       WHERE doc_id   = ? "
                "         AND path     = ? "
                "         AND value    = ? "
                "         AND node_loc = ? ");
        for (i = unwantedTerms.begin(); i != unwantedTerms.end(); ++i) {
            delStmt.setInt   (1, i->doc_id);
            delStmt.setString(2, i->path);
            delStmt.setString(3, i->value);
            delStmt.setString(4, i->node_loc);
            delStmt.executeUpdate();
        }
        SHOW_ELAPSED("finished selective term deletion", queryTermsTimer);
    }

    // Insert new rows.
    cdr::db::PreparedStatement insStmt = docDbConn.prepareStatement(
            " INSERT INTO query_term                               "
            "             (doc_id, path, value, int_val, node_loc) "
            "      VALUES (?, ?, ?, ?, ?)                          ");
    for (i = termsToInsert->begin(); i != termsToInsert->end(); ++i) {
        insStmt.setInt   (1, i->doc_id);
        insStmt.setString(2, i->path);
        insStmt.setString(3, i->value);
        insStmt.setInt   (4, i->int_val);
        insStmt.setString(5, i->node_loc);
        insStmt.executeUpdate();
    }
    SHOW_ELAPSED((cdr::String("finished inserting ") +
                  cdr::String::toString(termsToInsert->size()) +
                  cdr::String(" new terms")).toUtf8().c_str(), 
                  queryTermsTimer);
}

// Routines to modify the document before saving or validating.
// Make sure these get called in an order which meets any interdependencies
// between the preprocessing requirements.
void cdr::CdrDoc::preProcess(bool validating)
{
    updateProtocolStatus(validating);
    stripXmetalPis(validating);
    genFragmentIds();
}

// Eliminate elements supplied by the template which the user has decided
// not to use.  See the XSL/T script itself for documentation of the logic.
void cdr::CdrDoc::stripXmetalPis(bool validating)
{
    // Only do this when the user has requested validation during a save.
    if (!validating)
        return;

    // Don't touch certain doc types
    if (TextDocType == L"Filter" ||
        TextDocType == L"css"    ||
        TextDocType == L"schema")
        return;

    cdr::FilterParmVector pv;
    cdr::String newXml;
    cdr::String errorStr;
    try {
        newXml = cdr::filterDocumentByScriptTitle(Xml,
                                                  L"Strip XMetaL PIs",
                                                  docDbConn, &errorStr, &pv);

        // Save the transformation for later steps.
        Xml = newXml;

        // If there is a stored parse tree for the document
        //   make sure it is not re-used since document may have changed
        parsed = false;
        revisedXml = "";

        // Save any warnings.
        if (!errorStr.empty())
            errList.push_back(L"CdrDoc::stripXmetalPis: " + errorStr);
    }
    catch (cdr::Exception& e) {
        errList.push_back(
                L"Failure stripping XMetaL processing instructions: " +
                cdr::String(e.what()));
    }
}

// Generate unique fragment identifiers for all elements which can have a
//   cdr:id attribute, but don't have one.
// This may replace the passed in XML with new XML that has
//   auto-generated cdr:id attributes in some fields
void cdr::CdrDoc::genFragmentIds ()
{
    cdr::String xslt;       // XSLT transform for creating fragment ids
    cdr::String newXml;     // Document XML after fragment id's are entered


    // If we don't have an XSL transform for this document type yet,
    //   create one.
    // XXXX Current version always creates it.
    //      Might optimize in future by caching them - which would
    //      save the time needed to retrieve and parse the schema, as
    //      well as to generate the script
    xslt = createFragIdTransform (docDbConn, schemaDocId);

    // If the script isn't null
    if (xslt.size() > 0) {

        // Execute it
        cdr::String filterMsgs = L"";
        try {
            newXml = cdr::filterDocument (Xml, xslt, docDbConn,
                                          &filterMsgs);

            // If there is a stored parse tree for the document
            //   make sure it is not re-used since document may have changed
            parsed = false;
            revisedXml = "";
        }
        catch (cdr::Exception& e) {
            // Add an error to the doc object
            errList.push_back (L"Generating fragment ids: " +
                               cdr::String (e.what()));

            // Serious error, revert to the unfiltered version
            newXml = Xml;
        }

        // If any messages returned, make them available for later viewing
        if (filterMsgs.size() > 0)
            errList.push_back (
                L"Generating fragment ids, filter produced these messages: " +
                filterMsgs);

        // Scan the new XML, replacing the arbitrary place holder
        //   "@@DummyCdrId@@" with sequenced numbers
        const wchar_t *srcp;                            // Source for copy
        wchar_t *destp;                                 // Destination for copy
        wchar_t *fixedXml = new wchar_t[newXml.size() + 1]; // Convert here
        srcp  = newXml.c_str();
        destp = fixedXml;
        while (*srcp) {
            if (*srcp != (wchar_t) '@')
                *destp++ = *srcp++;
            else {
                // If the @ begins our dummy string
                if (!wcsncmp (srcp, L"@@DummyCdrId@@", 14)) {

                    // Insert auto generated fragment id
                    swprintf (destp, L"_%d", ++lastFragmentId);

                    // Advance pointer past real attr value
                    while (*destp)
                        ++destp;

                    // Advance past dummy - a fixed size string
                    srcp += 14;
                }
                else
                    // '@' was part of the data
                    *destp++ = *srcp++;
            }
        }

        // Terminate the whole
        *destp = (wchar_t) '\0';

        // Replace the xml with the transformed version
        Xml = (cdr::String) fixedXml;

        // Release the temporary buffer
        delete [] fixedXml;
    }
    // Final update of modified lastFragmentId is done when the
    //   document is stored
}

// Helper function for updateProtocolStatus() below.
static cdr::String getLeadOrgStatus(const cdr::dom::Node& node,
                                    cdr::StringList& errList)
{
    // Find the LeadOrgProtocolStatuses element.
    cdr::dom::Node child = node.getFirstChild();
    cdr::String targetName = L"LeadOrgProtocolStatuses";
    while (child != 0 && targetName != child.getNodeName())
        child = child.getNextSibling();
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return L"";
    }

    // Find the CurrentOrgStatus element.
    child = child.getFirstChild();
    targetName = L"CurrentOrgStatus";
    while (child != 0 && targetName != child.getNodeName())
        child = child.getNextSibling();
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return L"";
    }

    // Find the ProtocolStatusName element.
    child = child.getFirstChild();
    targetName = L"StatusName";
    while (child != 0 && targetName != child.getNodeName())
        child = child.getNextSibling();
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return L"";
    }

    // Extract the text content.
    return cdr::dom::getTextContent(child);
}

// Implements requirement in Protocol Requirements Document 1.5 Status
// Setting.  Status is based on the status of the lead organization.
void cdr::CdrDoc::updateProtocolStatus(bool validating)
{
    // Nothing to do if this isn't a protocol document or if not validating.
    if (!validating || TextDocType != L"InScopeProtocol")
        return;

    // Get a parsed copy of the document so we can examine the org statuses.
    cdr::dom::Parser  parser;
    cdr::dom::Element docElement;
    try {
        parser.parse(Xml);
        docElement = parser.getDocument().getDocumentElement();
    }

    // Parsing will be tried again later, errors here are redundant
    catch (cdr::Exception&) {
        // Validation will catch the fact that the document is malformed.
        return;
    }
    catch (cdr::dom::XMLException &e) {
        // Eliminate warning on unused variable
        void *foo = (void *) &e;

        // Validation will catch the fact that the document is malformed.
        return;
    }

    // Find the statuses for the lead organizations.
    std::set<cdr::String> statusSet;
    cdr::dom::Node child = docElement.getFirstChild();
    cdr::String targetName = L"ProtocolAdminInfo";
    while (child != 0 && targetName != child.getNodeName()) {
        //errList.push_back(L"child name is " +
                //cdr::String(child.getNodeName()));
        child = child.getNextSibling();
    }
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return;
    }
    child = child.getFirstChild();
    targetName = L"ProtocolLeadOrg";
    while (child != 0) {
        if (targetName == cdr::String(child.getNodeName())) {
            cdr::String status = getLeadOrgStatus(child, errList);
            if (!status.empty())
                statusSet.insert(status);
        }
        child = child.getNextSibling();
    }

    // Decide which status the whole protocol should have.
    cdr::String status;
    if (statusSet.find(L"Active") != statusSet.end())
        status = L"Active";
    else {
        if (statusSet.size() > 1)
            errList.push_back(L"Status mismatch between Lead Organizations.  "
                              L"Status needs to be checked.");
        if (statusSet.find(L"Withdrawn") != statusSet.end())
            status = L"Withdrawn";
        else if (statusSet.find(L"Temporarily closed") != statusSet.end())
            status = L"Temporarily Closed";
        else if (statusSet.find(L"Completed") != statusSet.end())
            status = L"Completed";
        else if (statusSet.find(L"Closed") != statusSet.end())
            status = L"Closed";
        else if (statusSet.find(L"Approved-not yet active") != statusSet.end())
            status = L"Approved-not yet active";
        else {
            errList.push_back(L"No valid lead organization status found");
            status = L"No valid lead organization status found.";
        }
    }

    // Set the status for the protocol.
    cdr::FilterParmVector pv;        // Parameters passed to it
    pv.push_back (std::pair<cdr::String,cdr::String>(L"newStatus", status));
    cdr::String newXml;
    cdr::String errorStr;
    try {
        newXml = cdr::filterDocumentByScriptTitle(Xml,
                                                  L"Set Protocol Status",
                                                  docDbConn, &errorStr, &pv);

        // Save the transformation for later steps
        Xml = newXml;

        // If there is a stored parse tree for the document
        //   make sure it is not re-used since document may have changed
        parsed = false;
        revisedXml = "";
    }
    catch (cdr::Exception& e) {
        errList.push_back(L"Failure setting protocol status: " +
                          cdr::String(e.what()));
    }
}

/**
 * Create an XSLT script for inserting fragment identifier attributes
 * into a document.
 *
 */
static cdr::String createFragIdTransform (cdr::db::Connection& conn,
                                          int schemaDocId)
{
    // Output XSLT script includes these fixed portions
    static cdr::String s_xslScriptFront =
        L"<?xml version='1.0'?>\n"
        L"<xsl:transform xmlns:xsl = 'http://www.w3.org/1999/XSL/Transform'\n"
                       L"xmlns:cdr = 'cips.nci.nih.gov/cdr'\n"
                         L"version = '1.0'>\n"
        /*
        <!-- Identity rule, copies everything to the output tree. -->\nL"
        */
        L"<xsl:template match = '@*|node()'>\n"
        L" <xsl:copy>\n"
        L"  <xsl:apply-templates select = '@*|node()'/>\n"
        L" </xsl:copy>\n"
        L"</xsl:template>\n"

        /*
        <!-- For a specific set of the elements in the documents, if any
             has no cdr:id attribute, or has one with an empty value,
             add the attribute with a placeholder value.  Pass everything
             else through unscathed.
          -->
        */
        L"<xsl:template match = '";

    static cdr::String s_xslScriptBack =
        L"'>\n"
          L"<xsl:variable    name = 'cdrId'\n"
                           L"select = 'string(@cdr:id)'/>\n"
          L"<xsl:element     name = '{name()}'>\n"
           L"<xsl:if         test = 'not($cdrId)'>\n"
            L"<xsl:attribute name = 'cdr:id'>@@DummyCdrId@@</xsl:attribute>\n"
           L"</xsl:if>\n"
           L"<xsl:apply-templates select = '@*|node()'/>\n"
          L"</xsl:element>\n"
         L"</xsl:template>\n"
         /*
         <!-- Needed to prevent a cdr:id attribute which is present in the
              document, but with an empty value (cdr:id='') from overwriting
              the one we just generated.
           -->
         */
         L"<xsl:template     match = '@cdr:id'>\n"
          L"<xsl:if          test = 'string(.)'>\n"
           L"<xsl:copy/>\n"
          L"</xsl:if>\n"
         L"</xsl:template>\n"
        L"</xsl:transform>\n";


    // If there is no schema document id, just return an empty string
    if (schemaDocId == 0)
        return L"";

    // Retrieve the schema from the database
    std::string query = "SELECT xml FROM document WHERE id = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setInt(1, schemaDocId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"createFragIdTransform: Unable to load schema");
    cdr::String schemaXml = rs.getString (1);
    select.close();

    // Parse the schema, preparatory to creating a schema object
    cdr::dom::Parser parser;
    cdr::dom::Element schemaDocElem;
    try {
        parser.parse (schemaXml);
        schemaDocElem = parser.getDocument().getDocumentElement();
    }
    catch (const cdr::dom::XMLException& e) {
        throw cdr::Exception(L"createFragIdTransform: Unable to parse schema:"
                             + (cdr::String) e.getMessage());
    }

    // Create the object
    cdr::xsd::Schema schema (schemaDocElem, &conn);

    // Get a list of elements in the schema that can have a cdr:id attr
    cdr::StringList elemList;
    schema.elemsWithAttr (L"cdr:id", elemList);

    // If there are any, create the script
    cdr::String xslt = "";
    if (elemList.size() > 0) {

        // Beginning of the script
        xslt += s_xslScriptFront;

        // Add first field to the script
        cdr::StringList::const_iterator i = elemList.begin();
        xslt += *i++;

        // Add the rest of the fields, with '|' separator
        while (i != elemList.end()) {
            xslt += L"|";
            xslt += *i++;
        }

        // Rest of script
        xslt += s_xslScriptBack;
// DEBUG
// cdr::log::WriteFile (L"createFragIdTransforms", xslt);
    }

    // Return script, may be empty string
    return xslt;
}

/**
 * Delete all query terms from the database.
 *
 * Called before updating them, or when a doc is deleted.
 *
 *  @param conn         Connection string
 *  @param doc_id       Document id
 */

static void delQueryTerms (cdr::db::Connection& conn, int doc_id)
{
    cdr::db::Statement delStmt = conn.createStatement();
    char delQuery[80];
    sprintf(delQuery, "DELETE query_term WHERE doc_id = %d", doc_id);
    delStmt.executeUpdate(delQuery);
}

/**
 * Save one query term in the set of query terms for the document.
 *
 *  @param docId        document ID.
 *  @param path         Absolute XPath path (element and attr ids) to this
 *                      value.
 *  @param value        Value associated with the key.
 *  @param ordPosPathp  hex encoding of the node's position in the document.
 *  @param qtSet        set of query terms to which to add this term.
 */
void rememberQueryTerm(
        int docId,
        cdr::String& path, 
        cdr::String& value,
        wchar_t *ordPosPathp,
        cdr::QueryTermSet& qtSet
) {
    // If the value has any numerics in it, index them as numerics
    const wchar_t *p = value.c_str();
    cdr::Int intVal(true);

    while (*p) {
        if (iswdigit(*p)) {
            cdr::String temp (p);
            intVal = cdr::Int(temp.getInt());
            break;
        }
        ++p;
    }

    cdr::QueryTerm qt(docId, path, value, intVal, ordPosPathp);
    qtSet.insert(qt);
}

// Recursively collect all the query terms present in the document.
void cdr::CdrDoc::collectQueryTerms(
        const cdr::String& parentPath,
        const cdr::String& nodeName,
        const cdr::dom::Node& node,
        const cdr::StringSet& paths,
        wchar_t *ordPosPathp,
        int depth,
        QueryTermSet& qtSet)
{
    // Create full paths to check in the path lookup table
    cdr::String fullPath = parentPath + L"/" + nodeName;
    cdr::String wildPath = cdr::String (L"//") + nodeName;

    // Is either possible description of our current node in the table?
    if (paths.find(fullPath) != paths.end() ||
        paths.find(wildPath) != paths.end()) {

        // Add the absolute path to the content to the query table
        cdr::String value = cdr::dom::getTextContent(node);
        rememberQueryTerm(Id, fullPath, value, ordPosPathp, qtSet);
    }

    // Attributes of the node might also be indexed.
    // Check each of them
    cdr::dom::NamedNodeMap nMap = node.getAttributes();
    unsigned int len = nMap.getLength();
    for (unsigned int i=0; i<len; i++) {

        // Do we have an attribute?
        cdr::dom::Node aNode = nMap.item (i);
        if (aNode.getNodeType() == cdr::dom::Node::ATTRIBUTE_NODE) {

            // Search for it, and its wild counterpart
            cdr::String name = aNode.getNodeName();
            cdr::String fullAttrPath = fullPath + L"/@" + name;
            cdr::String wildAttrPath = cdr::String (L"//@") + name;

            if (paths.find(fullAttrPath) != paths.end() ||
                paths.find(wildAttrPath) != paths.end()) {
                    cdr::String value = aNode.getNodeValue();
                    rememberQueryTerm(Id, fullAttrPath, value,
                                      ordPosPathp, qtSet);
            }
        }
    }

    // Recursively add terms for sub-elements.
    cdr::dom::Node child = node.getFirstChild();

    // Sub-elements are numbered in the ordinal position path beginning at 0
    int ordPos = 0;

    // They are at a level one deeper than the passed level
    ++depth;
    if (depth >= cdr::MAX_INDEX_ELEMENT_DEPTH)
        throw cdr::Exception (
                L"CdrDoc: Indexing element beyond max allowed depth");

    while (child != 0) {

        // If child is an element
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {

            // Construct the ordinal position path
            swprintf (ordPosPathp + (depth * cdr::INDEX_POS_WIDTH), L"%0*X",
                      cdr::INDEX_POS_WIDTH, ordPos);
            collectQueryTerms (fullPath, child.getNodeName(), child, 
                               paths, ordPosPathp, depth, qtSet);
            ++ordPos;
        }

        child = child.getNextSibling();
    }
}

/*
 * Guards against creation of documents with duplicate titles for special
 * document types.
 *
 *  @param  conn        reference to database connection object.
 *  @param  id          id of document being stored (0 for new document).
 *  @param  docType     reference to string representation of document's type.
 *  @param  title       reference to title string for document being stored.
 *
 *  @throws             cdr::Exception if database error encountered or
 *                      attempt is made to store an illegal document for
 *                      a type which does not allow duplicate titles (e.g.
 *                      Filters and Schemas).
 */
void checkForDuplicateTitle(
        cdr::db::Connection& conn, 
        int id,
        const cdr::String& docType,
        const cdr::String& title)
{
    std::string query = "SELECT COUNT(*)  "
                        "  FROM document  "
                        " WHERE title = ? ";
    if (id)
        query += " AND id <> ?";
    cdr::db::PreparedStatement s = conn.prepareStatement(query);
    s.setString(1, title);
    if (id)
        s.setInt(2, id);
    cdr::db::ResultSet r = s.executeQuery();
    if (!r.next())
        throw cdr::Exception(L"Internal error checking for duplicate title");
    int count = r.getInt(1);
    if (count > 0)
        throw cdr::Exception(L"Duplicate title (" + title + L"); not allowed "
                             L"for documents of type " + docType);
}
