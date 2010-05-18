/**
 * CdrDoc.cpp
 *
 * Implementation of adding or replacing a document in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id$
 *
 * BZIssue::4767
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
#include "CdrBlobExtern.h"
#include "CdrGetDoc.h"
#include "CdrLink.h"
#include "CdrValidateDoc.h"
#include "CdrXsd.h"

#include <set>

#include <iostream>
#include <string>
// Uncomment following line to activate timing code.
// #define CDR_TIMINGS 1
#include "CdrTiming.h"

static cdr::String cdrPutDoc (cdr::Session&, const cdr::dom::Node&,
                              cdr::db::Connection&, bool);

static int findHighestFragmentId (cdr::String& xml);

static void auditDoc (cdr::db::Connection&, int, int, cdr::String,
                      cdr::String, cdr::String);

static void delQueryTerms (cdr::db::Connection&, int, char *);

static cdr::String createFragIdTransform (cdr::db::Connection&, int);

static void rememberQueryTerm(int, cdr::String&, cdr::String&, wchar_t*,
                              cdr::QueryTermSet&);

static void checkForDuplicateTitle(cdr::db::Connection&, int,
                                   const cdr::String&,
                                   const cdr::String&);

static int deleteDoc(cdr::Session&, int, bool, const cdr::String&,
                     cdr::StringList&, cdr::db::Connection&);

static cdr::String stripCdrEidAttrs(cdr::String &, cdr::db::Connection &);

static cdr::String addCdrEidAttrs(cdr::String& docStr);

// For debugging only
void saveXmlToFile(char *msg, char *fname, cdr::String xml) {
    try {
        FILE *fp;
        std::string xmlStr;
        size_t xsize;
        if ((fp = fopen(fname, "ab")) != NULL) {
            fprintf (fp, "Saving record: %s\n", msg);
            xmlStr = xml.toUtf8();
            xsize  = xmlStr.size();
            fwrite(xmlStr.c_str(), 1, xsize, fp);
            fclose(fp);
        }
    }
    catch (...) {
        perror("Unable to write xml");
    }
}



// String constants used as substitute titles
#define MALFORMED_DOC_TITLE L"!Malformed document - no title can be generated"
#define NO_TITLE_AVAILABLE  L"!No title available for this document"

const int MAX_SQLSERVER_INDEX_SIZE = 800;

// Constructor for a CdrDoc
// Extracts the various elements from the CdrDoc for database update
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    cdr::dom::Node& docDom,
    cdr::Session& session,
    bool withLocators
) : docDbConn (dbConn),
    comment (true),
    blobData (true),
    valDate (true),
    parsed (false),
    malformed (false),
    Id (0),
    DocType (0),
    activeStatus (L"A"),
    dbActiveStatus (L"A"),
    verPubStatus (unknown),
    valStatus (L"U"),
    Xml (L""),
    schemaDocId (0),
    lastFragmentId (0),
    blobId(0),
    revisedXml (L""),
    revFilterFailed (false),
    revFilterLevel (DEFAULT_REVISION_LEVEL),
    needsReview (false),
    titleFilterId (0),
    title (L""),
    conType (not_set)
{
    MAKE_TIMER(ctorDocTimer);
    // Get doctype and id from CdrDoc attributes
    const cdr::dom::Element& docElement(docDom);
    textDocType = docElement.getAttribute (L"Type");
    if (textDocType.size() == 0)
        throw cdr::Exception (L"CdrDoc: Doctag missing 'Type' attribute");
    String strFilterLevel = docElement.getAttribute(L"RevisionFilterLevel");
    revFilterLevel = strFilterLevel.getInt();
    if (!revFilterLevel)
        revFilterLevel = DEFAULT_REVISION_LEVEL;

    // Get the document ID if present in the transaction
    textId = docElement.getAttribute (L"Id");

    // If Id was passed, document must exist in database.
    if (textId.size() > 0) {
        Id = textId.extractDocId ();

        // Does the document exist?
        // Find out and also get info that must come from the database
        //   active_status
        //   last_fragment_id
        // This overrides any default initializations
        std::string qry =
          "SELECT active_status, last_frag_id FROM document WHERE Id = ?";
        cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
        select.setInt(1, Id);
        cdr::db::ResultSet rs = select.executeQuery();
        if (!rs.next()) {
            select.close();
            throw cdr::Exception(L"CdrDoc: Unable to find document " + textId);
        }
        dbActiveStatus = rs.getString (1);
        lastFragmentId = rs.getInt (2); // XXX See findHighestFragmentId below
        select.close();

        // We know the original status, here's a copy that might change
        activeStatus = dbActiveStatus;
    }
    else
        Id = 0;

    MAKE_TIMER(checkDocTypeTimer);
    // Check DocType - must be one of recognized types
    // Get name of XSLT filter to generate title at the same time
    // And get document id of the schema for this doc type
    std::string qry = "SELECT id, title_filter, xml_schema "
                      "  FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
    select.setString (1, textDocType);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next()) {
        select.close();
        throw cdr::Exception(L"CdrDoc: DocType '" + textDocType +
                             L"' is invalid");
    }
    DocType       = rs.getInt(1);
    cdr::Int tfi  = rs.getInt(2);
    cdr::Int sdi  = rs.getInt(3);
    select.close();
    SHOW_ELAPSED("Checking doctype", checkDocTypeTimer);

    // There may not be title filter and/or schemas for all doc types,
    //   so we need to check
    if (tfi.isNull())
        titleFilterId = 0;
    else
        titleFilterId = tfi;
    if (sdi.isNull())
        schemaDocId = 0;
    else
        schemaDocId = sdi;

    // Pull out the parts of the document from the dom
    MAKE_TIMER(parseCdrDocCtlTimer);
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

                        // XXX: Hopefully temporary code
                        // Due to fubar factors, CdrValidateDoc transactions
                        //   don't currently have an Id attribute in the
                        //   CdrDoc element.
                        // They use a DocCtl/DocId subelement instead
                        // The processing done above on textId is,
                        //   fortunately, unnecessary for validateDoc calls
                        //   so we can simplify our temporary workaround
                        //   by picking up the docId here, where it's easily
                        //   found without re-traversing the tree.
                        // XXX: Hopefully temporary code
                        if (ctlTag == L"DocId") {
                            if (Id == 0) {
                                textId = cdr::dom::getTextContent (ctlNode);
                                if (textId.size() > 0)
                                    Id = textId.extractDocId();
                                else
                                    throw cdr::Exception(
                                            "No text in DocId element");
                            }
                        }

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
                            title = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocComment")
                            comment = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocActiveStatus") {
                            activeStatus = cdr::dom::getTextContent (ctlNode);
                            if (activeStatus != L"A" && activeStatus != L"I")
                                throw cdr::Exception(
                                   L"CdrDoc: Unrecognized DocActiveStatus '" +
                                   activeStatus + L"' - expecting 'A' or 'I'");
                        }
                        else if (ctlTag == L"DocNeedsReview") {
                            needsReview = cdr::ynCheck (
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

                        setXml(ctlNode.getNodeValue());
                        break;
                    }
                    ctlNode = ctlNode.getNextSibling ();
                }
            }

            else if (name == L"CdrDocBlob")
                blobData = session.getClientBlob();

            else
                // Nothing else is allowed
                throw cdr::Exception(L"CdrDoc: Unrecognized element '" +
                                     name + L"' in CdrDoc");
        }

        child = child.getNextSibling ();
    }
    SHOW_ELAPSED("Finished parsing CdrDocCtl", parseCdrDocCtlTimer);

    // No passed XML may be legal or may not, have to check other things
    if (Xml.size() == 0) {
        // Note: In developing the blob code I appear to have accidentally
        //       developed a pathological test case with a blob, no XML
        //       and no ID.  That should not happen in real usage, but is
        //       protected against with the following two tests - AHM.

        // If it's a new record (no Id), XML is required
        if (!Id)
            throw cdr::Exception(
                    "LCdrDoc: Can't construct new record with no XML");

        // If no passed XML must at least pass a blob
        if (blobData.isNull())
            throw cdr::Exception(
                    L"CdrDoc: Must have CdrDocXml or CdrDocBlob element");

        // XML stays the same, but fetch it from the database so we
        //   can validate, gen title, and do all other actions by
        //   current rules as caller might expect
        cdr::db::PreparedStatement getXML = docDbConn.prepareStatement(
            "SELECT xml FROM document WHERE id = ?");
        getXML.setInt(1, Id);
        cdr::db::ResultSet rs = getXML.executeQuery();
        if (!rs.next()) {
            getXML.close();
            throw cdr::Exception(L"CdrDoc: Unable to fetch document " +
                                  textId + L" for constructor");
        }
        setXml(rs.getString(1));
        getXML.close();
    }

    // Override database info about last fragment ID with an actual count.
    // Original code left in until we decide for sure we don't need it.
    // See also the other CdrDoc constructors.
    // AHM 2008-04-10
    MAKE_TIMER(fragIdTimer);
    lastFragmentId = findHighestFragmentId(Xml);
    SHOW_ELAPSED("Finding last fragment ID", fragIdTimer);

    // Add/replace cdr-eids if needed and remember whether we're using them
    setLocators(withLocators);

    // Generate a title, or use an existing one, or create an error title
    MAKE_TIMER(createTitleTimer);
    createTitle();
    SHOW_ELAPSED("Creating the title", createTitleTimer);

    SHOW_ELAPSED("constructing CdrDoc", ctorDocTimer);
} // CdrDoc ()


// Constructor for a CdrDoc
// Extracts the various elements from the database by document ID
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    int docId,
    bool withLocators
) : docDbConn (dbConn), Id (docId), revFilterLevel (DEFAULT_REVISION_LEVEL)
 {
    // Text version of identifier is from passed id
    textId = cdr::stringDocId (Id);

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
                        "                 r.doc_id"
                        "            FROM document d"
                        "            JOIN doc_type t"
                        "              ON t.id = d.doc_type"
                        " LEFT OUTER JOIN ready_for_review r"
                        "              ON r.doc_id = d.id"
                        "           WHERE d.id = ?";
    cdr::db::PreparedStatement select = docDbConn.prepareStatement(query);
    select.setInt(1, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next()) {
        select.close();
        throw cdr::Exception(L"CdrDoc: Unable to load document " + textId);
    }

    // Copy info into this object
    valStatus      = rs.getString (1);
    valDate        = rs.getString (2);
    title          = cdr::entConvert (rs.getString (3));
    dbActiveStatus = rs.getString (4);
    DocType        = rs.getInt (5);
    setXml(rs.getString (6));
    lastFragmentId = rs.getInt (7);
    comment        = cdr::entConvert (rs.getString (8));
    textDocType    = rs.getString (9);
    cdr::Int tfi   = rs.getInt (10);
    cdr::Int sdi   = rs.getInt (11);
    cdr::Int rvRdy = rs.getInt (12);
    select.close();

    // Override database info about last fragment ID with an actual count.
    // Original code left in until we decide for sure we don't need it.
    // See also the other CdrDoc constructors.
    // AHM 2008-04-10
    lastFragmentId = findHighestFragmentId(Xml);

    // Modifiable copy of status, must be initialized
    activeStatus = dbActiveStatus;

    // Content or control type document
    conType = not_set;

    // Add/replace cdr-eids if needed and remember whether we're using them
    setLocators(withLocators);

    // Don't know yet if this is a publishable version
    // Won't check database to find out unless we need that information
    verPubStatus = unknown;

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
        needsReview = false;
    else
        needsReview = true;

    // Only need a blob id if we add a new blob
    blobId = 0;

    // We haven't filtered this for insertion, deletion markup
    revisedXml = L"";

    // We have not parsed the xml yet and don't know of anything wrong with it
    parsed    = false;
    malformed = false;

    // Haven't filtered it either
    revFilterFailed = false;
}

// Constructor for a CdrDoc
// Extracts xml from the doc_version table
// NOTE BENE:
//    This version was written for re-indexing publishable versions
//    I have not attempted to fill out every possible field at this time -
//    Maybe I'll do that in the future.
//    In the meantime, check all fields below before using this for other
//    purposes to be sure the CdrDoc fields you need are properly set.
//       AHM
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    int docId,
    int verNum,
    bool withLocators
) : docDbConn (dbConn),
    comment (true),
    blobData (true),
    valDate (true),
    parsed (false),
    malformed (false),
    Id (docId),
    DocType (0),
    activeStatus (L"A"),
    dbActiveStatus (L"A"),
    verPubStatus (unknown),
    valStatus (L"U"),
    Xml (L""),
    schemaDocId (0),
    lastFragmentId (0),
    blobId(0),
    revisedXml (L""),
    revFilterFailed (false),
    revFilterLevel (DEFAULT_REVISION_LEVEL),
    needsReview (false),
    titleFilterId (0),
    title (L""),
    conType (not_set)
{
    // Get info from version control
    std::string qry =
        "SELECT v.val_status, v.val_date, v.publishable, v.title, v.xml, "
        "       v.doc_type, t.name "
        "  FROM doc_version v "
        "  JOIN doc_type t "
        "    ON v.doc_type = t.id "
        " WHERE v.id = ? AND v.num = ?";

    cdr::db::PreparedStatement stmt = docDbConn.prepareStatement(qry);
    stmt.setInt(1, Id);
    stmt.setInt(2, verNum);
    cdr::db::ResultSet rs = stmt.executeQuery();

    if (!rs.next()) {
        stmt.close();
        throw cdr::Exception(L"CdrDoc: Unable to load document " + textId);
    }

    // Copy info into this object
    valStatus    = rs.getString(1);
    valDate      = rs.getString(2);
    verPubStatus = (rs.getString(3) == L"Y") ? publishable : nonPublishable;
    title        = cdr::entConvert (rs.getString(4));
    setXml(rs.getString(5));
    DocType      = rs.getInt(6);
    textDocType  = rs.getString(7);
    stmt.close();

    // Will we use cdr-eid attributes with this doc?
    setLocators(withLocators);
}

/**
 * Install XML in the object.
 * If it's different, discard anything generated from the earlier version.
 */
