/*
 * $Id: CdrValidateDoc.cpp,v 1.1 2000-04-25 03:42:18 bkline Exp $
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * $Log: not supported by cvs2svn $
 */

// Project headers.
#include "CdrCommand.h"
#include "CdrDbResultSet.h"

// Local functions.
static void extractDoc (
        cdr::dom::Element&      docElement, 
        cdr::dom::Node&         wrapperNode,
        cdr::String&            schemaString, 
        cdr::String&            docIdString,
        cdr::String&            docTypeString,
        cdr::db::Connection&    dbConnection);
static void retrieveDoc(
        cdr::dom::Element&      docElement, 
        cdr::String&            schemaString, 
        cdr::String&            docIdString,
        cdr::String&            docTypeString,
        cdr::db::Connection&    dbConnection);
static cdr::String makeResponse(
        cdr::String&            docIdString,
        cdr::String&            status,
        cdr::StringList&        errors);
static void setDocStatus(
        cdr::db::Connection&    dbConnection,
        int                     docId,
        cdr::String&            status);
static void validateElement(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors);
static void validateSimpleType(
        cdr::dom::Element&      docElement,
        cdr::xsd::SimpleType&   simpleType,
        cdr::StringList&        errors);
static void verifyNoText(
        cdr::dom::Element&      docElement,
        cdr::StringList&        errors);
static void verifyElementList(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors);
static void verifyNoElements(
        cdr::dom::Element&      docElement,
        cdr::StringList&        errors);
static void verifyElements(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors);

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
cdr::String cdr::validateDoc(cdr::Session& session, 
                             const cdr::dom::Node& commandNode,
                             cdr::db::Connection& dbConnection) 
{
    // Extract the data elements from the command node.
    cdr::String         docIdString;
    cdr::String         docTypeString;
    cdr::StringList     errors;
    bool                memoryOnly = true;

    // Catch our own exceptions so we can set DocStatus appropriately.
    try {

        // Nested `try' to consolidate some of the common error handling.
        try {
            cdr::String         schemaString;
            cdr::dom::Element   docElement;
            cdr::dom::Node      child = commandNode.getFirstChild();
            while (child != 0) {
                if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                    cdr::String name = child.getNodeName();
                    if (name == L"CdrDoc") {
                        extractDoc(docElement, child, schemaString,
                                   docIdString, docTypeString, dbConnection);
                    }
                    else if (name == L"DocId") {
                        docIdString = cdr::dom::getTextContent(child);
                        if (docIdString.size() == 0) {
                            errors.push_back(L"Empty document ID");
                            return makeResponse(docIdString, L"U", errors);
                        }
                        memoryOnly = false;
                        retrieveDoc(docElement, schemaString, docIdString, 
                                    docTypeString, dbConnection);
                    }
                    else
                        throw cdr::Exception(L"Unexpected element", name);
                    if (!session.canDo(dbConnection, L"VALIDATE DOC",
                                       docTypeString)) {
                        errors.push_back(L"User not authorized for action");
                        return makeResponse(docIdString, L"U", errors);
                    }
                    break;
                }
                child = child.getNextSibling();
            }

            // We have the parsed document; now compile the schema.
            if (schemaString.size() == 0) {
                errors.push_back(cdr::String(
                            L"Schema missing for document type: ") 
                            + docTypeString);
                return makeResponse(docIdString, L"U", errors);
            }
            cdr::dom::Parser parser;
            parser.parse(schemaString);
            cdr::dom::Document document = parser.getDocument();
            cdr::dom::Element docElement = document.getDocumentElement();
            cdr::xsd::Schema schema(docElement);

            // Use the schema to validate the document
            cdr::xsd::Element schemaElement = schema.getTopElement();
            if (schemaElement.getName() != docElement.getNodeName()) {
                cdr::String err = L"Wrong element at top of document: ";
                err += docElement.getNodeName() 
                    + L" (expected) "
                    +  schemaElement
                    + L")";
                errors.push_back(err);
            }
            else 
                validateElement(docElement, schemaElement, schema, errors);

            // Validate the links if appropriate XXX NOT YET IMPLEMENTED
#if 0
            if (!memoryOnly)
                validateLinks(docElement, 
                              session, 
                              docIdString.getDocId());
#endif

            // Report the outcome.
            cdr::String status = errors.size() > 0 ? L"I" : L"U";
            return makeResponse(docIdString, status, errors);
        }

        // First handler for all exceptions.
        catch (...) {

            /*
             * If we can't determine the doc type, we can't tell whether
             * the user is authorized to request the validation action.
             */
            if (docTypeString.size() == 0) {
                errors.push_back(L"Unable to determine document type");
                return makeResponse(docIdString, L"U", errors);
            }

            // Don't do anything further if user not authorized for command.
            if (!session.canDo(dbConnection, L"VALIDATE DOC", docTypeString)) {
                errors.push_back(L"User not authorized for requested action");
                return makeResponse(docIdString, L"U", errors);
            }

            // Record the failure to determine validity if appropriate.
            if (!memoryOnly && docIdString.size() > 0)
                setDocStatus(docIdString.extractDocId(), L"U");

            // Now we can use specific exception handlers.
            throw;
        }
    }
    catch (const cdr::Exception& cdrEx) {
        errors.push_back(cdrEx.getString());
        return makeResponse(docIdString, L"U", errors);
    }
    catch (const cdr::dom::DOMException& ex) {
        errors.push_back(cdr::String(ex.getMessage()));
        return makeResponse(docIdString, L"U", errors);
    }
    catch (...) {
        errors.push_back(L"Unexpected exception caught");
        return makeResponse(docIdString, L"U", errors);
    }
}

