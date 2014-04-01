/*
 * $Id$
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * BZIssue::4767
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

// System headers.
#include <exception>
// XXX DEBUGGING ONLY.
#include <iostream>

// Uncomment next line for timing outputs
// #define CDR_TIMINGS
#include "CdrTiming.h"

// Project headers.
#include "CdrString.h"
#include "CdrCommand.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrXsd.h"
#include "CdrGetDoc.h"
#include "CdrLink.h"
#include "CdrValidateDoc.h"

// For DEBUG only
extern void saveXmlToFile(char *, char *, cdr::String);

// Local functions.
static cdr::String makeResponse(
        const cdr::String& docIdString,
        const cdr::String& status,
        const cdr::String& errors,
        const cdr::String& docXml);
static void setValStatus(
        cdr::db::Connection&   conn,
        int                    docId,
        const wchar_t*         status);

// Constructor using the error attribute value
cdr::ValidationError::ValidationError(
    cdr::String& msg,
    cdr::String& errorId
) {
    // Store passed parms
    errMsg   = msg;
    errIdStr = errorId;

    // Set default values for type and error level
    // Use setLastErrorType() or setLastErrorLevel() if change is required
    errType  = ETYPE_VALIDATION;
    errLevel = ELVL_ERROR;
}

// Does an error have context associated with it?
bool cdr::ValidationError::hasContext() {
    if (errIdStr == cdr::NO_ERROR_CONTEXT)
        return false;
    return true;
}

// Extract an error in XML string format
cdr::String cdr::ValidationError::toXmlString(
    bool includeLocator
) {
    // Wrap the error message
    cdr::String errStr = L"<Err";
    if (includeLocator && hasContext()) {
        // If there was an errorId specifically set, use it
        cdr::String eref = errIdStr;

        if (eref.size() > 0)
            errStr += L" eref='" + eref + L"'";
    }

    // Additional attributes can appear with or without context, but
    //   only if the client is using new style error location
    if (includeLocator) {

        // Human readable representations of type
        cdr::String eType;
        switch (errType) {
            case ETYPE_VALIDATION:
                eType = L"validation";
                break;
            case ETYPE_OTHER:
                eType = L"other";
        }
        errStr += L" etype='" + eType + L"'";

        // Human readable representation of severity
        cdr::String eLevel;
        switch (errLevel) {
            case ELVL_INFO:
                eLevel = L"info";
                break;
            case ELVL_WARNING:
                eLevel = L"warning";
                break;
            case ELVL_ERROR:
                eLevel = L"error";
                break;
            case ELVL_FATAL:
                eLevel = L"fatal";
        }
        errStr += L" elevel='" + eLevel + L"'";
    }

    // Add the error message and terminator
    // Note:
    //  No entity conversion is done here.  A study of the code shows that
    //   entity conversion has already been performed in CdrXsd errors and is
    //   not needed in others because they are constant strings, reports of
    //   CDR-IDs, etc.
    //  Calling entConvert() here would therefore seem to do no good and
    //   might do harm from double conversions.
    //  See JIRA issue OCECDR-3744 for discussions of entity conversion.
    errStr += L">" + errMsg + L"</Err>\n";

    return errStr;
}

// Constructor for controls governing one validation
cdr::ValidationControl::ValidationControl()
{
    // Assume no error locators unless they are requested later
    usingErrorIds = false;
}

// Remember what element we're validating
void cdr::ValidationControl::setElementContext(
    const cdr::dom::Element& ctxtNode
) {
    cdr::String attrValue = ctxtNode.getAttribute(L"cdr-eid");
    if (attrValue.empty())
        currentCtxt = cdr::NO_ERROR_CONTEXT;
    else
        currentCtxt = attrValue;
}

// Add one error
void cdr::ValidationControl::addError(
    cdr::String msg,
    cdr::String errorId
) {
    // If no explicit errorId passed, take the ValidationCtl currentCtxt,
    //  which may have something (or may also be set to NO_ERROR_CONTEXT)
    if (errorId == cdr::NO_ERROR_CONTEXT)
        errorId = currentCtxt;

    // Create the error object
    ValidationError ve(msg, errorId);

    // Add it to the sequence of errors
    errVector.push_back(ve);
}

// Modify the type of the previous error
void cdr::ValidationControl::setLastErrorType(
    cdr::ErrType type
) {
    errVector[errVector.size() - 1].setErrorType(type);
}

// Modify the severity level of the previous error
void cdr::ValidationControl::setLastErrorLevel(
    cdr::ErrLevel level
) {
    errVector[errVector.size() - 1].setErrorLevel(level);
}

// Append errors from another ValidationControl to this one
void cdr::ValidationControl::cumulateErrors(
    cdr::ValidationControl& errSrc
) {
    // If there are any errors
    if (errSrc.getErrorCount() > 0) {
        std::vector<ValidationError>::const_iterator ei =
                                      errSrc.errVector.begin();
        // Add each one from the source vector to this vector
        while (ei != errSrc.errVector.end())
            errVector.push_back(*ei++);
    }
}

// How many errors have been recorded so far
int cdr::ValidationControl::getErrorCount() const
{
    return errVector.size();
}

// How many errors have been recorded at a particular severity level
int cdr::ValidationControl::getLevelCount(
    ErrLevel level
) const {
    int count = 0;

    if (getErrorCount() > 0) {
        std::vector<ValidationError>::const_iterator ei = errVector.begin();
        while (ei != errVector.end()) {
            if (ei->getErrorLevel() == level)
                ++count;
            ++ei;
        }
    }

    return count;
}

// Say whether we want to track error locations (cdr-eid values).
void cdr::ValidationControl::setLocators(bool locators)
{
    usingErrorIds = locators;
}

// Are we tracking error locators (cdr-eid values)?
bool cdr::ValidationControl::hasLocators() const
{
    return usingErrorIds;
}

// Get String representation of errors
void cdr::ValidationControl::getErrorMsgs(
    cdr::StringList& msgList
) {
    // Iterate through all error messages, adding them to the list
    std::vector<cdr::ValidationError>::iterator ei = errVector.begin();
    while (ei != errVector.end())
        msgList.push_back(ei->getMessage());
}

// Get XML string representing all errors, or empty string
cdr::String cdr::ValidationControl::getErrorXml(
    StringList& errList
) {
    // Assemble numbers here
    wchar_t buf[80];

    // Assemble messages here
    cdr::String xml = L"";

    // Number of errors
    size_t count = errVector.size() + errList.size();

    // If there aren't any, we're done.  Empty string means no errors.
    if (count == 0)
        return xml;

    // Errors not associated with specific elements, cumulated in a string
    // We may not need this anymore, but it doesn't hurt to leave it in
    if (errList.size() > 0) {
        cdr::StringList::const_iterator i = errList.begin();
        while (i != errList.end())
            xml += L"<Err>" + *i++ + L"</Err>\n";
    }

    // String version of the count
    // This is a new format, only use it if code is aware of error locators
    // Otherwise it might break something
    cdr::String countStr = L"";
    if (usingErrorIds) {
        swprintf(buf, L" count='%d'", count);
        countStr = buf;
    }

    // Add each error
    // Whitespace is just for style compatibility with older code
    if (errVector.size() > 0) {
        std::vector<ValidationError>::iterator ei = errVector.begin();
        while (ei != errVector.end()) {
            xml += (*ei).toXmlString(usingErrorIds);
            ++ei;
        }
    }

    // Add wrapper
    xml = L"<Errors" + countStr + L">\n" + xml + L"</Errors>\n";

    return xml;
}


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

    MAKE_TIMER(fullValTimer);
    MAKE_TIMER(setupValTimer);
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

    // Find out whether the client wants cdr-eid attributes sent back to him
    bool withLocators = cdr::ynCheck(commandElement.getAttribute(
                                     L"ErrorLocators"), false);

    SHOW_ELAPSED("Validation setup", setupValTimer);
    try {
        MAKE_TIMER(docPrepValTimer);
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
                    MAKE_TIMER(createDocTimer);
                    docObj = new cdr::CdrDoc (conn, docNode, session,
                                              withLocators);
                    SHOW_ELAPSED("Create CdrDoc obj from xml", createDocTimer);

                    // This version assumes that doc is passed in solely
                    //   for validation.
                    // It may not even be in the database at all.
                    // So we won't try to update any stored validation status.
                    validRule = ValidateOnly;
                }

                else if (name == L"DocId") {

                    MAKE_TIMER(fetchDocTimer);
                    // Document is in the database
                    // Get ID for it
                    docIdString = cdr::dom::getTextContent (child);

                    // Construct object from the database
                    docObj = new cdr::CdrDoc (conn, docIdString.extractDocId(),
                                              withLocators);

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
                    SHOW_ELAPSED("Create CdrDoc obj from db", fetchDocTimer);
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

        SHOW_ELAPSED("Preparing doc for validation", docPrepValTimer);
        // Execute validation
        valStatus = cdr::execValidateDoc (docObj, validRule, validationTypes);
    }

    // Cleanup from any exception, anywhere in or beneath us
    catch (...) {
        // Don't leave allocated objects in the heap
        if (docObj)
            delete docObj;

        // Rethrow exception
        throw;
    }

    // Prepare fields that are returned to the caller
    docIdString = docObj->getTextId();

    // Current convention is to not return an errors element if there
    //   are no errors.  getErrorXml() follows the convention.
    cdr::String xmlErrs = docObj->getErrorXml();

    // If we need to return XML with cdr-eid attributes, fetch it
    cdr::String xmlToSend = L"";
    if (docObj->hasLocators() && docObj->getErrorCount() > 0)
        xmlToSend = docObj->getSerialXml();

    // makeReponse parameters have been extracted from the docObj and
    //   separate saved.
    // Cleanup memory, we've extracted everything we need.
    delete docObj;

    // Return response
    SHOW_ELAPSED("Full validation", fullValTimer);
    return makeResponse (docIdString, valStatus, xmlErrs, xmlToSend);

} // validateDoc


/*
 * Front end function to all validation.
 * Called by validateDoc to perform validation on behalf of a client.
 * Also called by CdrPutDoc when adding or replacing a document in
 *   the database.
 *
 * See CdrValidateDoc.h
 */