void cdr::CdrDoc::setXml (
    cdr::String newXml
) {
    // If anything changed, discard dependent objects
    if (newXml != Xml) {
        // Discards
        parsed     = false;
        revisedXml = L"";

        // Put the new Xml in place
        Xml = newXml;
    }

    // Otherwise we're already there.  Do nothing.
}

/**
 * Store a document.
 * Assumes all checking already done in cdrPutDoc()
 * Throws exception if failed.
 *
 */
void cdr::CdrDoc::store ()
{
    std::string sqlStmt;    // SQL for update
    cdr::String storeXml;   // XML string to store in the document table

    // Make sure the client program hasn't tampered with the revision
    // filter level.
    if (getRevFilterLevel() != DEFAULT_REVISION_LEVEL)
        throw cdr::Exception(L"CdrDoc::store: RevisionFilterLevel cannot be "
                             L"overridden when saving a CDR document.");

    storeXml = Xml;
    if (isContentType()) {
        // Filter the document to remove any cdr-eid attributes
        // These should not be stored.
        // Error IDs are never allowed in non-content type documents, so
        //   we check above to avoid possible mangling of fancy formatting
        //   in schemas, filters, or css objects.
        storeXml = stripCdrEidAttrs(Xml, docDbConn);
    }

    // New record
    if (!Id) {
        sqlStmt =
            "INSERT INTO document ("
            "   val_status,"
            "   active_status,"
            "   doc_type,"
            "   title,"
            "   xml,"
            "   comment,"
            "   last_frag_id"
            ") VALUES (?, ?, ?, ?, ?, ?, ?)";
    }

    // Existing record
    else {
        sqlStmt =
            "UPDATE document "
            "  SET val_status = ?,"
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
    docStmt.setString (1, valStatus);
    docStmt.setString (2, activeStatus);
    docStmt.setInt    (3, DocType);
    docStmt.setString (4, title);
    docStmt.setString (5, storeXml);
    docStmt.setString (6, comment);
    docStmt.setInt    (7, lastFragmentId);

    // For existing records, fill in ID
    if (Id) {
        docStmt.setInt (8, Id);
    }

    // Do it
    docStmt.executeUpdate ();

    // If new record, find out what was the key
    if (!Id) {
        Id = docDbConn.getLastIdent ();

        // Convert id to wide characters
        char idbuf[32];
        sprintf (idbuf, "CDR%010d", Id);
        textId = idbuf;
    }

    // If there was a blob, store it
    // setBlob() knows how and what to do for any circumstance
    // If we need a version, it's handled by caller
    if (!blobData.isNull())
        blobId = setBlob(docDbConn, blobData, Id);

    // Update ready_for_review table indicating that initial mailers
    //   may be sent for this document (the physician, organization or
    //   protocol person can "review" the doc now.)
    // Our original conception was that documents could be made
    //   ready, or not, hence code was written for each kind of
    //   update.
    // Our new conception is that once made ready, a document will
    //   never be made unready except in extraordinary circumstances.
    //   Hence deleting the flag is removed from the code as a simple
    //   way to prevent client programs from accidentally deleting it.
    if (needsReview) {
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
            // There was no significant performance advantage to
            //   checking first
            ;
        }
    }
#if WE_EVER_DELETE_ready_for_review_FLAGS
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
#endif

} // store