/**
 * Extract document from wrapper DOM node and CDATA section.
 */
void extractDoc (
        cdr::dom::Element&      docElement, 
        cdr::dom::Node&         wrapperNode,
        cdr::String&            schemaString, 
        cdr::String&            docIdString,
        cdr::String&            docTypeString,
        cdr::db::Connection&    dbConnection);
{
    // Extract attributes from the <CdrDoc> element.
    cdr::dom::Element cdrDoc    = static_cast<cdr::dom::Element>(wrapperNode);
    docIdString                 = cdrDoc.getAttribute(L"Id");
    docTypeString               = cdrDoc.getAttribute(L"Type");

    // Look up the schema for the document type
    if (docTypeString.empty())
        throw cdr::Exception(L"Missing document type attribute");
    cdr::db::Statement select(dbConnection);
    select.setString(1, docTypeString);
    cdr::db::ResultSet rs = select.executeQuery("SELECT xml_schema"
                                                "  FROM doc_type"
                                                " WHERE name = ?");
    if (!rs.next())
        throw cdr::Exception(L"Unknown document type", docTypeString);
    schemaString = rs.getString(1);

    // Pull the XML out of the wrapper.
    bool foundDocument = false;
    cdr::dom::Node child = wrapperNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"CdrDocXml") {

                // Look for the CDATA section.
                cdr::dom::Node cdata = child.getFirstChild();
                while (cdata != 0) {
                    if (cdata.getNodeType() ==
                            cdr::dom::Node::CDATA_SECTION_NODE) {

                        // Parse the CDATA section and grab the top element.
                        cdr::dom::Parser parser;
                        parser.parse(cdata.getNodeValue());
                        cdr::dom::Document document = parser.getDocument();
                        docElement = document.getDocumentElement();
                        foundDocument = true;
                        break;
                    }
                }
                break;
            }
        }
    }
    if (!foundDocument)
        throw cdr::Exception(L"XML for document not found");
}

/**
 * Retrieves the document from the database and parses it.
 */
void retrieveDoc(
        cdr::dom::Element&      docElement, 
        cdr::String&            schemaString, 
        cdr::String&            docIdString,
        cdr::String&            docTypeString,
        cdr::db::Connection&    dbConnection);
{
    // Submit a query to retrieve the document and schema from the database.
    cdr::db::Statement select(dbConnection);
    int id = docId.extractDocId();
    select.setInt(1, id);
    cdr::db::ResultSet rs = select.executeQuery("SELECT d.xml,"
                                                "       t.xml_schema,"
                                                "       t.name"
                                                "  FROM document d,"
                                                "       doc_type t"
                                                " WHERE d.id = ?"
                                                "   AND t.id = d.doc_type");
    if (!rs.next())
        throw cdr::Exception(L"Document not found", docIdString);
    cdr::String docXml  = rs.getString(1);
    schemaString        = rs.getString(2);
    docTypeName         = rs.getString(3);
    if (doc.size() == 0)
        throw cdr::Exception(L"XML for document is empty");
    if (doctypeName.size() == 0)
        throw cdr::Exception(L"Unable to retrieve document type name");

    // Parse the XML for the document.
    cdr::dom::Parser parser;
    parser.parse(docXml);
    cdr::dom::Document document = parser.getDocument();
    docElement = parser.getDocument().getDocumentElement();
}

/**
 * Sends a response buffer to the client reporting the outcome of the
 * document validation process.
 */
cdr::String makeResponse(cdr::String& docId,
                         cdr::String& status,
                         cdr::StringList& errors)
{
    cdr::String response = L"  <CdrValidateDocResp>\n"
                           L"   <DocId>
                         + docId
                         + L"</DocId>\n"   <DocStatus>"
                         + status
                         + L"</DocStatus>\n";
    if (errors.size() > 0) {
        response += L"   <Errors>\n";
        cdr::StringList::iterator i = errors.begin();
        while ( != errors.end())
            response += L"    <Error> + *i++ + </Error>\n";
        response += L"   </Errors>\n";
    }
    return response + L"  </CdrValidateDocResp>\n";
}

