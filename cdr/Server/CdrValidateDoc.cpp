/*
 * $Id: CdrValidateDoc.cpp,v 1.9 2001-04-05 19:58:18 ameyer Exp $
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * $Log: not supported by cvs2svn $
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
        cdr::String&                    docIdString,
        const wchar_t*                  status,
        cdr::StringList&                errors);
static void setDocStatus(
        cdr::db::Connection&            conn,
        int                             docId,
        const wchar_t*                  status);

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
    cdr::CdrDoc    *docObj;         // Pointer to doc object to validate
    cdr::String    docTypeString;   // String form of doc type name
    cdr::String    validationTypes; // E.g., "Schema Links"
    cdr::ValidRule validRule;       // Do we update the DB or just validate?

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

    // Extract the document or its ID from the command.
    cdr::dom::Node docNode;
    cdr::String    docIdString;
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

                // Make sure the caller is telling the truth about the DocType.
                if (docTypeString != docObj->getTextDocType()) {
                    delete docObj;
                    throw cdr::Exception(
                       L"Command specifies incorrect DocType", docTypeString);
                }

                // If doc is in the database, we assume we want to
                //  update the stored validation status, unless told
                //  otherwise.
                cdr::dom::Element& elem =
                    static_cast<cdr::dom::Element&>(child);
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
    cdr::String returnString = cdr::execValidateDoc (*docObj, validRule,
                                                     validationTypes);

    // Delete temporary object and return results
    delete docObj;
    return returnString;
}


/**
 * Front end function to all validation.
 * Called by validateDoc to perform validation on behalf of a client.
 * Also called by CdrPutDoc when adding or replacing a document in
 *   the database.
 *
 * @param  docObj           Reference to CdrDoc object to validate
 * @param  validRule        Instructions pertaining to database update
 *                              ValidateOnly - Do not update status/link info.
 *                              UpdateIfValid - Update info if doc valid.
 *                              UpdateUnconditionally - Update status/link
 * @param  validationTypes  String containing list of all types of validation
 *                          to perform.
 *
 * @return                  Packed error list string.
 */

cdr::String cdr::execValidateDoc (
    cdr::CdrDoc&         docObj,
    cdr::ValidRule       validRule,
    const cdr::String&   validationTypes
) {
    bool              parseOK = false;
    cdr::String       docTypeString = docObj.getTextDocType();
    cdr::StringList   errList;


cdr::log::WriteFile (L"DEBUG XML", docObj.getXml());
    // Parse the xml text and grab the top element.
    cdr::dom::Parser parser;
    try {
        parser.parse (docObj.getXml());
        parseOK = true;
    }
    catch (const cdr::dom::XMLException& de) {
        errList.push_back (L"Parsing error in XML: "
                          + cdr::String (de.getMessage()));
    }
    catch (...) {
        errList.push_back (L"Unable to parse XML: ");
    }

    if (parseOK) {
        cdr::dom::Document document = parser.getDocument();
        if (document == 0)
            throw cdr::Exception(L"CdrDocXml element for document not found");

        cdr::dom::Element docXml  = document.getDocumentElement();
        cdr::dom::Node    docNode = docXml;

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
            cdr::link::CdrSetLinks (docXml, docObj.getConn(), docObj.getId(),
                                    docTypeString, validRule, errList);
    }

    // Note the outcome of the validation.
    const wchar_t* status = errList.size() > 0 ? L"I" : L"V";
    if (validRule == cdr::UpdateUnconditionally ||
            (validRule == cdr::UpdateIfValid && errList.size() == 0))
        setDocStatus (docObj.getConn(), docObj.getId(), status);

    // Report the outcome.
    return makeResponse (docObj.getTextId(), status, errList);
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
    std::string query = "SELECT xml_schema FROM doc_type WHERE name = ?";
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
    parser.parse(schemaString);
    cdr::xsd::validateDocAgainstSchema(docElem,
                                  parser.getDocument().getDocumentElement(),
                                  errors);
}

/**
 * Sends a response buffer to the client reporting the outcome of the
 * document validation process.
 */
cdr::String makeResponse(cdr::String&     docId,
                         const wchar_t*   status,
                         cdr::StringList& errors)
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
void setDocStatus(
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