// Set the flag to say if error id locators will be used
bool cdr::CdrDoc::setLocators(
    bool locators
) {
    // This call can only be made after the document type is known
    if (DocType == 0)
        throw cdr::Exception(
            L"Attempted CdrDoc::setLocators before doctype is established");

    // Locators may only be used on content type documents
    if (isControlType())
        // Whatever caller wanted, we won't put in locators
        locators = false;

    // Set the flag
    valCtl.setLocators(locators);

    // Add the cdr-eid attributes, if required
    if (locators)
        setXml(createErrorIdXml());

    // May return something different from what caller passed
    return locators;
}

/**
 * Add a new document to the database.
 * A front end to cdrPutDoc.
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
    // Call cdrPutDoc with flag indicating new document.
    // Return result.
    return cdrPutDoc (session, node, conn, 1);
}


/**
 * Update a document in the database by replacing it.
 * A front end to cdrPutDoc.
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
    // Call cdrPutDoc with flag indicating we have an existing document.
    // Return result.
    return cdrPutDoc (session, node, conn, 0);
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

static cdr::String cdrPutDoc (
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

    // Assume validation does not use cdr-eid error locators
    bool withLocators = false;

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

                    // User may also request cdr-eid error locator attributes
                    if ((attr = attrMap.getNamedItem ("ErrorLocators"))
                             != NULL)
                        withLocators = cdr::ynCheck(attr.getNodeValue(),
                             withLocators,
                             L"ErrorLocators attribute on Validate element");
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

    // Construct a doc object containing all the data
    // The constructor may look for additonal elements in the DocCtl
    //   not parsed out by the above logic
    if (docNode == 0)
        throw cdr::Exception(L"cdrPutDoc: No 'CdrDoc' element in transaction");

    // Create the doc object
    // Note locators won't be added to control type docs, whatever we pass
    cdr::CdrDoc doc (dbConn, docNode, session, withLocators);
    SHOW_ELAPSED("CdrDoc constructed", incrementalTimer);

    // filter level.
    if (doc.getRevFilterLevel() != cdr::DEFAULT_REVISION_LEVEL)
        throw cdr::Exception(L"cdrPutDoc: RevisionFilterLevel cannot be "
                             L"overridden when saving a CDR document.");

    // Make sure validation has been invoked if a publishable version
    // has been requested.  The DLL will enforce this, but we do it
    // here as well, since the DLL is our primary, but not our only client.
    if (cmdPublishVersion && (!cmdSchemaVal || !cmdLinkVal)
                          && doc.isContentType())
        throw cdr::Exception(L"cdrPutDoc: creation of a published version "
                             L"not allowed unless full document validation is "
                             L"invoked.");

    // Check user authorizations
    cdr::String action = newrec ? L"ADD DOCUMENT" : L"MODIFY DOCUMENT";
    cdr::String doctype = doc.getTextDocType();
    if (!session.canDo (dbConn, action, doctype))
        throw cdr::Exception (L"cdrPutDoc: User '" + session.getUserName() +
                              L"' not authorized to '" + action +
                              L"' with docs of type '" + doctype + L"'");

    // Make sure user is authorized for the desired active status.
    // Note that docs found via the DOCUMENT view can't have status='D'elete
    if (doc.getDbActiveStatus() != doc.getActiveStatus()
            && !session.canDo(dbConn, L"PUBLISH DOCUMENT", doctype)) {
        if (doc.getActiveStatus() == L"I")
            throw cdr::Exception(L"User not authorized to block "
                                 L"publication for this document");
        else if (doc.getActiveStatus() == L"A")
            throw cdr::Exception(L"User not authorized to remove "
                                 L"publication block for this document");
    }

    // Make sure user is authorized to create the first publishable version.
    if (cmdPublishVersion) {
        static const char* const query = "SELECT COUNT(*)         "
                                         "  FROM doc_version      "
                                         " WHERE id = ?           "
                                         "   AND publishable = 'Y'";
        cdr::db::PreparedStatement st = dbConn.prepareStatement(query);
        st.setInt(1, doc.getId());
        cdr::db::ResultSet rs = st.executeQuery();
        if (!rs.next()) {
            st.close();
            throw cdr::Exception(L"Failure determining publishable version "
                                 L"count for document");
        }
        int nPublishableVersions = rs.getInt(1);
        st.close();
        if (!nPublishableVersions
                && !session.canDo(dbConn, L"PUBLISH DOCUMENT", doctype))
            throw cdr::Exception(L"User not authorized to create the first "
                                 L"publishable version for this document");
    }
    SHOW_ELAPSED("permissions checked", incrementalTimer);

    // Block creation of documents with duplicate titles for special document
    // types.
    if (doc.isControlType())
        checkForDuplicateTitle(dbConn, doc.getId(), doctype, doc.getTitle());

    // Does document id attribute match expected action type?
    if (doc.getId() && newrec)
        throw cdr::Exception (L"cdrPutDoc: Attempt to add doc with "
                              L"existing ID as new");
    else if (!doc.getId() && !newrec)
        throw cdr::Exception (L"cdrPutDoc: Attempt to replace doc "
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
        doc.store ();

        // Some docs might incorporate the docId into the title,
        //   but the docId didn't exist until the store was done.
        // Check and update the title if need.
        doc.checkTitleChange();

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
                if (!rs.next()) {
                    stmt.close();
                    throw cdr::Exception (L"Document is checked out but "
                                          L"user not identified!");
                }

                // Report who has it, and when
                // Give brief user name and full name ifwe have it
                cdr::String lockUserName     = rs.getString (1);
                cdr::String lockUserFullName = rs.getString (2);
                stmt.close();
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
    // But only validate content type documents, not control documents
    if (cmdValidate && doc.isContentType()) {
        if (cmdLinkVal && !cmdSetLinks)
            // If not setting links, can't do any link validation because
            //   it automatically does a link_net update
            throw cdr::Exception (
                    L"Validation of links without SetLinks is currently "
                    L"unsupported.  Sorry.");

        // Perform validation
        // execValidateDoc will set valStatus in the doc object
        // Will overwrite whatever is there
        cdr::execValidateDoc (&doc,cdr::UpdateUnconditionally,validationTypes);

        if (cmdPublishVersion) {
            // Set publishable status based on results
            // Note: Logic above has forced both schema and link validation
            //       or we won't get this far
            if (doc.getValStatus() == L"V")
                doc.setVerPubStatus(cdr::publishable);

            else {
                // Doc failed validation
                doc.setVerPubStatus(cdr::nonPublishable);

                // If the user was hoping for a publishable version,
                //   set him straight.
                doc.addError(L"Non-publishable version will be created.");

                // This produces an <Err> return, but it's not a validation
                //   error.
                doc.getValCtl().setLastErrorType(cdr::ETYPE_OTHER);
                doc.getValCtl().setLastErrorLevel(cdr::ELVL_WARNING);

                cmdPublishVersion = false;
                cmdVersion = true;
            }
        }
        SHOW_ELAPSED("validation completed", incrementalTimer);
    }
    else
        // Document is unvalidated.
        // Set status accordingly.  Leaves val_date alone.
        doc.setValStatus (L"U");

    if (cmdSetLinks && !cmdLinkVal && doc.isContentType()) {
        // Update of link tables is part of unconditional link validation
        // But if not done there, we have to do it here unless it was
        //   specifically suppressed with the SetLinks command.
        // Only do it if it's a content type doc, not a control doc
        // Call to parseAvailable is done first to check if the document
        //   was already parsed, or if not, parse it.  It also
        //   handles revision filtering before the parse.
        if (doc.parseAvailable())
            cdr::link::cdrSetLinks (cdr::dom::Node(doc.getDocumentElement()),
                                    dbConn,
                                    doc.getId(), doc.getTextDocType(),
                                    cdr::UpdateLinkTablesOnly,
                                    doc.getValCtl());
        SHOW_ELAPSED("setting links", incrementalTimer);
    }

    // If we haven't already done so, store it
    if (!newrec) {
        doc.store ();
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
              L"cdrPutDoc", cmdReason);
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
                     + doc.getErrorXml();

    // If we are processing a transaction that included validation,
    //   and if the caller requested location information of errors,
    //   and if there were errors,
    // then:
    //   Be sure that we echo back the document with cdr-eid attributes.
    if (cmdValidate && doc.hasLocators() && doc.getErrorCount() > 0
                    && doc.isContentType())
        cmdEcho = true;

    if (cmdEcho) {

        // Old way fetched doc from the database, via denormalization
        // resp+=cdr::getDocString(doc.getTextId(), dbConn, true, true) + L"\n";

        // New way has to use the in-memory version, with error id attrs,
        //   and simple denormalized content.
        resp += doc.getSerialXml();
    }

    resp += L"  </Cdr" + rtag + L"DocResp>\n";
    SHOW_ELAPSED("command response ready", incrementalTimer);
    SHOW_ELAPSED("total elapsed time for cdrPutDoc", putDocTimer);

    /*
    // Uncomment this to write the response to stdout for debugging
    const wchar_t *xcstr = resp.c_str();
    wchar_t *xpos = (wchar_t *) xcstr;
    while (*xpos) {
      std::wcout << *xpos;
      ++xpos;
    }
    std::wcout << std::endl;
    */

    return resp;

} // cdrPutDoc


