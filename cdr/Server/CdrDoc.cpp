/*
 * CdrDoc.cpp
 *
 * Implementation of adding or replacing a document in the database.
 *
 *                                          Alan Meyer  May, 2000
 *
 * $Id: CdrDoc.cpp,v 1.1 2000-05-17 12:51:39 bkline Exp $
 *
 */

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDbConnection.h"
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrDoc.h"

static cdr::String CdrPutDoc (cdr::Session& session,
   const cdr::dom::Node& cmdNode, cdr::db::Connection& dbConn, bool newrec);


// Constructor for a CdrDoc
// Extracts the various elements from the CdrDoc for database update
cdr::CdrDoc::CdrDoc (
    cdr::db::Connection& dbConn,
    cdr::dom::Node& docDom
) : docDbConn (dbConn) {

    // Get doctype and id from CdrDoc attributes
    const cdr::dom::Element& docElement =
        static_cast<const cdr::dom::Element&>(docDom);
    TextDocType = docElement.getAttribute (L"Type");
    if (TextDocType.size() == 0)
        throw cdr::Exception (L"CdrDoc: Doctag missing Type attribute");
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
        throw cdr::Exception(L"CdrDoc: DocType \"" + TextDocType +
                             "\" is invalid");
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
                Blob = cdr::dom::getTextContent (child);
            }

            else {
                // Nothing else is allowed
                throw cdr::Exception(L"CdrDoc: Unrecognized element in CdrDoc");
            }
        }

        child = child.getNextSibling ();
    }

    // Check for absolutely required fields
    if (Title.size() == 0)
        throw cdr::Exception (L"CdrDoc: Missing required DocTitle element");
    if (Xml.size() == 0)
        throw cdr::Exception (L"CdrDoc: Missing required Xml element");

} // CdrDoc ()


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
            "UPDATE document ("
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
    if (!Id)
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

            else
                throw cdr::Exception
                    (L"CdrPutDoc: Unrecognized element in command");
        }

        topNode = topNode.getNextSibling ();
    }

    // Construct a doc object containing all the data
    if (docNode == 0)
        throw cdr::Exception (L"CdrPutDoc: No CdrDoc element in transaction");
    cdr::CdrDoc doc (dbConn, docNode);

    // Check user authorization
    cdr::String action = newrec ? L"ADD DOCUMENT" : L"UPDATE DOCUMENT";
    if (!session.canDo (dbConn, action, doc.getTextDocType()))
        throw cdr::Exception (L"CdrPutDoc: User not authorized to perform "
                              L"this action on this doc type");

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
    dbConn.setAutoCommit (false);

    // If new record, store it so we can get the new ID
    if (newrec)
        doc.Store ();

    else
        // Is document locked by another user?
        // XXXX - Call something here to find out, throw exception if so
        ;

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

    // Check-in to version control if requested
    if (cmdCheckIn == L"Y") {
        // XXXX - VERSION CONTROL
        ;
    }

    // XXXX - Unlock document

    // XXXX - Append audit info

    // If we got here okay commit all updates
    dbConn.setAutoCommit (true);

    // Return string with errors, etc.
    // XXXX - FIX THIS
    cdr::String rtag = newrec ? L"Add" : L"Rep";
    cdr::String resp = cdr::String (L"<Cdr") + rtag + L"DocResp>"
                     + L"<DocId>" + doc.getTextId() + L"</DocId>"
                     + L"</Cdr" + rtag + L"DocResp>";
    return resp;

} // CdrPutDoc
