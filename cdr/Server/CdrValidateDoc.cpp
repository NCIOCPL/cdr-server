/*
 * $Id: CdrValidateDoc.cpp,v 1.24 2008-01-29 15:16:38 bkline Exp $
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.23  2005/03/04 02:58:56  ameyer
 * Small changes for new Xerces DOM parser.
 *
 * Revision 1.22  2004/08/20 19:58:56  bkline
 * Added new CdrSetDocStatus command.
 *
 * Revision 1.21  2003/04/30 10:36:27  bkline
 * Implemented support for validation warning messages which do not
 * cause the document to be marked as invalid.
 *
 * Revision 1.20  2002/08/14 01:52:39  ameyer
 * Replaced independent check of schema/filter/css document types with
 * call to CdrDoc::isControlType()
 *
 * Revision 1.19  2002/07/15 18:57:05  bkline
 * Restored 'schema' to 'Schema' for validation type.
 *
 * Revision 1.18  2002/07/03 12:55:19  bkline
 * Fixed check for Schema doctype.
 *
 * Revision 1.17  2002/03/15 21:56:50  bkline
 * Fixed log comment for previous version.
 *
 * Revision 1.16  2002/03/15 21:53:12  bkline
 * Added fix for val_status setting.
 *
 * Revision 1.15  2002/02/26 22:39:39  ameyer
 * Now updating document.val_status with 'M' if document is malformed.
 * (previous comment about this was in error.)
 * Now catching DOM exception if schema parse fails to throw a more
 * informative cdr::Exception to report doctype and that it was a schema.
 *
 * Revision 1.14  2001/09/25 14:22:57  ameyer
 * Updated call to validateDocAgainstSchema for new parameter.
 *
 * Revision 1.13  2001/06/20 00:54:51  ameyer
 * Changed handling of error lists in order to incorporate error info
 * from other stages of processing into one returned list to client,
 * and to eliminate CdrValidateDocResp wrappers from inappropriate place.
 *
 * Revision 1.12  2001/06/15 02:30:04  ameyer
 * Now using a common parse of the document instead of performing a new one.
 * Returning a proper error message if document is malformed.
 * Updating val_status accordingly to 'M'.
 *
 * Revision 1.11  2001/05/16 15:46:11  bkline
 * Adjusted query to get the top-level schema document from the
 * document table instead of the doc_type table, where it used
 * to live.
 *
 * Revision 1.10  2001/04/10 21:39:02  ameyer
 * Added catch(...) to ensure doc object allocate on heap is deleted.
 *
 * Revision 1.9  2001/04/05 19:58:18  ameyer
 * Fixed comments.
 *
 * Revision 1.8  2000/10/05 21:26:14  bkline
 * Moved most of the lower-level schema validation routines to the CdrXsd
 * module.
 *
 * Revision 1.7  2000/10/05 14:40:33  ameyer
 * Converted to work with CdrDoc objects, to implement link validation,
 * and to handle database update flags differently.
 *
 * Revision 1.6  2000/05/17 12:50:49  bkline
 * Replaced code to create Errors element with call to cdr::packErrors().
 *
 * Revision 1.5  2000/05/03 15:20:38  bkline
 * Reworked to expose document validation to other modules.  Fixed use
 * of prepared db statements.
 *
 * Revision 1.4  2000/04/29 15:50:13  bkline
 * First version with all stubs replaced.
 *
 * Revision 1.3  2000/04/27 13:10:49  bkline
 * Replaced stubs for validation of simple types.
 *
 * Revision 1.2  2000/04/26 01:25:01  bkline
 * First working version, with stubs for validation of simple types.
 *
 * Revision 1.1  2000/04/25 03:42:18  bkline
 * Initial revision
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

// System headers.
#include <exception>
// XXX DEBUGGING ONLY.
#include <iostream>

// Project headers.
#include "CdrString.h"
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrXsd.h"
#include "CdrGetDoc.h"
#include "CdrLink.h"
#include "CdrValidateDoc.h"

// Local functions.
static cdr::String makeResponse(
        const cdr::String&     docIdString,
        const cdr::String&     status,
        const cdr::StringList& errors);
static void setValStatus(
        cdr::db::Connection&   conn,
        int                    docId,
        const wchar_t*         status);

/**
 * Validates a CDR document, using the following steps.
 * <PRE>
 *  <OL>
 *   <LI> Get the document to be validated (from the database or directly
 *        from the command, depending on the variant invoked)</LI>
 *   <LI> Parse the document</LI>
 *   <LI> Extract the document type</LI>
 *   <LI> Retrieve the schema for the document type</LI>
 *   <LI> Validate the document against the schema</LI>
 *   <LI> Verify the validity of the links to and from the document</LI>
 *  </OL>
 * </PRE>
 *
 * As many errors are reported as possible (in contrast to an algorithm
 * which bails out when the first error is encountered).  Obviously, if the
 * document isn't even well-formed, there's not much else we can report,
 * however.
 */