// Access count of error messages in ValidationControl
int cdr::CdrDoc::getErrorCount() const
{
    return valCtl.getErrorCount();
}

/*
 * Produce a serialized version of the XML document, including
 * some standard CdrDoc, CdrDocCtl, and CdrDocXml elements as used
 * in GetCdrDoc.getDocString() functions.
 *
 * See NOTES in CdrDoc.h.
 */
cdr::String cdr::CdrDoc::getSerialXml()
{
    // Denormalize the data - a service to users
    cdr::String denormXml;
    try {
        denormXml = cdr::filterDocumentByScriptTitle(Xml,
                               L"Fast Denormalization Filter", docDbConn);
    }
    catch (cdr::Exception&) {
        // Can't do it, just return what we had
        denormXml = Xml;
    }

    // Our convention is to include cdr-eids if they are present and there
    //   are errors that might refer to them.
    // Else we strip out any cdr-eids
    if (hasLocators()) {
        if (getErrorCount() == 0)
            denormXml = stripCdrEidAttrs(denormXml, docDbConn);
    }

    // Build the CdrDoc string.
    cdr::String docStr = cdr::String(L"<CdrDoc Type='")
                       + textDocType
                       + L"' Id='" + textId + L"'>\n"
                       + L"<CdrDocCtl>\n"
                       + L" <DocTitle>" + title + L"</DocTitle>\n"
                       + L"</CdrDocCtl>\n"
                       + cdr::makeDocXml(denormXml)
                       + L"\n</CdrDoc>\n";

    return docStr;

} // getSerialXml


/**
 * Internal function for deleting a document.  Used by the CdrDelDoc command,
 * as well as by the CdrMailerCleanup command.
 */
int deleteDoc (
    cdr::Session&        session,
    int                  docId,
    bool                 validate,
    const cdr::String&   reason,
    cdr::StringList&     errList,
    cdr::db::Connection& conn
) {
    // Check added to make sure the document isn't published.
    cdr::String docIdString = cdr::stringDocId(docId);
    std::string pQry = "SELECT COUNT(*) FROM pub_proc_cg WHERE id = ?";
    cdr::db::PreparedStatement pSel = conn.prepareStatement (pQry);
    pSel.setInt(1, docId);
    cdr::db::ResultSet pRs = pSel.executeQuery();
    if (!pRs.next())
        throw cdr::Exception(L"deleteDoc: Unable to check publication for " +
                             docIdString);
    int count = pRs.getInt(1);
    pSel.close();
    if (count > 0)
        throw cdr::Exception(L"deleteDoc: Cannot delete published doc " +
                             docIdString);

    // Check added to make sure the document isn't used by the
    // external_map table.
    std::string eQry = "SELECT COUNT(*) FROM external_map WHERE doc_id = ?";
    cdr::db::PreparedStatement eSel = conn.prepareStatement(eQry);
    eSel.setInt(1, docId);
    cdr::db::ResultSet eRs = eSel.executeQuery();
    if (!eRs.next())
        throw cdr::Exception(L"deleteDoc: Unable to check external_map for " +
                             docIdString);
    int eCount = eRs.getInt(1);
    eSel.close();
    if (eCount > 0)
        throw cdr::Exception(L"deleteDoc: cannot delete " + docIdString +
                             L", which is in the external mapping table");

    // From now on, do everything or nothing
    // setAutoCommit() checks state first, so it won't end an existing
    //   transaction
    bool autoCommitted = conn.getAutoCommit();
    conn.setAutoCommit(false);

    // Has anyone got this document checked out?
    // If not, that's okay.  We don't require a check-out in order to
    //   delete a document - as long as no one else has it checked out.
    // However if it is checked out we'll check it in.  Won't allow
    //   a deleted document to stay checked out.
    int outUserId;
    int userId = session.getUserId();
    if (cdr::isCheckedOut(docId, conn, &outUserId)) {
        if (outUserId != userId)
            throw cdr::Exception (L"Document " + cdr::stringDocId(docId) +
                                  L" is checked out by another user");

        // If it's checked out, we'll check it in
        // No version is created in version control
        cdr::checkIn (session, docId, conn, L"I", &reason, true, false);
    }

    // Update the link net to delete links from this document
    // If validation was requested, we tell cdrDelLinks to abort if
    //   there are any links _to_ this document
    cdr::ValidRule vRule = validate ? cdr::UpdateIfValid :
                                      cdr::UpdateUnconditionally;
    int errCount = cdr::link::cdrDelLinks (conn, docId, vRule, errList);

    // Proceed if no errors, or we don't care if there were errors
    if (errCount == 0 || vRule == cdr::UpdateUnconditionally) {

        // Proceed with deletion
        // Begin by deleting all query terms pointing to this doc
        delQueryTerms (conn, docId, "query_term");
        delQueryTerms (conn, docId, "query_term_pub");

        // The document itself is not removed from the database, it
        //   is simply marked as deleted
        // Note that if document is already deleted, we silently do
        //   nothing
        std::string dQry = "UPDATE document "
                           "   SET active_status = 'D' "
                           " WHERE id = ?";
        cdr::db::PreparedStatement dSel = conn.prepareStatement (dQry);
        dSel.setInt (1, docId);
        dSel.executeUpdate();
        dSel.close();

        // Record what we've done
        auditDoc (conn, docId, userId, L"DELETE DOCUMENT",
                  L"CdrDelDoc", reason);
    }

    // Restore autocommit status if required
    conn.setAutoCommit(autoCommitted);

    // Tell how many errors we encountered.
    return errCount;
}


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
    cdr::String cmdReason,      // Reason to associate with document deletion
                docIdStr,       // Doc id in string form
                docTypeStr;     // Doc type in string form
    int         docId,          // Doc id as unadorned integer
                docType,        // Document type
                errCount;       // Number of link releated errors
    bool        cmdValidate;    // True=Validate that nothing links to doc
    cdr::dom::Node child,       // Child node in command
                   docNode;     // Node containing CdrDoc
    cdr::StringList errList;    // List of errors from cdrDelLinks


    // Default values, may be replace by elements in command
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

    // Perform the deletion.
    errCount = deleteDoc(session, docId, cmdValidate, cmdReason, errList,
                         conn);

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
    if (revisedXml.size() > 0) {
        data = revisedXml;
    }
    else if (Xml.size() > 0) {
        data = Xml;
    }
    else {
        // Nothing to build a tree from
        errList.push_back (L"No XML in this document to parse");
        return false;
    }

    // Build a parse tree, being sure memory is saved
    //   after the parser goes out of scope
    cdr::dom::Parser parser(true);
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
 * Remove cdr-eid attributes, if necessary, from XML.
 */