/**
 * Records the new status of the document in the database.
 */
void setDocStatus(cdr::db::Connection& conn, int id, cdr::String& status)
{
    cdr::db::Statement update(conn);
    update.setString(1, status);
    update.setInt(2, id); 
    update.executeQuery("UPDATE document"
                        "   SET val_status = ?,"
                        "       val_date = GETDATE()"
                        " WHERE id = ?");
}

/**
 * Recursively validates specified element against its schema specification.
 */
void validateElement(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors)
{
    cdr::xsd::Type* type = schemaElement.getType(schema);
    cdr::xsd::SimpleType* simpleType;
    cdr::xsd::ComplexType* complexType;
    simpleType  = dynamic_cast<cdr::xsd::SimpleType*>(type);
    complexType = dynamic_cast<cdr::xsd::ComplexType*>(type);
    if (simpleType)
        validateSimpleType(docElement, simpleType, errors);
    else {
        switch (complexType->getContentType()) {
        case cdr::xsd::EMPTY:
            verifyNoText(docElement, errors);
            verifyNoElements(docElement, errors);
            break;
        case cdr::xsd::ELEMENT_ONLY:
            verifyNoText(docElement, errors);
            verifyElementList(docElement, schemaElement, complexType, schema, 
                              errors);
            break;
        case cdr::xsd::TEXT_ONLY:
            verifyNoElements(docElement, errors);
            break;
        case cdr::xsd::MIXED:
            verifyElements(docElement, schemaElement, schema, errors);
            break;
        }

        verifyAttributes(docElement, schemaElement, errors);
    }
}

void validateDate(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateDate stub: " << name << L"=" << val << L"\n";
}
void validateTime(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateTime stub: " << name << L"=" << val << L"\n";
}
void validateDecimal(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateDecimal stub: " << name << L"=" << val << L"\n";
}
void validateInteger(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateInteger stub: " << name << L"=" << val << L"\n";
}
void validateUri(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateUri stub: " << name << L"=" << val << L"\n";
}
void validateBinary(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateBinary stub: " << name << L"=" << val << L"\n";
}
void validateTimeInstant(
        cdr::String& name, 
        cdr::String& val, 
        cdr::xsd::SimpleType& t,
        cdr::StringList& errors) 
{ 
    std::wcout << L"validateTimeInstant stub: " << name << L"=" << val << L"\n";
}

void validateSimpleType(
        cdr::dom::Element&      docElement,
        cdr::xsd::SimpleType&   simpleType,
        cdr::StringList&        errors)
{
    cdr::String name = docElement.getNodeName();
    cdr::String value = cdr::dom::getTextContent(docElement);
    switch (simpleType.getBuiltinType()) {
    case cdr::xsd::simpleType::STRING:
        std::wcout << L"validateSimpleType: STRING\n";
        break;
    case cdr::xsd::simpleType::DATE:
        validateDate(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::TIME:
        validateTime(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::DECIMAL:
        validateDecimal(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::INTEGER:
        validateInteger(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::URI:
        validateUri(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::BINARY:
        validateBinary(name, value, simpleType);
        break;
    case cdr::xsd::simpleType::TIME_INSTANT:
        validateTimeInstant(name, value, simpleType);
        break;
    default:
        throw cdr::Exception(L"Unrecognized base type for element", name);
    }
    errors.push_back(L"Stub for verifyElements");
}

/**
 * Make sure that the element contains no text content, other than whitespace.
 * Used for ELEMENT_ONLY and for EMPTY content types.
 */
void verifyNoText(
        cdr::dom::Element&      docElement,
        cdr::StringList&        errors)
{
    cdr::String value = cdr::dom::getTextContent(docElement);
    if (value.find_first_not_of(L" \t\r\n") != value.npos)
        errors.push_back(cdr::String(L"No text content allowed for element")
                + docElement.getNodeName());
}

/**
 * Verity that the child elements appear in the order prescribed, that they
 * meet the requirements for their individual types, and that the number of
 * occurrences of each meets the minOccurs and maxOccurs requirements.
 */
void verifyElementList(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Type&         parentType,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors)
{
    
    errors.push_back(L"Stub for verifyElements");
}

/**
 * Verify that the element has no sub-elements (used for TEXT_ONLY and EMPTY
 * content type and simple types).
 */
void verifyNoElements(
        cdr::dom::Element&      docElement,
        cdr::StringList&        errors)
{
    errors.push_back(L"Stub for verifyElements");
}

/**
 * Verify elements in a Mixed-content element.  Sub-elements can occur in any
 * order, as long as they are defined for the parent element, and as long as
 * they are valid themselves.
 */
void verifyElements(
        cdr::dom::Element&      docElement,
        cdr::xsd::Element&      schemaElement,
        cdr::xsd::Schema&       schema,
        cdr::StringList&        errors)
{
    errors.push_back(L"Stub for verifyElements");
}