cdr::String cdr::validateDoc(
        cdr::Session&                   session,
        const cdr::dom::Node&           commandNode,
        cdr::db::Connection&            conn)
{
    cdr::CdrDoc    *docObj = NULL;  // Pointer to doc object to validate
    cdr::String    docIdString;     // String form of doc type name
    cdr::String    docTypeString;   // String form of doc type name
    cdr::String    validationTypes; // E.g., "Schema Links"
    cdr::ValidRule validRule;       // Do we update the DB or just validate?
    cdr::String    valStatus;       // V)alid, I)valid, M)alformed

    // Get document type and check user authorization to validate it
    const cdr::dom::Element& commandElement =
        static_cast<const cdr::dom::Element&>(commandNode);
    validationTypes = commandElement.getAttribute(L"ValidationTypes");
    docTypeString   = commandElement.getAttribute(L"DocType");
    if (docTypeString.empty())
        throw cdr::Exception(L"DocType attribute missing from "
                           L"CdrValidateDoc command element");
    if (!session.canDo(conn, L"VALIDATE DOCUMENT", docTypeString)) {
        cdr::String err = cdr::String(L"Validation of ")
                        + docTypeString
                        + L" documents is not authorized for this user";
        throw cdr::Exception(err.c_str());
    }

    try {
        // Extract the document or its ID from the command.
        cdr::dom::Node docNode;
        cdr::dom::Node child = commandNode.getFirstChild();
        while (child != 0) {
            if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                cdr::String name = child.getNodeName();
                if (name == L"CdrDoc") {

                    // Document is right here
                    // Create an object for it
                    docNode = child;
                    docObj = new cdr::CdrDoc (conn, docNode);

                    // This version assumes that doc is passed in solely
                    //   for validation.
                    // It may not even be in the database at all.
                    // So we won't try to update any stored validation status.
                    validRule = ValidateOnly;
                }

                else if (name == L"DocId") {

                    // Document is in the database
                    // Get ID for it
                    docIdString = cdr::dom::getTextContent (child);

                    // Construct object from the database
                    docObj = new cdr::CdrDoc (conn, docIdString.extractDocId());

                    // Make sure the caller tells the truth about the DocType.
                    if (docTypeString != docObj->getTextDocType())
                        // catch at end will cleanup docObj
                        throw cdr::Exception (
                                L"Command specifies incorrect DocType",
                                docTypeString);

                    // If doc is in the database, we assume we want to
                    //  update the stored validation status, unless told
                    //  otherwise.
                    cdr::dom::Element elem(child);
                    cdr::String validateOnlyAttr =
                        elem.getAttribute(L"ValidateOnly");
                    if (validateOnlyAttr == L"Y")
                        validRule = ValidateOnly;
                    else
                        validRule = UpdateUnconditionally;
                }
            }
            child = child.getNextSibling();
        }

        // Make sure we got what we need.
        int  docId = 0;
        if (docIdString.empty() && docNode == 0)
            throw cdr::Exception(L"Command requires DocId or CdrDoc element");
        if (docNode != 0 && !docIdString.empty())
            throw cdr::Exception(L"Both DocId and CdrDoc specified");

        // Execute validation
        valStatus = cdr::execValidateDoc (*docObj, validRule, validationTypes);
    }

    // Cleanup from any exception, anywhere in or beneath us
    catch (...) {
        // Don't leave allocated objects in the heap
        if (docObj)
            delete docObj;

        // Rethrow exception
        throw;
    }

    // Delete temporary object and return results
    // Note references to errList and textId should preserve contents
    //   of these strings beyond docObj destruction
    docIdString                = docObj->getTextId();
    cdr::StringList docErrList = docObj->getErrList();
    delete docObj;
    return makeResponse (docIdString, valStatus, docErrList);
}


/*
 * Front end function to all validation.
 * Called by validateDoc to perform validation on behalf of a client.
 * Also called by CdrPutDoc when adding or replacing a document in
 *   the database.
 *
 * See CdrValidateDoc.h
 */

