/*
 * $Id: CdrValidateDoc.cpp,v 1.3 2000-04-27 13:10:49 bkline Exp $
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * $Log: not supported by cvs2svn $
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
#include "CdrDbResultSet.h"
#include "CdrXsd.h"
#include "CdrRegEx.h"

// Local functions.
static void extractDoc(
        cdr::dom::Element&              docElement, 
        cdr::dom::Node&                 wrapperNode,
        cdr::String&                    schemaString, 
        cdr::String&                    docIdString,
        cdr::String&                    docTypeString,
        cdr::db::Connection&            dbConnection);
static void retrieveDoc(
        cdr::dom::Element&              docElement, 
        cdr::String&                    schemaString, 
        cdr::String&                    docIdString,
        cdr::String&                    docTypeString,
        cdr::db::Connection&            dbConnection);
static cdr::String makeResponse(
        cdr::String&                    docIdString,
        const wchar_t*                  status,
        cdr::StringList&                errors);
static void setDocStatus(
        cdr::db::Connection&            dbConnection,
        int                             docId,
        const wchar_t*                  status);
static void validateElement(
        cdr::dom::Element&              docElement,
        const cdr::xsd::Type&           type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors);
static void verifyAttributes(
        cdr::dom::Element&              element, 
        const cdr::xsd::ComplexType&    type,
        cdr::StringList&                errors);
static void validateSimpleType(
        cdr::dom::Element&              docElement,
        const cdr::xsd::SimpleType&     simpleType,
        cdr::StringList&                errors);
static void verifyNoText(
        cdr::dom::Element&              docElement,
        cdr::StringList&                errors);
static void verifyElementList(
        cdr::dom::Element&              docElement,
        const cdr::xsd::ComplexType&    parentType,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors);
static void verifyNoElements(
        cdr::dom::Element&              docElement,
        cdr::StringList&                errors);
static void verifyElements(
        cdr::dom::Element&              docElement,
        const cdr::xsd::ComplexType&    type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors);
static void checkMaxInclusive(
        const cdr::String&              name,
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void checkMinInclusive(
        const cdr::String&              name,
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateTimeInstant(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateUri(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateBinary(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateInteger(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateDecimal(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateTime(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateDate(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors); 
static bool matchPattern(
        const wchar_t*                  pattern, 
        const cdr::String&              value);
static bool matchPattern(
        const cdr::String&              pattern, 
        const cdr::String&              value);

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
                            session.lastStatus = L"failure";
                            return makeResponse(docIdString, L"U", errors);
                        }
                        memoryOnly = false;
                        retrieveDoc(docElement, schemaString, docIdString, 
                                    docTypeString, dbConnection);
                    }
                    else
                        throw cdr::Exception(L"Unexpected element", name);
                    if (!session.canDo(dbConnection, L"VALIDATE DOCUMENT",
                                       docTypeString)) {
                        errors.push_back(L"User not authorized for action");
                        session.lastStatus = L"failure";
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
                session.lastStatus = L"failure";
                return makeResponse(docIdString, L"U", errors);
            }
            cdr::dom::Parser parser;
            parser.parse(schemaString);
            cdr::xsd::Schema schema(parser.getDocument().getDocumentElement());

            // Use the schema to validate the document
            cdr::xsd::Element schemaElement = schema.getTopElement();
            if (schemaElement.getName() != docElement.getNodeName()) {
                cdr::String err;
                err = cdr::String(L"Wrong element at top of document: ")
                    + L" (expected) "
                    + schemaElement.getName()
                    + L")";
                errors.push_back(err);
            }
            else {
                const cdr::xsd::Type& elementType =
                    *schemaElement.getType(schema);
                validateElement(docElement, elementType, schema, errors);
            }

            // Validate the links if appropriate XXX NOT YET IMPLEMENTED
#if 0
            if (!memoryOnly)
                validateLinks(docElement, 
                              session, 
                              docIdString.getDocId());
#endif

            // Report the outcome.
            const wchar_t* status = errors.size() > 0 ? L"I" : L"V";
            if (errors.size() > 0)
                session.lastStatus = L"warning";
            return makeResponse(docIdString, status, errors);
        }

        // First handler for all exceptions.
        catch (...) {

            // No matter which problem we hit here, set status to failure.
            session.lastStatus = L"failure";

            /*
             * If we can't determine the doc type, we can't tell whether
             * the user is authorized to request the validation action.
             */
            if (docTypeString.size() == 0) {
                errors.push_back(L"Unable to determine document type");
                //return makeResponse(docIdString, L"U", errors);
                throw;
            }

            // Don't do anything further if user not authorized for command.
            if (!session.canDo(dbConnection, L"VALIDATE DOCUMENT", 
                               docTypeString)) {
                errors.push_back(L"User not authorized for requested action");
                //return makeResponse(docIdString, L"U", errors);
                throw;
            }

            // Record the failure to determine validity if appropriate.
            if (!memoryOnly && docIdString.size() > 0)
                setDocStatus(dbConnection, docIdString.extractDocId(), L"U");

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
    catch (std::exception& stdEx) {
        cdr::String err = cdr::String(L"Standard exception caught: ")
                        + cdr::String(stdEx.what());
        errors.push_back(err);
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
void extractDoc(
        cdr::dom::Element&      docElement, 
        cdr::dom::Node&         wrapperNode,
        cdr::String&            schemaString, 
        cdr::String&            docIdString,
        cdr::String&            docTypeString,
        cdr::db::Connection&    dbConnection)
{
    // Extract attributes from the <CdrDoc> element.
    cdr::dom::Element& cdrDoc   = static_cast<cdr::dom::Element&>(wrapperNode);
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
        cdr::db::Connection&    dbConnection)
{
    // Submit a query to retrieve the document and schema from the database.
    cdr::db::Statement select(dbConnection);
    int id = docIdString.extractDocId();
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
    docTypeString       = rs.getString(3);
    if (docXml.size() == 0)
        throw cdr::Exception(L"XML for document is empty");
    if (docTypeString.size() == 0)
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
    if (errors.size() > 0) {
        response += L"   <Errors>\n";
        cdr::StringList::iterator i = errors.begin();
        while (i != errors.end())
            response += L"    <Error>" + *i++ + L"</Error>\n";
        response += L"   </Errors>\n";
    }
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
    cdr::db::Statement update(conn);
    update.setString(1, status);
    update.setInt(2, id); 
    update.executeQuery("UPDATE document"
                        "   SET val_status = ?,"
                        "       val_date = GETDATE()"
                        " WHERE id = ?");
}

/**
 * Verify that the required attributes are present. and that no
 * attributes are present which are undefined for this element
 * type or which do not meet the type requirements for the
 * attribute.
 */
void verifyAttributes(
        cdr::dom::Element&              e, 
        const cdr::xsd::ComplexType&    t,
        cdr::StringList&                errors) 
{
    std::wcerr << L"Stub for verifyAttributes...\n";
}

/**
 * Recursively validates specified element against its schema specification.
 */
void validateElement(
        cdr::dom::Element&              docElement,
        const cdr::xsd::Type&           type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors)
{
    const cdr::xsd::SimpleType* simpleType;
    const cdr::xsd::ComplexType* complexType;
    simpleType  = dynamic_cast<const cdr::xsd::SimpleType*>(&type);
    complexType = dynamic_cast<const cdr::xsd::ComplexType*>(&type);
    if (simpleType)
        validateSimpleType(docElement, *simpleType, errors);
    else {
        switch (complexType->getContentType()) {
        case cdr::xsd::ComplexType::EMPTY:
            verifyNoText(docElement, errors);
            verifyNoElements(docElement, errors);
            break;
        case cdr::xsd::ComplexType::ELEMENT_ONLY:
            verifyNoText(docElement, errors);
            verifyElementList(docElement, *complexType, schema, errors);
            break;
        case cdr::xsd::ComplexType::TEXT_ONLY:
            verifyNoElements(docElement, errors);
            break;
        case cdr::xsd::ComplexType::MIXED:
            verifyElements(docElement, *complexType, schema, errors);
            break;
        }

        verifyAttributes(docElement, *complexType, errors);
    }
}

/**
 * Determines whether the specified <code>value</code> matches the regular 
 * expression represented by <code>pattern</code>.
 */
bool matchPattern(
        const cdr::String&              pattern, 
        const cdr::String&              value)
{
    return matchPattern(pattern.c_str(), value);
}

/**
 * Determines whether the specified <code>value</code> matches the regular 
 * expression represented by <code>pattern</code>.
 */
bool matchPattern(
        const wchar_t*                  pattern, 
        const cdr::String&              value)
{
    cdr::RegEx re(pattern);
    return re.match(value);
}

/**
 * Reports any errors found with a date value.
 */
void validateDate(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^\\d\\d\\d\\d-\\d\\d-\\d\\d$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid date value: '")
                        + val
                        + L"' in element "
                        + name;
        errors.push_back(err);
    }
}

