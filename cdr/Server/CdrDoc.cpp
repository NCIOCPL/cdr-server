/**
 * CdrDoc.cpp
 *
 * Implementation of adding or replacing a document in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.cpp,v 1.6 2000-10-27 02:31:12 ameyer Exp $
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
#include "CdrDoc.h"
#include "CdrLink.h"
#include "CdrValidateDoc.h"

// XXX For debugging
#include <iostream>

static cdr::String CdrPutDoc (cdr::Session&, const cdr::dom::Node&,
                              cdr::db::Connection&, bool);

static void auditDoc (cdr::db::Connection&, int, int, cdr::String,
                      cdr::String, cdr::String);

// Constructor for a CdrDoc
// Extracts the various elements from the CdrDoc for database update
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    cdr::dom::Node& docDom
) : docDbConn (dbConn),
    Comment (false),
    BlobData (false),
    ValDate (false),
    Id (0),
    DocType (0) {

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
        std::string qry = "SELECT Id FROM document WHERE Id = ?";
        cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
        select.setInt(1, Id);
        cdr::db::ResultSet rs = select.executeQuery();
        if (!rs.next())
            throw cdr::Exception(L"CdrDoc: Unable to find document " + TextId);
    }
    else
        Id = 0;

    // Check DocType - must be one of recognized types
    std::string qry = "SELECT id FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement select = dbConn.prepareStatement(qry);
    select.setString (1, TextDocType);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"CdrDoc: DocType '" + TextDocType +
                             L"' is invalid");
    DocType = rs.getInt(1);

    // Default values for control elements in the document table
    // All others are defaulted to NULL by cdr::String constructor
    ValStatus    = L"U";
    Approved     = L"N";

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
                            Title = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocComment")
                            Comment = cdr::dom::getTextContent (ctlNode);

                        else if (ctlTag == L"DocApproved")
                            Approved = cdr::dom::getTextContent (ctlNode);

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

                        // Here's how Bob says to do it
                        cdr::dom::Parser parser;
                        Xml = ctlNode.getNodeValue();
                        break;
                    }
                    ctlNode = ctlNode.getNextSibling ();
                }
            }

            else if (name == L"CdrDocBlob") {

                // XXXX Need to decode before storing
                // XXXX NOT STORING YET
                // BlobData = cdr::dom::getTextContent (child);
            }

            else
                // Nothing else is allowed
                throw cdr::Exception(L"CdrDoc: Unrecognized element '" +
                                     name + L"' in CdrDoc");
        }

        child = child.getNextSibling ();
    }

    // Check for absolutely required fields
    if (Title.size() == 0)
        throw cdr::Exception (L"CdrDoc: Missing required 'DocTitle' element");
    if (Xml.size() == 0)
        throw cdr::Exception (L"CdrDoc: Missing required 'Xml' element");

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
                        "                 d.approved,"
                        "                 d.doc_type,"
                        "                 d.xml,"
                        "                 d.comment,"
                        "                 t.name,"
                        "                 b.data"
                        "            FROM document d"
                        "            JOIN doc_type t"
                        "              ON t.id = d.doc_type"
                        " LEFT OUTER JOIN doc_blob b"
                        "              ON b.id = d.id"
                        "           WHERE d.id = ?";
    cdr::db::PreparedStatement select = docDbConn.prepareStatement(query);
    select.setInt(1, docId);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"CdrDoc: Unable to load document " + TextId);

    ValStatus   = rs.getString (1);
    ValDate     = rs.getString (2);
    Title       = cdr::entConvert (rs.getString (3));
    Approved    = rs.getString (4);
    DocType     = rs.getInt (5);
    Xml         = rs.getString (6);
    Comment     = cdr::entConvert (rs.getString (7));
    TextDocType = rs.getString (8);
    BlobData    = rs.getBytes (9);
    select.close();
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
            "   approved,"
            "   doc_type,"
            "   title,"
            "   xml,"
            "   comment"
            ") VALUES (?, ?, ?, ?, ?, ?, ?)";
    }

    // Existing record
    else {
        sqlStmt =
            "UPDATE document "
            "  SET val_status = ?,"
            "      val_date = ?,"
            "      approved = ?,"
            "      doc_type = ?,"
            "      title = ?,"
            "      xml = ?,"
            "      comment = ?"
            "  WHERE"
            "      id = ?";
    }

    // Fill in values
    cdr::db::PreparedStatement docStmt = docDbConn.prepareStatement (sqlStmt);
    docStmt.setString (1, ValStatus);
    docStmt.setString (2, ValDate);
    docStmt.setString (3, Approved);
    docStmt.setInt    (4, DocType);
    docStmt.setString (5, Title);
    docStmt.setString (6, Xml);
    docStmt.setString (7, Comment);

    // For existing records, fill in ID
    if (Id)
        docStmt.setInt    (8, Id);

    // Do it
    docStmt.executeQuery ();

    // If new record, find out what was the key
    if (!Id) {
        Id = docDbConn.getLastIdent ();

        // Convert id to wide characters
        char idbuf[32];
        sprintf (idbuf, "CDR%010d", Id);
        TextId = idbuf;
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
    cdr::String cmdCheckIn,     // Checkin flag from command
                cmdValidate,    // Validate flag from command
                cmdReason;      // Reason to associate with new version
    cdr::dom::Node topNode,     // Top level node in command
                   child,       // Child node in command
                   docNode;     // Node containing CdrDoc


    // Default values, may be replace by elements in command
    // Default reason is NULL created by cdr::String contructor
    cmdValidate = L"Y";
    cmdCheckIn  = L"N";
    cmdReason   = L"";

    // Parse command to get document and relevant parts
    topNode = cmdNode.getFirstChild();
    while (topNode != 0) {
        if (topNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = topNode.getNodeName();

            if (name == L"CheckIn")
                cmdCheckIn = cdr::dom::getTextContent (topNode);

            else if (name == L"Validate")
                cmdValidate = cdr::dom::getTextContent (topNode);

            else if (name == L"Reason")
                cmdReason = cdr::dom::getTextContent (topNode);

            else if (name == L"CdrDoc")

                // Save node for later use
                docNode = topNode;

            else {
                throw cdr::Exception (L"CdrDoc: Unrecognized element '" +
                                        name + L"' in command");
            }
        }

        topNode = topNode.getNextSibling ();
    }

    // Construct a doc object containing all the data
    if (docNode == 0)
        throw cdr::Exception(L"CdrPutDoc: No 'CdrDoc' element in transaction");
    cdr::CdrDoc doc (dbConn, docNode);

    // Check user authorization
    cdr::String action = newrec ? L"ADD DOCUMENT" : L"MODIFY DOCUMENT";
    cdr::String doctype = doc.getTextDocType();
    if (!session.canDo (dbConn, action, doctype))
        throw cdr::Exception (L"CdrPutDoc: User '" + session.getUserName() +
                              L"' not authorized to '" + action +
                              L"' with docs of type '" + doctype + L"'");

    // Does document id attribute match expected action type?
    if (doc.getId() && newrec)
        throw cdr::Exception (L"CdrPutDoc: Attempt to add doc with "
                              L"existing ID as new");
    else if (!doc.getId() && !newrec)
        throw cdr::Exception (L"CdrPutDoc: Attempt to replace doc "
                              L"without existing ID");

    // Can perform schema validation before starting transaction
    if (cmdValidate == L"Y") {
        // XXXX
        ;
    }

    // Start transaction - everything is serialized from here on
    // If any called function throws an exception, CdrServer
    //   will catch it and perform a rollback
    dbConn.setAutoCommit (false);

    // If new record, store it so we can get the new ID
    if (newrec)
        doc.Store ();

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
    }

    // Perform link validation if requested
    // Must be inside transaction wrapper
    if (cmdValidate == L"Y") {
        // XXXX Call link validation, link table update
        // Any other validation
        // Set valid_status and valid_date in the doc object
        // Will overwrite whatever is there
        ;
    }

    // If no errors, store it
    if (!newrec)
        doc.Store ();

    // Add support for queries.
    doc.updateQueryTerms();

    // Check-in to version control if requested
    int version = 0;
    if (cmdCheckIn == L"Y")
        version = cdr::checkIn (doc.getId(), dbConn, session.getUserId(),
                                &cmdReason, 0, 0);

    // Append audit info
    auditDoc (dbConn, doc.getId(), session.getUserId(), action,
              L"CdrPutDoc", cmdReason);

    // If we got here okay commit all updates
    dbConn.commit();
    dbConn.setAutoCommit (true);

    // Return string with errors, etc.
    // XXXX - FIX THIS
    cdr::String rtag = newrec ? L"Add" : L"Rep";
    cdr::String resp = cdr::String (L"  <Cdr") + rtag + L"DocResp>\n"
                     + L"   <DocId>" + doc.getTextId() + L"</DocId>\n"
                     + L"  </Cdr" + rtag + L"DocResp>\n";
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
    cdr::String cmdValidate,    // Validate flag from command
                cmdReason,      // Reason to associate with new version
                docIdStr;       // Doc id in string form
    int         docId,          // Doc id as unadorned integer
                docType,        // Document type
                userId,         // User invoking this command
                outUserId,      // User with the doc checked out, if it is
                errCount;       // Number of link releated errors
    bool        checkedOut,     // True=doc checked out by someone
                autoCommitted;  // True=Not inside SQL transaction at start
    cdr::dom::Node topNode,     // Top level node in command
                   child,       // Child node in command
                   docNode;     // Node containing CdrDoc
    cdr::ValidRule vRule;       // Validation request to CdrDelLinks
    cdr::StringList errList;    // List of errors from CdrDelLinks


    // Default values, may be replace by elements in command
    // Default reason is NULL created by cdr::String contructor
    cmdValidate = L"Y";
    cmdReason   = L"";

    // Parse incoming command
    topNode = cmdNode.getFirstChild();
    while (topNode != 0) {
        if (topNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = topNode.getNodeName();

            if (name == L"DocId") {
                docIdStr = cdr::dom::getTextContent (topNode);
                docId    = docIdStr.extractDocId ();
            }

            else if (name == L"Validate")
                cmdValidate = cdr::dom::getTextContent (topNode);

            else if (name == L"Reason")
                cmdReason = cdr::dom::getTextContent (topNode);

            else
                throw cdr::Exception (L"CdrDelDoc: Unrecognized element '" +
                                        name + L"' in command");
        }

        topNode = topNode.getNextSibling ();
    }

    // Get document type, need it to check user authorization
    std::string tQry = "SELECT doc_type FROM document WHERE id = ?";
    cdr::db::PreparedStatement tSel = conn.prepareStatement (tQry);
    tSel.setInt (1, docId);
    cdr::db::ResultSet tRs = tSel.executeQuery();
    if (!tRs.next())
        throw cdr::Exception (L"delDoc: Unable to find document " + docIdStr);
    docType = tRs.getInt (1);

    // Is user authorized to do this?
    cdr::String foo = L"DELETE DOCUMENT";
    // if (!session.canDo (conn, L"DELETE DOCUMENT", docType))
    if (!session.canDo (conn, foo, docType))
        throw cdr::Exception (L"delDoc: User not authorized to delete docs "
                              L"of this type");

    // From now on, do everything or nothing
    // setAutoCommit() checks state first, so it won't end an existing
    //   transaction
    autoCommitted = conn.getAutoCommit();
    conn.setAutoCommit(false);

    // Has anyone got this document checked out?
    checkedOut = false;
    userId = session.getUserId();
    if (isCheckedOut(docId, conn, &outUserId)) {
        if (outUserId != userId)
            throw cdr::Exception (L"Document " + docIdStr + L" is checked out"
                                  L" by another user");
        checkedOut = true;
    }

    // Update the link net to delete links from this document
    // If validation was requested, we tell CdrDelLinks to abort if
    //   there are any links _to_ this document
    vRule =  (cmdValidate == L"Y" || cmdValidate == L"y") ?
             UpdateIfValid : UpdateUnconditionally;

    errCount = cdr::link::CdrDelLinks (conn, docId, vRule, errList);

    if (errCount == 0 || vRule == UpdateUnconditionally) {

        // Proceed with deletion
        // Begin by deleting all query terms pointing to this doc
        std::string qQry = "DELETE query_term WHERE doc_id = ?";
        cdr::db::PreparedStatement qSel = conn.prepareStatement (qQry);
        qSel.setInt (1, docId);
        cdr::db::ResultSet tRs = qSel.executeQuery();

        // And the document itself
        std::string dQry = "DELETE document WHERE id = ?";
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
 * Replaces the rows in the query_term table for the current document.
 */