static cdr::String stripCdrEidAttrs(
    cdr::String         &xml,
    cdr::db::Connection &conn
) {
    // As a simple optimization, we'll only call this if we find a cdr-eid
    //   in a quick string scan
    if (wcsstr(xml.c_str(), L"cdr-eid") == NULL)
        return xml;

    // Else there are some, filter them out
    cdr::String errStr;
    cdr::FilterParmVector pv;
    cdr::String newXml = cdr::filterDocumentByScriptTitle (xml,
        L"Validation Error IDs: Delete all cdr-eid attributes",
        conn, &errStr, &pv);
    if (errStr.size() > 0)
        throw cdr::Exception(L"Error filtering out cdr-eids" + errStr);

    // Replace this objects Xml
    return newXml;
}

/**
 * Add cdr-eid ID attributes to each element for use
 * in validation.
 */
cdr::String cdr::CdrDoc::createErrorIdXml()
{
    cdr::String eidXml;

    // Invoke the filter that will strip any old error IDs and
    //   add new ones
    // We strip old ones because new elements may be added that don't
    //   have the markup
    // For now, we'll let any filter exception bubble up.  May change
    //   that later if need
    if (isContentType()) {
        MAKE_TIMER(errorIdTimer);
        eidXml = stripCdrEidAttrs(Xml, docDbConn);
        SHOW_ELAPSED("Delete cdr-eid attrs", errorIdTimer);
        MAKE_TIMER(errorIdAddTimer);
        eidXml = addCdrEidAttrs(eidXml);
        SHOW_ELAPSED("Add cdr-eid attrs", errorIdAddTimer);
    }

    return eidXml;
} // createErrorIdXml

/**
 * Return an XML string with revision markup filtered out as per the
 * standard techniques.
 */