cdr::String cdr::execValidateDoc (
    cdr::CdrDoc&         docObj,
    cdr::ValidRule       validRule,
    const cdr::String&   validationTypes
) {
    // Should not get here for control docs, but secondary check just in case
    // Just say it's valid if this is one of them.
    if (docObj.isControlType())
        return L"V";

    bool              parseOK = false;
    cdr::String       docTypeString = docObj.getTextDocType();

    // Document status: 'V'alid, 'I'nvalid, or 'M'alformed
    // Invalid until found otherwise
    const wchar_t* status = L"I";

    // Will append to the error list from the document object
    cdr::StringList& errList = docObj.getErrList();

    // Check for private-use characters, which we don't allow (request #3823).
    cdr::String serializedDoc = docObj.getXml();
    for (size_t i = 0; i < serializedDoc.size(); ++i) {
        unsigned int c = (unsigned int)serializedDoc[i];
        if (c >= 0xE000 && c <= 0xF8FF) {
            wchar_t err[80];
            swprintf(err, L"private use character U+%04X at position %u",
                     c, i + 1);
            errList.push_back(err);
        }
    }

    // Get a parse tree for the XML
    if (docObj.parseAvailable()) {
        cdr::dom::Element docXml = docObj.getDocumentElement();

        // Validate the document against the schema if appropriate.
        if (validationTypes.empty()
        ||   validationTypes.find(L"Schema") != validationTypes.npos) {
            cdr::validateDocAgainstSchema (docXml, docTypeString,
                                           docObj.getConn(), errList);

            // If there were schema errors and we aren't supposed to update
            //   the database in the event of errors, reset the flag to
            //   prevent updates
            if (validRule == cdr::UpdateIfValid && errList.size() > 0)
                validRule = cdr::ValidateOnly;
        }

        // Validate links if appropriate
        if (validationTypes.empty()
        ||   validationTypes.find(L"Links") != validationTypes.npos)
            cdr::link::CdrSetLinks (cdr::dom::Node(docXml),
                                    docObj.getConn(), docObj.getId(),
                                    docTypeString, validRule, errList);
    }

    else {
        // Add this to the error messages.  Parse error msg already there
        errList.push_back (L"Document malformed.  Validation not performed");

        // Set status in databse to malformed
        status = L"M";
    }

    // If not malformed, note the outcome of the validation.
    if (wcscmp(status, L"M")) {
        size_t warningCount = static_cast<size_t>(docObj.getWarningCount());
        bool invalid = errList.size() > warningCount;
        status = invalid ? L"I" : L"V";
    }

    // Update database and object status
    docObj.setValStatus(status);
    if (validRule == cdr::UpdateUnconditionally ||
            (validRule == cdr::UpdateIfValid && errList.size() == 0))
        setValStatus (docObj.getConn(), docObj.getId(), status);

    // Report the outcome
    return status;
}


/**
 * Checks document against the schema for its document type, reporting any
 * errors found in the caller's <code>Errors</code> vector.  Caller is
 * responsible for ensuring that the docTypeName is correct for this
 * document.
 *
 *  @param  docElem             top level element for CdrDoc element
 *  @param  docTypeName         string for name of document's type
 *  @param  conn                reference to CDR database connection object
 *  @param  errors              vector of strings to be populated by this
 *                              function
 *
 *  @exception  cdr::Exception  if database or parsing error encountered
 */
void cdr::validateDocAgainstSchema(
        cdr::dom::Element&              docElem,
        const cdr::String&              docTypeName,
        cdr::db::Connection&            conn,
        cdr::StringList&                errors)
{
    // Go get the schema for the document's type.
    std::string query = "SELECT xml"
                        "  FROM document,"
                        "       doc_type"
                        " WHERE doc_type.name       = ?"
                        "   AND doc_type.xml_schema = document.id";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setString(1, docTypeName);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unknown document type", docTypeName);
    cdr::String schemaString = rs.getString(1);
    if (schemaString.empty())
        throw cdr::Exception (L"No schema available for doc type"
                              + docTypeName);

    // Check the document against its schema.
    cdr::dom::Parser parser;
    try {
        parser.parse(schemaString);
    }
    catch (const cdr::dom::XMLException& e) {
        throw cdr::Exception (
                L"validateDocAgainstSchema: Error parsing " + docTypeName +
                L" schema: " + e.getMessage());
    }

    cdr::xsd::validateDocAgainstSchema(docElem,
                                  parser.getDocument().getDocumentElement(),
                                  errors, &conn);
}

/**
 * Sends a response buffer to the client reporting the outcome of the
 * document validation process.
 */
cdr::String makeResponse(const cdr::String&     docId,
                         const cdr::String&     status,
                         const cdr::StringList& errors)
{
    cdr::String response = L"  <CdrValidateDocResp>\n"
                           L"   <DocId>"
                         + docId
                         + L"</DocId>\n   <DocStatus>"
                         + status
                         + L"</DocStatus>\n";
    if (errors.size() > 0)
        response += cdr::packErrors(errors);
    return response + L"  </CdrValidateDocResp>\n";
}

/**
 * Records the new status of the document in the database.
 */
void setValStatus(
        cdr::db::Connection&            conn,
        int                             id,
        const wchar_t*                  status)
{
    std::string query = "UPDATE document"
                        "   SET val_status = ?,"
                        "       val_date = GETDATE()"
                        " WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setString(1, status);
    update.setInt(2, id);
    update.executeQuery();
}