/**
 * Reports any errors found with a time value.
 */
void validateTime(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] =
        L"^\\d\\d:\\d\\d((:\\d\\d)(\\.\\d+)?)?( ?[-+]\\d{1,2}:\\d{2})?$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid time value: '")
                        + val
                        + L"' in element "
                        + name;
        errors.push_back(err);
    }
}

/**
 * Reports any errors found with a decimal value.
 */
void validateDecimal(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^\\d+(\\.\\d*)?|\\d*\\.\\d+$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid decimal value: '")
                        + val
                        + L"' in element "
                        + name;
        errors.push_back(err);
    }
    int precision = t.getPrecision();
    int scale     = t.getScale();
    if (precision != -1 || scale != -1) {
        int valPrecision       = 0;
        int valScale           = 0;

        // Skip past leading zeroes.
        size_t i = 0;
        while (i < val.size() && val[i] == L'0')
            ++i;

        // You get at least 1 digit of precision even if the value is zero.
        if (i == val.size())
            valPrecision = 1;

        while (i < val.size()) {

            // Check for the decimal point.
            if (val[i] == L'.') {

                // Back up past trailing zeroes.
                size_t j = val.size() - 1;
                while (j > i && val[j] == L'0')
                    --j;
                valScale;
                valPrecision += valScale;
                break;
            }
            ++valPrecision;
            ++i;
        }

        // Check the scale if appropriate.
        if (scale != -1 && valScale > scale) {
            cdr::String err = cdr::String(L"Invalid scale for element ")
                            + name 
                            + L": '"
                            + val
                            + L"'";
            errors.push_back(err);
        }

        // Check the precision.
        if (precision != -1 && valPrecision > precision) {
            cdr::String err = cdr::String(L"Invalid precision for element ")
                            + name 
                            + L": '"
                            + val
                            + L"'";
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with an integer value.
 */
void validateInteger(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^\\d+$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid integer value: '")
                        + val
                        + L"' in element "
                        + name;
        errors.push_back(err);
    }

    int precision = t.getPrecision();
    if (precision != -1) {

        int valPrecision       = 0;

        // Skip past leading zeroes.
        size_t i = 0;
        while (i < val.size() && val[i] == L'0')
            ++i;

        // You get at least 1 digit of precision even if the value is zero.
        if (i == val.size())
            valPrecision = 1;
        else
            valPrecision = val.size() - i;

        // Check the precision.
        if (valPrecision > precision) {
            cdr::String err = cdr::String(L"Invalid precision for element ")
                            + name 
                            + L": '"
                            + val
                            + L"'";
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with a URI value.
 */
void validateUri(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    std::wcout << L"validateUri stub: " << name << L"=" << val << L"\n";
}

/**
 * Reports any errors found with a binary value.
 */
void validateBinary(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    // @see RFC 2045; ignoring 76-character/line limit.
    static const wchar_t base64Pattern[] = L"([A-Za-z0-9+/\\s]*=?=?";
    static const wchar_t hexPattern[] = L"^([0-9a-fA-F]{2}|\\s)*$";
    cdr::xsd::SimpleType::Encoding encoding = t.getEncoding();
    if (encoding == cdr::xsd::SimpleType::HEX) {
        if (!matchPattern(hexPattern, val)) {
            cdr::String err = cdr::String(L"Invalid HEX encoding: '")
                            + val
                            + L"' in element "
                            + name;
            errors.push_back(err);
        }
    }
    else {
        if (!matchPattern(base64Pattern, val)) {
            cdr::String err = cdr::String(L"Invalid base-64 encoding: '")
                            + val
                            + L"' in element "
                            + name;
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with a date/time value.
 */
void validateTimeInstant(
        const cdr::String&              name, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] =
        L"^\\d\\d\\d\\d-\\d\\d-\\d\\dT"
        L"\\d\\d:\\d\\d((:\\d\\d)(\\.\\d+)?)?( ?[-+]\\d{1,2}:\\d{2})?$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid date/time value: '")
                        + val
                        + L"' in element "
                        + name;
        errors.push_back(err);
    }
}

/**
 * Determines whether the value is less than any minimum specified.
 */
void checkMinInclusive(
        const cdr::String&              name,
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{
    if (t.getMinInclusive().size() > 0) {
        switch (t.getBuiltinType()) {
        case cdr::xsd::SimpleType::DECIMAL:
            if (value.getFloat() < t.getMinInclusive().getFloat()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is below the minimum '"
                                + t.getMinInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        case cdr::xsd::SimpleType::INTEGER:
            if (value.getInt() < t.getMinInclusive().getInt()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is below the minimum '"
                                + t.getMinInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        default:
            if (value < t.getMinInclusive()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is below the minimum '"
                                + t.getMinInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        }
    }
}

/**
 * Determines whether the value is greater than any maximum specified.
 */
void checkMaxInclusive(
        const cdr::String&              name,
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{
    if (t.getMaxInclusive().size() > 0) {
        switch (t.getBuiltinType()) {
        case cdr::xsd::SimpleType::DECIMAL:
            if (value.getFloat() > t.getMaxInclusive().getFloat()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is above the maximum '"
                                + t.getMaxInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        case cdr::xsd::SimpleType::INTEGER:
            if (value.getInt() > t.getMaxInclusive().getInt()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is above the maximum '"
                                + t.getMaxInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        default:
            if (value < t.getMaxInclusive()) {
                cdr::String err = cdr::String(L"'")
                                + value
                                + L"' is above the maximum '"
                                + t.getMaxInclusive()
                                + L"' allowed for element "
                                + name;
                errors.push_back(err);
            }
            break;
        }
    }
}

/**
 * Checks the value of a text value against its simple type requirements.
 */
void validateSimpleType(
        cdr::dom::Element&              docElement,
        const cdr::xsd::SimpleType&     simpleType,
        cdr::StringList&                errors)
{
    cdr::String name = docElement.getNodeName();
    cdr::String value = cdr::dom::getTextContent(docElement);
    switch (simpleType.getBuiltinType()) {
    case cdr::xsd::SimpleType::STRING:
        std::wcout << L"validateSimpleType: STRING\n";
        break;
    case cdr::xsd::SimpleType::DATE:
        validateDate(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::TIME:
        validateTime(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::DECIMAL:
        validateDecimal(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::INTEGER:
        validateInteger(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::URI:
        validateUri(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::BINARY:
        validateBinary(name, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::TIME_INSTANT:
        validateTimeInstant(name, value, simpleType, errors);
        break;
    default:
        throw cdr::Exception(L"Unrecognized base type for element", name);
    }

    // Check the value length.
    int valueLen = value.size();
    int typeLen  = simpleType.getLength();
    int minLen   = simpleType.getMinLength();
    int maxLen   = simpleType.getMaxLength();
    if (typeLen != -1 && valueLen != typeLen
    ||  minLen  != -1 && valueLen <  minLen
    ||  maxLen  != -1 && valueLen >  maxLen) {
        cdr::String err = cdr::String(L"Invalid length for element ")
                        + name 
                        + L": '"
                        + value
                        + L"'";
        errors.push_back(err);
    }

    // Check pattern constraints.
    const cdr::StringVector& patterns = simpleType.getPatterns();
    if (patterns.size() > 0) {
        bool foundMatch = false;
        for (size_t i = 0; !foundMatch && i < patterns.size(); ++i)
            foundMatch = matchPattern(patterns[i], value);
        if (!foundMatch) {
            cdr::String err = cdr::String(L"Pattern constraints not matched"
                                          L"for element ")
                            + name 
                            + L": '"
                            + value
                            + L"'";
            errors.push_back(err);
        }
    }
    const cdr::StringSet& enums = simpleType.getEnumSet();
    if (enums.size() > 0) {
        bool foundMatch = false;
        cdr::StringSet::const_iterator i = enums.begin();
        while (!foundMatch && i != enums.end())
            foundMatch = *i++ == value;
        if (!foundMatch) {
            cdr::String err = cdr::String(L"'")
                            + value
                            + L"' is not a valid "
                            + name
                            + L" value.";
            errors.push_back(err);
        }
    }

    // Check for upper and lower bounds on the value space.
    checkMinInclusive(name, value, simpleType, errors);
    checkMaxInclusive(name, value, simpleType, errors);
}

/**
 * Make sure that the element contains no text content, other than whitespace.
 * Used for ELEMENT_ONLY and for EMPTY content types.
 */
void verifyNoText(
        cdr::dom::Element&              docElement,
        cdr::StringList&                errors)
{
    cdr::String value = cdr::dom::getTextContent(docElement);
    if (value.find_first_not_of(L" \t\r\n") != value.npos)
        errors.push_back(cdr::String(L"No text content allowed for element")
                + cdr::String(docElement.getNodeName()));
}

/**
 * Verify that the child elements appear in the order prescribed, that they
 * meet the requirements for their individual types, and that the number of
 * occurrences of each meets the minOccurs and maxOccurs requirements.
 */
void verifyElementList(
        cdr::dom::Element&              docElement,
        const cdr::xsd::ComplexType&    parentType,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors)
{
    cdr::xsd::ElemEnum i = parentType.getElements();
    cdr::dom::Node     n = docElement.getFirstChild();
    cdr::String        parentName = docElement.getNodeName();
    cdr::String        occName;

    // Check each expected element for the type.
    while (i != parentType.getElemEnd()) {
        cdr::xsd::Element* e = *i++;
        int nOccs = 0;

        // Skip nodes which aren't elements.
        while (n != 0) {
            if (n.getNodeType() != cdr::dom::Node::ELEMENT_NODE)
                n = n.getNextSibling();
            else
                break;
        }

        // Check for too few occurrences.
        occName = n.getNodeName();
        if (n == 0 || occName != e->getName()) {
            if (nOccs < e->getMinOccs()) {
                cdr::String err;
                if (nOccs == 0)
                    err = cdr::String(L"Missing required element ")
                        + e->getName()
                        + L" within element "
                        + parentName;
                else
                    err = cdr::String(L"Too few occurrences of element ")
                        + e->getName()
                        + L" within element "
                        + parentName;
                errors.push_back(err);
            }

            // Skip to next expected element.
            continue;
        }

        // Check for too many occurrences.
        if (nOccs > e->getMaxOccs()) {
            cdr::String err = cdr::String(L"Too many occurrences of element ")
                            + e->getName()
                            + L" within element "
                            + parentName;
            errors.push_back(err);
        }

        // Recursively check the element.
        validateElement(static_cast<cdr::dom::Element&>(n), 
                        *e->getType(schema), schema, errors);
        n = n.getNextSibling();
    }

    // Complain about leftover elements which weren't expected.
    while (n != 0) {

        // Skip nodes which aren't elements.
        if (n.getNodeType() != cdr::dom::Node::ELEMENT_NODE) {
            n = n.getNextSibling();
            continue;
        }

        occName = n.getNodeName();
        cdr::String err = cdr::String(L"Unexpected element ")
                        + occName
                        + L" within element "
                        + parentName;
        errors.push_back(err);

        // Find the element's type so we can validate it.
        cdr::String typeName = schema.lookupElementType(occName);
        if (typeName.empty()) {
            cdr::String err = cdr::String(L"Unable to find type for element ")
                            + occName;
            errors.push_back(err);
        }
        const cdr::xsd::Type* type = schema.lookupType(typeName);
        if (!type) {
            cdr::String err = cdr::String(L"Unable to find type ")
                            + typeName
                            + L" for element "
                            + occName;
            errors.push_back(err);
        }
        validateElement(static_cast<cdr::dom::Element&>(n), 
                        *type, schema, errors);
        n = n.getNextSibling();
    }
}

/**
 * Verify that the element has no sub-elements (used for TEXT_ONLY and EMPTY
 * content type and simple types).
 */
void verifyNoElements(
        cdr::dom::Element&              docElement,
        cdr::StringList&                errors)
{
    cdr::dom::Node child = docElement.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            errors.push_back(
                    cdr::String(L"Sub-elements not allowed for element ")
                                + cdr::String(docElement.getNodeName()));
            return;
        }
        child = child.getNextSibling();
    }
}

/**
 * Verify elements in a Mixed-content element.  Sub-elements can occur in any
 * order, as long as they are defined for the parent element, and as long as
 * they are valid themselves.
 */
void verifyElements(
        cdr::dom::Element&              docElement,
        const cdr::xsd::ComplexType&    type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors)
{
    cdr::String parentName = docElement.getNodeName();
    cdr::dom::Node child = docElement.getFirstChild();
    for ( ; child != 0; child = child.getNextSibling()) {
        if (child.getNodeType() != cdr::dom::Node::ELEMENT_NODE) 
            continue;
        cdr::String name     = child.getNodeName();
        cdr::String typeName = schema.lookupElementType(name);
        if (typeName.empty()) {
            cdr::String err = cdr::String(L"Undefined element ")
                            + name;
            errors.push_back(err);
            continue;
        }
        const cdr::xsd::Type* childType = schema.lookupType(typeName);
        if (!childType) {
            cdr::String err = cdr::String(L"Undefined type ")
                            + name;
            errors.push_back(err);
            continue;
        }
        if (!type.hasElement(name)) {
            cdr::String err = cdr::String(L"Element ")
                            + name
                            + L" not allowed as part of element "
                            + parentName;
            errors.push_back(err);
        }
        validateElement(static_cast<cdr::dom::Element&>(child), 
                        *childType, schema, errors);
    }
}