cdr::String cdr::execValidateDoc (
    cdr::CdrDoc          *docObj,
    cdr::ValidRule       validRule,
    const cdr::String&   validationTypes
) {
    // Should not get here for control docs, but secondary check just in case
    // Just say it's valid if this is one of them.
    if (docObj->isControlType())
        return L"V";

    bool              parseOK = false;
    cdr::String       docTypeString = docObj->getTextDocType();

    MAKE_TIMER(execValTimer);
    // Document status: 'V'alid, 'I'nvalid, or 'M'alformed
    // Invalid until found otherwise
    const wchar_t* status = L"I";

    // Check for private-use characters, which we don't allow (request #3823)
    // Will append to the error list from the document object
    // Note: We don't try to find out what element has the invalid character,
    //   or what it was, we're just trying to find out if there is at least
    //   one.  The client will need to find it.
    cdr::String serializedDoc = docObj->getXml();
    const wchar_t *pos = serializedDoc.c_str();
    const wchar_t *end = pos + serializedDoc.size();
    while (pos < end) {
        unsigned int c = (unsigned int) *pos++;
        if (c >= 0xE000 && c <= 0xF8FF) {
            // At least one char was found
            docObj->addError(
                    L"Document contains one or more private use characters");
            // Don't need to check for more
            break;
        }
    }

    // Get a parse tree for the XML
    if (docObj->parseAvailable()) {
        cdr::dom::Element docXml = docObj->getDocumentElement();

        // Validate the document against the schema if appropriate.
        if (validationTypes.empty()
        ||   validationTypes.find(L"Schema") != validationTypes.npos) {
            MAKE_TIMER(schemaValTimer);
            cdr::validateDocAgainstSchema (docXml, docTypeString,
                                        docObj->getConn(), docObj->getValCtl());
            SHOW_ELAPSED("Schema validation", schemaValTimer);
            // If there were schema errors and we aren't supposed to update
            //   the database in the event of errors, reset the flag to
            //   prevent updates
            if (validRule == cdr::UpdateIfValid && docObj->getErrorCount() > 0)
                validRule = cdr::ValidateOnly;
        }

        // Validate links if appropriate
        if (validationTypes.empty()
                || validationTypes.find(L"Links") != validationTypes.npos) {
            MAKE_TIMER(linkValTimer);
            cdr::link::cdrSetLinks (docObj, (cdr::dom::Node) docXml,
                                    validRule);
            SHOW_ELAPSED("Link validation", linkValTimer);
        }
    }

    else {
        // Add this to the error messages.  Parse error msg already there
        docObj->addError (L"Document malformed.  Validation not performed");

        // Strictly speaking, this is not a validation error message
        docObj->getValCtl().setLastErrorType(cdr::ETYPE_OTHER);

        // Set status in databse to malformed
        status = L"M";
    }

    MAKE_TIMER(valCleanupTimer);
    // If not malformed, note the outcome of the validation.
    if (wcscmp(status, L"M")) {
        // Were there any true errors, not just warnings?
        int hardErrorCount = 0;
        ValidationControl& valCtl = docObj->getValCtl();
        hardErrorCount += valCtl.getLevelCount(cdr::ELVL_ERROR);
        hardErrorCount += valCtl.getLevelCount(cdr::ELVL_FATAL);

        // Set return status accordingly
        status = hardErrorCount ? L"I" : L"V";
    }

    // Update database and object status
    docObj->setValStatus(status);
    if (validRule == cdr::UpdateUnconditionally ||
        (validRule == cdr::UpdateIfValid && docObj->getErrorCount() == 0)) {

        // Apply any required PermaTargId changes - very rare
        // Only updated the link_permatarg table if validRule says update
        docObj->applyPermaTargChanges();

        // Update validation status
        setValStatus (docObj->getConn(), docObj->getId(), status);
    }

    // Report the outcome
    SHOW_ELAPSED("Validation cleanup", valCleanupTimer);
    SHOW_ELAPSED("execValidateDoc", execValTimer);
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
 *
 *  @exception  cdr::Exception  if database or parsing error encountered
 */
void cdr::validateDocAgainstSchema(
        cdr::dom::Element&              docElem,
        const cdr::String&              docTypeName,
        cdr::db::Connection&            conn,
        ValidationControl&              errCtl
) {
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
                                  errCtl, &conn);
}

/**
 * Sends a response buffer to the client reporting the outcome of the
 * document validation process.
 */
cdr::String makeResponse(const cdr::String& docId,
                         const cdr::String& status,
                         const cdr::String& errors,
                         const cdr::String& xmlDoc)
{
    // Invariant part of response
    cdr::String response = L"  <CdrValidateDocResp "
                           L"xmlns:cdr='cips.nci.nih.gov/cdr'>\n"
                           L"   <DocId>"
                         + docId
                         + L"</DocId>\n   <DocActiveStatus>"
                         + status
                         + L"</DocActiveStatus>\n"
                         + errors
                         + xmlDoc
                         + L"  </CdrValidateDocResp>\n";

    return response;
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