cdr::String cdr::CdrDoc::getRevisionFilteredXml (
    bool getIfUnfiltered
) {
    // String to receive warnings, errors, etc. from XSLT
    cdr::String errorStr = L"";

    // Validate revision level.
    if (revFilterLevel < 1 || revFilterLevel > 3)
        throw cdr::Exception(L"CdrDoc::getRevisedXml: Illegal revision filter "
                           + cdr::String::toString (revFilterLevel)
                           + L" requested for document");

    // Need to filter revision markup if:
    //   This is a content (not a control) document
    //   Filter attempt has not already failed (!revFilterFailed)
    //   Any previous revision filtering discarded (...size() == 0)
    if (isContentType() && !revFilterFailed && revisedXml.size() == 0) {

        // Attempt to filter at the requested level
        cdr::FilterParmVector pv;        // Parameters passed to it
        pv.push_back (std::pair<cdr::String,cdr::String>
            (L"useLevel", cdr::String::toString (revFilterLevel)));

        try {
            MAKE_TIMER(revXmlTimer);
            revisedXml = cdr::filterDocumentByScriptTitle (
                    Xml, L"Revision Markup Filter",
                    docDbConn, &errorStr, &pv);
            SHOW_ELAPSED("Revision Filter Xml",
                         revXmlTimer);
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
            if (revisedXml.size() == 0) {
                // Didn't really succeed
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
        MAKE_TIMER(revFilterTimer);
        cdr::String xml = getRevisionFilteredXml(true);
        SHOW_ELAPSED("Applying revision filtering", revFilterTimer);

        // Generate title
        cdr::String filterTitle = L"";
        cdr::String filterMsgs  = L"";
        cdr::FilterParmVector pv;
        pv.push_back (std::pair<cdr::String,cdr::String>(L"docId", textId));
        try {
            MAKE_TIMER(applyTitleFilterTimer);
            filterTitle = cdr::filterDocumentByScriptId (
                    xml, titleFilterId, docDbConn, &filterMsgs, &pv, textId);
            SHOW_ELAPSED("Applying title filter", applyTitleFilterTimer);
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
            title = filterTitle;
    }

    // If we still have no title because:
    //   No title filter defined and no title supplied by other means, or
    //   Title filter ran but generated no output, or
    //   Title filter generated terminal error
    // Then
    //   Generate a default error title.
    if (title.size() == 0) {
        errList.push_back (
            L"No title available for document, using default error title");
        title = NO_TITLE_AVAILABLE;
    }
}

/**
 * Check to be sure a title doesn't need to change, or change it
 */
bool cdr::CdrDoc::checkTitleChange()
{
    // Save the current title
    cdr::String savedTitle = title;

    // Regenerate the title
    createTitle();

    // If it's different, re-save it in the database
    if (title != savedTitle) {
        std::string sqlStmt =
            "UPDATE document \n"
            "   SET title = ?\n"
            " WHERE id = ?\n";

        cdr::db::PreparedStatement docStmt =
            docDbConn.prepareStatement (sqlStmt);
        docStmt.setString (1, title);
        docStmt.setInt    (2, Id);
        docStmt.executeUpdate ();

        // Tell caller we changed it
        return true;
    }

    return false;
}

/**
 * Regenerate the title for a document identified by its ID.
 * See CdrCommand.h.
 */
cdr::String cdr::updateTitle (
    cdr::Session& session,
    const cdr::dom::Node& cmdNode,
    cdr::db::Connection& conn
) {
    cdr::dom::Node child;       // Child node in command
    cdr::String    docIdStr;    // ID in CDR000... format
    int            docIdNum;    // Same as numeric
    bool           changed;     // True = title changed

    // Parse command to get the document CDR ID
    child    = cmdNode.getFirstChild();
    docIdStr = L"";
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE &&
            child.getNodeName() == L"DocId") {
            docIdStr = cdr::trimWhiteSpace(cdr::dom::getTextContent(child));
            docIdNum = docIdStr.extractDocId();
            break;
        }
        child = child.getNextSibling();
    }

    // Transaction format check
    if (docIdStr == L"")
        throw cdr::Exception (
                L"updateTitle could not find DocId element in cmd");

    // Instantiate a CdrDoc for this document
    CdrDoc doc(conn, docIdNum, false);

    // Generate and, if necessary, update the title
    changed = doc.checkTitleChange();

    // Formulate response
    cdr::String result = changed ? L"changed" : L"unchanged";
    cdr::String response = L" <CdrUpdateTitleResp>" +
                           result +
                           L"</CdrUpdateTitleResp>\n";

    return response;
}


/**
 * Mark a document as malformed
 */

void cdr::CdrDoc::malFormed()
{
    // Mark this objec in memory
    malformed = true;

    // Set validation status in case doc is stored
    valStatus = L"M";

    // Title derives from the document, but we can't derive a title
    //   from a malformed document.
    // If there is a title, leave it alone, else see if there's a filter
    if (title.size() == 0) {
        if (titleFilterId == 0)
            title = NO_TITLE_AVAILABLE;
        else
            title = MALFORMED_DOC_TITLE;
    }
}


/**
 * Scan the xml for cdr:id attributes and find the highest numbered
 * one in our internal _nn* format.
 *
 * Newly assigned fragment ids will be numbered above this one.
 *
 * Pass:
 *    xml   - as a Unicode cdr::String.
 *
 * Return:
 *    Integer representation of the highest id found.
 */
static int findHighestFragmentId(
    cdr::String& xml
) {
    // We'll scan for this string in the XML
    // It is conceivable that the string could be found as data, which
    //   could, in vanishingly rare cases, cause a higher number than
    //   needed to be generated.  However no error will occur.
    static const wchar_t idAttrName[] = L" cdr:id";

    // The length of the attribute name plus a quote delimiter
    static const size_t  attrFixedLen = wcslen(idAttrName);

    // Track the scan with this
    const wchar_t *pXml = xml.c_str();
    const wchar_t *pXmlEnd = pXml + xml.length();

    // Largest found, to be returned to caller
    int maxFragId = 0;

    // Copy a digit string here for conversion to a number
    const int NUM_BUF_SIZE = 12;
    char numBuf[NUM_BUF_SIZE + 1];

    // Keep scanning the string until no more matches on the attr name
    while ((pXml = wcsstr(pXml, idAttrName)) != NULL) {

        // Point past the attribute name chars and other structure
        pXml += attrFixedLen;
        while (*pXml == L' ' || *pXml == L'\n')
            ++pXml;
        if (*pXml != L'=')
            continue;
        ++pXml;
        while (*pXml == L' ' || *pXml == L'\n')
            ++pXml;
        if (*pXml != L'\'' && *pXml != L'"')
            continue;
        ++pXml;

        // Is it in our format?
        if (*pXml++ != L'_')
            continue;

        // Copy digits up to max size, implicitly converting to ASCII
        size_t digitCount = 0;
        while (iswdigit(*pXml) && digitCount < NUM_BUF_SIZE)
            numBuf[digitCount++] = (char) *pXml++;

        // If no quote delimiter, this isn't one of ours
        if (*pXml != L'\'' && *pXml != L'"')
            continue;

        // Test the number
        numBuf[digitCount] = 0;
        int num = atoi(numBuf);
        if (num > maxFragId)
            maxFragId = num;
    }

    return maxFragId;
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
    if (!actRs.next()) {
        actStmt.close();
        throw cdr::Exception (L"auditDoc: Bad action string '"
                              + action
                              + L"'  Can't happen.");
    }
    actionId = actRs.getInt (1);
    actStmt.close();

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
    stmt.close();
}


/**
 * Changes the active_status column for a row in the all_docs table.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::setDocStatus(Session&         session,
                              const dom::Node& node,
                              db::Connection&  conn)
{
    int docId = 0;
    String docIdStr(true);
    String newStatus(true);
    String comment;

    // Extract the parameters.
    dom::Node child = node.getFirstChild();
    while (child != 0) {
        String name = child.getNodeName();
        if (name == L"DocId") {
            docIdStr = dom::getTextContent(child);
            docId = docIdStr.extractDocId();
        }
        else if (name == L"NewStatus")
            newStatus = dom::getTextContent(child);
        else if (name == L"Comment")
            comment = dom::getTextContent(child);
        child = child.getNextSibling();
    }
    if (docIdStr.isNull())
        throw Exception(L"Missing required document ID");
    if (newStatus.isNull())
        throw Exception(L"Missing required status");
    if (newStatus != L"I" && newStatus != L"A")
        throw Exception(L"Invalid status " + newStatus);
    if (comment.length() == 0)
        comment = newStatus == L"I" ? L"Blocking document"
                                    : L"Unblocking document";

    // Find out if the user is authorized to change the doc status.
    db::PreparedStatement s1 = conn.prepareStatement(
            "SELECT t.name            "
            "  FROM doc_type t        "
            "  JOIN all_docs d        "
            "    ON d.doc_type = t.id "
            " WHERE d.id = ?          ");
    s1.setInt(1, docId);
    db::ResultSet r1 = s1.executeQuery();
    if (!r1.next())
        throw Exception(L"Invalid document ID " + docIdStr);
    String docType = r1.getString(1);
    s1.close();
    if (!session.canDo(conn, L"PUBLISH DOCUMENT", docType))
        throw Exception(L"User not authorized to change the status of " +
                        docType + L" documents");

    // Record the action.
    auditDoc(conn, docId, session.getUserId(), L"MODIFY DOCUMENT",
             L"SetDocStatus", comment);

    // Do it.
    db::PreparedStatement s2 = conn.prepareStatement(
            "UPDATE all_docs          "
            "   SET active_status = ? "
            " WHERE id = ?            ");
    s2.setString(1, newStatus);
    s2.setInt   (2, docId);
    s2.executeUpdate();
    s2.close();
    conn.commit();

    // Report success.
    return L"<CdrSetDocStatusResp/>";
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

    // Is it a publishable version?
    if (doc.getVerPubStatus() == unknown) {
        if (isCWDLastPub(docId, dbConnection))
            doc.setVerPubStatus(publishable);
        else
            doc.setVerPubStatus(nonPublishable);
    }

    // Update the index table
    doc.updateQueryTerms();

    // If doc was not publishable, only query_term was updated, not
    //   query_term_pub.
    // Check for a publishable version and index it too
    if (doc.getVerPubStatus() != publishable) {
        int lastPubVer = cdr::getLatestPublishableVersion(docId, dbConnection);
        // If there was a publishable version
        if (lastPubVer > 0) {
            // Load it and index, but only for the query_term_pub table
            cdr::CdrDoc pubDoc(dbConnection, docId, lastPubVer);
            pubDoc.updateQueryTerms(false, true);
        }
    }

    // Failures are impossible - except for internal exceptions
    return (L"<CdrReindexDocResp/>");
}

/**
 * Gather all query terms from the database for either the current
 * working document, or the last publishable version.  They will later
 * be compared to terms in the new document to find changes.
 *
 * @param conn      Database connection.
 * @param termSet   Reference to STL set in which to store the terms.
 * @param docId     CDR ID for doc
 * @param tblName   "query_term" or "query_term_pub" naming the stored
 *                  collection to retrieve.
 */
static void getExistingTerms(
    cdr::db::Connection& conn,
    cdr::QueryTermSet& termSet,
    int docId,
    char* tblName
) {
    std::ostringstream qry;
    qry << "SELECT path, value, int_val, node_loc FROM "
        << tblName
        << " WHERE doc_id="
        << docId;

    cdr::db::Statement selTerms = conn.createStatement();
    cdr::db::ResultSet rs       = selTerms.executeQuery(qry.str().c_str());

    while (rs.next()) {
        cdr::String path     = rs.getString(1);
        cdr::String value    = rs.getString(2);
        cdr::Int    int_val  = rs.getInt(3);
        cdr::String node_loc = rs.getString(4);
        cdr::QueryTerm qt(docId, path, value, int_val, node_loc);
        termSet.insert(qt);
    }
    selTerms.close();
}

/**
 * Update one of the query term tables - deleting existing terms that
 * are not in the new document and adding new terms that are not in the
 * old document.
 *
 * Either the query_term table for current working documents, or the
 * query_term_pub table for publishable versions, can be updated.
 *
 * @param conn      Database connection.
 * @param oldTerms  Reference to STL set for terms for old version of doc.
 * @param newTerms  Reference for terms for new version.
 * @param docId     CDR ID for doc
 * @param tblName   "query_term" or "query_term_pub" naming the stored
 *                  collection to update.
 */
void updateQueryTermTable(
    cdr::db::Connection& conn,
    cdr::QueryTermSet& oldTerms,
    cdr::QueryTermSet& newTerms,
    int   docId,
    char* tblName
) {
    // Determine which rows need to be deleted, which need to be added.
    cdr::QueryTermSet wantedTerms, unwantedTerms;
    cdr::QueryTermSet::const_iterator i;
    cdr::QueryTermSet* termsToInsert = &wantedTerms;
    for (i = newTerms.begin(); i != newTerms.end(); ++i)
        if (oldTerms.find(*i) == oldTerms.end())
            wantedTerms.insert(*i);
    for (i = oldTerms.begin(); i != oldTerms.end(); ++i)
        if (newTerms.find(*i) == newTerms.end())
            unwantedTerms.insert(*i);

    // Sometimes it's just faster to wipe out the rows and start fresh.
    if (unwantedTerms.size() > 1000 &&
        unwantedTerms.size() > newTerms.size() / 2) {
        delQueryTerms(conn, docId, tblName);
        termsToInsert = &newTerms;
        // SHOW_ELAPSED("finished wholesale term deletion", queryTermsTimer);
    }
    else {

        // Prepare query to delete a single unwanted term
        std::ostringstream qry;
        qry << " DELETE FROM " << tblName <<
               "       WHERE doc_id   = ? "
               "         AND path     = ? "
               "         AND value    = ? "
               "         AND node_loc = ? ";
        cdr::db::PreparedStatement delStmt =
            conn.prepareStatement(qry.str().c_str());

        // Wipe out the unwanted rows one by one.
        for (i = unwantedTerms.begin(); i != unwantedTerms.end(); ++i) {
            delStmt.setInt   (1, i->doc_id);
            delStmt.setString(2, i->path);
            delStmt.setString(3, i->value);
            delStmt.setString(4, i->node_loc);
            delStmt.executeUpdate();
        }
        delStmt.close();
    }

    // Insert new rows.
    std::ostringstream qry;
    qry << " INSERT INTO " << tblName <<
           "             (doc_id, path, value, int_val, node_loc) "
           "      VALUES (?, ?, ?, ?, ?)";
    cdr::db::PreparedStatement insStmt =
            conn.prepareStatement(qry.str().c_str());
    for (i = termsToInsert->begin(); i != termsToInsert->end(); ++i) {
        insStmt.setInt   (1, i->doc_id);
        insStmt.setString(2, i->path);
        insStmt.setString(3, i->value);
        insStmt.setInt   (4, i->int_val);
        insStmt.setString(5, i->node_loc);
        insStmt.executeUpdate();
    }
    insStmt.close();
    /*
    SHOW_ELAPSED((cdr::String("finished inserting ") +
                  cdr::String::toString(termsToInsert->size()) +
                  cdr::String(" new terms")).toUtf8().c_str(),
                  queryTermsTimer);
    */
}

/**
 * Replaces the rows in the query_term table and/or query_term_pub table
 * for the current document.
 */
void cdr::CdrDoc::updateQueryTerms(
    bool doCWD,
    bool doPUB
) {
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
                      textDocType.toUtf8().c_str());
    cdr::db::ResultSet rs = selStmt.executeQuery(selQuery);
    StringSet paths;
    while (rs.next()) {
        cdr::String path = rs.getString(1);
        if (paths.find(path) == paths.end()) {
            paths.insert(path);
        }
    }
    SHOW_ELAPSED("getting query_term defs", queryTermsTimer);

    // Find out which rows need to be in the query_term table now.
    // We could just return if paths are empty, but we'll continue
    //   in case of the unlikely event that there was once a set of
    //   query term definitions for a document type and there aren't
    //   now, so we want to continue to delete the existing data.
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

    // Gather all the old terms from the old CWD and process them
    if (doCWD) {
        getExistingTerms(docDbConn, oldTerms, Id, "query_term");
        updateQueryTermTable(docDbConn, oldTerms, newTerms, Id, "query_term");
    }

    // If the version we're saving is publishable, do it for query_term_pub
    if (doPUB && verPubStatus == publishable) {
        // Uses the same new terms but gets (possibly) different old terms
        oldTerms.clear();
        getExistingTerms(docDbConn, oldTerms, Id, "query_term_pub");
        updateQueryTermTable(docDbConn, oldTerms, newTerms, Id,
                             "query_term_pub");
    }
}