void cdr::CdrDoc::updateQueryTerms()
{
    // Step 0: sanity check.
    if (Id == 0 || DocType == 0)
        return;

    // Step 1: clear out the existing rows.
    cdr::db::Statement delStmt = docDbConn.createStatement();
    char delQuery[80];
    sprintf(delQuery, "DELETE query_term WHERE doc_id = %d", Id);
    delStmt.executeUpdate(delQuery);

    // Step 2: find out which paths get indexed.
    cdr::db::Statement selStmt = docDbConn.createStatement();
    char selQuery[256];
    sprintf(selQuery, "SELECT path"
                      "  FROM query_term_def"
                      " WHERE path LIKE '/%s/%%'",
                      TextDocType.toUtf8().c_str());
    cdr::db::ResultSet rs = selStmt.executeQuery(selQuery);
    StringSet paths;
    while (rs.next()) {
        cdr::String path = rs.getString(1);
        if (paths.find(path) == paths.end()) {
            paths.insert(path);
        }
    }

    // Step 3: add rows for query terms.
    if (!paths.empty()) {
        cdr::dom::Parser parser;
        parser.parse(Xml);
        cdr::dom::Node node = parser.getDocument().getDocumentElement();
        addQueryTerms(cdr::String("/") + node.getNodeName(), node, paths);
    }
}

/**
 * Adds a row to the query_term table for the current node if appropriate,
 * and recursively does the same for all sub-elements.
 *
 *  @param  path        reference to string representing path for current
 *                      node; e.g., "/Person/PersonStatus".
 *  @param  node        reference to current node of document's DOM tree.
 *  @param  paths       reference to set of paths to be indexed.
 */
void cdr::CdrDoc::addQueryTerms(const cdr::String& path,
                                const cdr::dom::Node& node,
                                const StringSet& paths)
{
    // Step 1: add a row to the query_term table if appropriate.
    if (paths.find(path) != paths.end()) {
        cdr::String value = cdr::dom::getTextContent(node);
        const char* insert = "INSERT INTO query_term(doc_id, path, value)"
                             "     VALUES(?,?,?)";
        cdr::db::PreparedStatement stmt = docDbConn.prepareStatement(insert);
        stmt.setInt(1, Id);
        stmt.setString(2, path);
        stmt.setString(3, value);
        stmt.executeQuery();
    }

    // Step 2: recursively add terms for sub-elements.
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            addQueryTerms(path + "/" + child.getNodeName(),
                          child,
                          paths);
        }
        child = child.getNextSibling();
    }
}