// Routines to modify the document before saving or validating.
// Make sure these get called in an order which meets any interdependencies
// between the preprocessing requirements.
//
// This is where we put calls to non-generic code that handles
// document type specific logic.
void cdr::CdrDoc::preProcess(bool validating)
{
    updateProtocolStatus(validating);
    sortProtocolSites();
    stripXmetalPis(validating);
    genFragmentIds();
}

// Eliminate elements supplied by the template which the user has decided
// not to use.  See the XSL/T script itself for documentation of the logic.
void cdr::CdrDoc::stripXmetalPis(bool validating)
{
    // Not for control documents
    if (isControlType())
        return;

    // Only do this when the user has requested validation during a save.
    if (!validating)
        return;

    cdr::FilterParmVector pv;
    cdr::String newXml;
    cdr::String errorStr;
    try {
        newXml = cdr::filterDocumentByScriptTitle(Xml,
                                                  L"Strip XMetaL PIs",
                                                  docDbConn, &errorStr, &pv);

        // Save the transformation for later steps.
        setXml(newXml);

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
    // Not for control documents
    if (isControlType())
        return;

    // Protocols are composed of two separate documents that get merged
    // Only generate fragment ids for the component that becomes the
    //   final, complete document after the merge, not for the component
    //   that disappears after the merge.
    // If we generate fragment ids for the component that gets merged in,
    //   they will conflict with the ones in the doc we merge into.
    if (textDocType == L"ScientificProtocolInfo")
        return;

    cdr::String xslt;       // XSLT transform for creating fragment ids
    cdr::String newXml;     // Document XML after fragment id's are entered

    // If we don't have an XSL transform for this document type yet,
    //   create one.
    // XXX Current version always creates it.
    //     Might optimize in future by caching them - which would
    //     save the time needed to retrieve and parse the schema, as
    //     well as to generate the script
    xslt = createFragIdTransform (docDbConn, schemaDocId);

    // If the script isn't null
    if (xslt.size() > 0) {

        // Execute it
        cdr::String filterMsgs = L"";
        try {
            newXml = cdr::filterDocument (Xml, xslt, docDbConn,
                                          &filterMsgs);
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
        setXml(fixedXml);

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
    while (child != 0 && targetName != cdr::String(child.getNodeName()))
        child = child.getNextSibling();
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return L"";
    }

    // Find the CurrentOrgStatus element.
    child = child.getFirstChild();
    targetName = L"CurrentOrgStatus";
    while (child != 0 && targetName != cdr::String(child.getNodeName()))
        child = child.getNextSibling();
    if (child == 0) {
        errList.push_back(L"updateProtocolStatus: missing " + targetName);
        return L"";
    }

    // Find the ProtocolStatusName element.
    child = child.getFirstChild();
    targetName = L"StatusName";
    while (child != 0 && targetName != cdr::String(child.getNodeName()))
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
    if (!validating || textDocType != L"InScopeProtocol")
        return;

    // Get a parsed copy of the document so we can examine the org statuses.
    // DOM tree will be in this->docElem if parse was successful
    if (!parseAvailable()) {
        // Parsing will be tried again later, errors here are redundant
        // Validation will catch the fact that the document is malformed.
        return;
    }

    // Find the statuses for the lead organizations.
    std::set<cdr::String> statusSet;
    cdr::dom::Node child = docElem.getFirstChild();
    cdr::String targetName = L"ProtocolAdminInfo";
    while (child != 0 && targetName != cdr::String(child.getNodeName())) {
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
    if (statusSet.size() == 1)
        status = *statusSet.begin();
    else {

        // Disabled at Lakshmi's request 2002-09-30.
        // Re-enabled at Lakshmi's request 2003-04-18.
        if (statusSet.size() > 1) {
            addError(L"Status mismatch between Lead "
                     L"Organizations.  Status needs to be "
                     L"checked.");
            valCtl.setLastErrorLevel(cdr::ELVL_WARNING);
        }


        if (statusSet.find(L"Active") != statusSet.end())
            status = L"Active";
        else if (statusSet.find(L"Temporarily closed") != statusSet.end())
            status = L"Temporarily closed";
        else if (statusSet.find(L"Completed") != statusSet.end())
            status = L"Completed";
        else if (statusSet.find(L"Closed") != statusSet.end())
            status = L"Closed";
        else if (statusSet.find(L"Approved-not yet active") != statusSet.end())
            status = L"Approved-not yet active";
        else {
            status = L"No valid lead organization status found.";
            addError(status);
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
        setXml(newXml);
    }
    catch (cdr::Exception& e) {
        errList.push_back(L"Failure setting protocol status: " +
                          cdr::String(e.what()));
    }
}

// Sort protocol sites before storing a record.
void cdr::CdrDoc::sortProtocolSites() {

    // Only run this on protocols
    if (textDocType != L"InScopeProtocol")
        return;

    // Generate title
    cdr::String sortedXml  = L"";
    cdr::String filterMsgs = L"";
    try {
        sortedXml = cdr::filterDocumentByScriptSetName (Xml,
                cdr::String ("Protocol Site Sort Set"),
                docDbConn, &filterMsgs);
    }
    catch (cdr::Exception& e) {
        // Add an error to the doc object
        errList.push_back (L"Sorting protocol sites: " +
                           cdr::String (e.what()));
    }

    // If any messages returned, make them available for later viewing
    if (filterMsgs.size() > 0)
        errList.push_back (
            L"Sorting protocol sites, filter produced these messages: " +
            filterMsgs);

    // If we got a result, it replaces the XML that we had
    if (sortedXml.size() > 0)
        setXml(sortedXml);
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
    if (!rs.next()) {
        select.close();
        throw cdr::Exception(L"createFragIdTransform: Unable to load schema");
    }
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
 *  @param tblName      Name of table, "query_term" or "query_term_pub"
 */

static void delQueryTerms (
    cdr::db::Connection& conn,
    int doc_id,
    char *tblName
) {
    cdr::db::Statement delStmt = conn.createStatement();
    char delQuery[80];
    sprintf(delQuery, "DELETE %s WHERE doc_id = %d", tblName, doc_id);
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

// Determine if this is a content type document.
bool cdr::CdrDoc::isContentType()
{
    if (conType == not_set) {
        if (textDocType == L"Filter" ||
            textDocType == L"css"    ||
            textDocType == L"schema")
          conType = control;
        else
          conType = content;
    }

    return (conType == content);
}

// Determine if this is a control type document.
bool cdr::CdrDoc::isControlType()
{
    return (!isContentType());
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

        // Extract value of the element
        cdr::String value = cdr::dom::getTextContent(node);

        // Truncate and notify user if it won't fit in our DBMS indexes
        if (value.size() > MAX_SQLSERVER_INDEX_SIZE) {
            value = value.substr(0, MAX_SQLSERVER_INDEX_SIZE);
            wchar_t msg[400];
            swprintf(msg, L"Warning: Only the first %d characters of "
                          L"field %s will be indexed",
                          MAX_SQLSERVER_INDEX_SIZE, fullPath.c_str());
            addError(msg);
            valCtl.setLastErrorLevel(cdr::ELVL_WARNING);
        }

        // Add the absolute path to the content to the query table
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
    if (!r.next()) {
        s.close();
        throw cdr::Exception(L"Internal error checking for duplicate title");
    }
    int count = r.getInt(1);
    s.close();
    if (count > 0)
        throw cdr::Exception(L"Duplicate title (" + title + L"); not allowed "
                             L"for documents of type " + docType);
}

/**
 * Delete mailer tracking documents left behind by failed mailer jobs.
 *
 * Note: The original version looks for tracking documents connected with
 *       any failed mailer job.  We may choose to optimize this query
 *       by remembering the last time this cleanup was run, or the highest
 *       mailer job number for which we last deleted tracking documents.
 *       Don't add this optimization unless clearly called for by performance
 *       measurements.
 */
cdr::String cdr::mailerCleanup(Session&         session,
                               const dom::Node& ,
                               db::Connection&  conn)
{
    // The test 'doc_id > 390000' is an optimization to skip legacy mailers.
    const char* query =
        " SELECT DISTINCT doc_id                                 "
        "            FROM query_term                             "
        "           WHERE path = '/Mailer/JobId'                 "
        "             AND doc_id > 390000                        "
        "             AND int_val IN (SELECT id                  "
        "                               FROM pub_proc            "
        "                              WHERE status = 'Failure') ";

    db::Statement s = conn.createStatement();
    db::ResultSet r = s.executeQuery(query);
    String reason   = L"Deleting tracking document for failed mailer job";
    StringList errs;
    int nErrs = 0;
    std::wostringstream os;
    std::vector<int> docIds;
    os << L"<CdrMailerCleanupResp>";
    while (r.next()) {
        int id = r.getInt(1);
        docIds.push_back(id);
    }
    s.close();
    for (size_t i = 0; i < docIds.size(); ++i) {
        int id = docIds[i];
        try {
            nErrs += deleteDoc(session, id, false, reason, errs, conn);
            os << L"<DeletedDoc>" << stringDocId(id) << L"</DeletedDoc>";
            conn.commit();
        }
        catch (const Exception& e) {
            ++nErrs;
            errs.push_back(e.what());
            conn.rollback();
        }
    }
    if (nErrs)
        os << packErrors(errs);
    os << L"</CdrMailerCleanupResp>";
    return os.str();
}

/**
 * Add cdr-eid attributes to every element in a document.
 *
 * This function was written to provide higher performance as compared to
 * the Sablotron implementation of xsl:number.
 *
 * Assumptions:
 *    The document is well formed.  Our usage of this function only
 *    occurs after some other logic has caused the document to be parsed
 *    and an exception thrown if the document was not well-formed.
 *    If this assumption does not obtain, there is a very small danger
 *    that we could run off the end of the document string while
 *    attempting to examine up to two or three characters beyond
 *    a recognized start tag or comment end delimiter.
 *
 * @param docStr        The serialized document to process.
 *
 * @return              A new copy of the serialized doc, with cdr-eid
 *                      attributes.  Each attribute has a unique numeric
 *                      value.
 */
static cdr::String addCdrEidAttrs(
    cdr::String &docStr
) {
    const wchar_t *p;         // Pointr into docStr
    const wchar_t *endp;      // Pointer to char after last char of docStr
    wchar_t c;          // Single char at p
    wchar_t c1;         // Single char at p+1
    int     nextEid;    // cdr-eid value
    bool    inTag;      // True = we're inside a start tag
    bool    inComment;  // We're inside an xml <!-- ... --> comment

    // Cumulate output here
    std::wostringstream os;

    // First assigned cdr-eid will be this one
    nextEid = 1;

    inTag = inComment = false;
    p     = docStr.c_str();
    endp  = p + wcslen(p);
    while (p < endp) {
        c = *p++;
        if (c == '<' && !inComment) {
            c1 = *p;
            if (c1 == '!') {
                if (*(p+1) == '-' && *(p+2) == '-') {
                    // Found comment start "<!--"
                    inComment = true;
                }
            }
            else if (c1 != '?' && c1 != '/') {
                // Not a PI or end tag
                inTag = true;
            }
        }
        else if (c == '-' && inComment) {
            if (*(p) == '-' && *(p+1) == '>') {
                // Found comment end "-->"
                inComment = false;
            }
        }
        // Check for two styles of end tag, ">" or "/>"
        else if (((c == '/' && *p == '>') || c == '>') && inTag) {
            // Append attribute like " cdr-eid='_123'"
            os << L" cdr-eid='_" << nextEid++ << "'";
            inTag = false;
        }

        // Output original char from docStr
        os << c;
    }

    // Finished docStr, return output
    return cdr::String(os.str());
}
