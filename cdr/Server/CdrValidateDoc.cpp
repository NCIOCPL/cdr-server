/*
 * $Id: CdrValidateDoc.cpp,v 1.5 2000-05-03 15:20:38 bkline Exp $
 *
 * Examines a CDR document to determine whether it complies with the
 * requirements for its document type.
 *
 * $Log: not supported by cvs2svn $
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
#include "CdrRegEx.h"
#include "CdrGetDoc.h"
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
static void validateElement(
        cdr::dom::Element&              docElement,
        const cdr::xsd::Type&           type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors);
static bool validateAttributes(
        cdr::dom::Element&              element, 
        const cdr::xsd::ComplexType&    type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors);
static void validateSimpleType(
        const cdr::String&              elemName,
        const cdr::String&              attrName,
        const cdr::String&              value,
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
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void checkMinInclusive(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateTimeInstant(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateUri(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateBinary(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateInteger(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateDecimal(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateTime(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     type,
        cdr::StringList&                errors);
static void validateDate(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
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
cdr::String cdr::validateDoc(
        cdr::Session&                   session, 
        const cdr::dom::Node&           commandNode,
        cdr::db::Connection&            conn) 
{
    // Make sure the user is authorized to validate this document.
    const cdr::dom::Element& commandElement =
        static_cast<const cdr::dom::Element&>(commandNode);
    cdr::String docTypeString = commandElement.getAttribute(L"DocType");
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
    cdr::dom::Node  docNode;
    cdr::String     docIdString;
    cdr::dom::Node  child = commandNode.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String name = child.getNodeName();
            if (name == L"CdrDoc")
                docNode = child;
            else if (name == L"DocId")
                docIdString = cdr::dom::getTextContent(child);
            else
                throw cdr::Exception(L"Unexpected element", name);
        }
        child = child.getNextSibling();
    }

    // Make sure we got what we need.
    bool updateDocStatus = false;
    int  docId = 0;
    if (docIdString.empty() && docNode == 0)
        throw cdr::Exception(L"Command requires DocId or CdrDoc element");
    if (docNode != 0 && !docIdString.empty())
        throw cdr::Exception(L"Both DocId and CdrDoc specified");
    if (docNode == 0) {
        cdr::String docString = cdr::getDocString(docIdString, conn);
        cdr::dom::Parser parser;
        parser.parse(docString);
        docNode = parser.getDocument().getDocumentElement();
        updateDocStatus = true;

        // Make sure the caller is telling the truth about the DocType.
        cdr::String realDocType;
        cdr::dom::Node docCtlNode = docNode.getFirstChild();
        while (docCtlNode != 0) {
            if (docCtlNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                cdr::String name = docCtlNode.getNodeName();
                if (name == L"CdrDocCtl") {
                    cdr::dom::Node n = docCtlNode.getFirstChild();
                    while (n != 0) {
                        if (n.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                            cdr::String name = n.getNodeName();
                            if (name == L"DocType")
                                realDocType = cdr::dom::getTextContent(n);
                            else if (name == L"DocId")
                                docId = 
                                    cdr::dom::getTextContent(n).extractDocId();
                            if (docId != 0 && !realDocType.empty())
                                break;
                        }
                        n = n.getNextSibling();
                    }
                    break;
                }
            }
            docCtlNode = docCtlNode.getNextSibling();
        }
        if (realDocType.empty())
            throw cdr::Exception(L"Unable to extract document type from "
                                 L"document");
        if (realDocType != docTypeString)
            throw cdr::Exception(L"Command specifies incorrect DocType",
                                 docTypeString);
    }

    // Validate the document against the schema.
    cdr::StringList errors;
    cdr::validateDocAgainstSchema(docNode, docTypeString, conn,
                                  errors,updateDocStatus);

    // Note the outcome of the validation.
    const wchar_t* status = errors.size() > 0 ? L"I" : L"V";
    if (updateDocStatus) {
        setDocStatus(conn, docId, status);

        // Validate the links if appropriate XXX NOT YET IMPLEMENTED
        //validateLinks(docElement, session, docIdString.getDocId());
    }

    // Report the outcome.
    return makeResponse(docIdString, status, errors);
}

/**
 * Checks document against the schema for its document type, reporting any
 * errors found in the caller's <code>Errors</code> vector.  Caller is
 * responsible for ensuring that the docTypeName is correct for this
 * document.
 *
 *  @param  docNode             DOM node for CdrDoc element
 *  @param  docTypeName         string for name of document's type
 *  @param  conn                reference to CDR database connection object
 *  @param  errors              vector of strings to be populated by this
 *                              function
 *  @param  updateDocStatus     if <code>true</code> update the doc_status
 *                              column of the document table
 *
 *  @exception  cdr::Exception  if database or parsing error encountered
 */
void cdr::validateDocAgainstSchema(
        const cdr::dom::Node&           docNode,
        const cdr::String&              docTypeName,
        cdr::db::Connection&            conn,
        cdr::StringList&                errors,
        bool                            updateDocStatus)
{
    // Pull the XML out of the wrapper.
    bool foundDocument = false;
    cdr::dom::Element docXmlElement;
    cdr::dom::Node child = docNode.getFirstChild();
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
                        docXmlElement = document.getDocumentElement();
                        foundDocument = true;
                        break;
                    }
                    cdata = cdata.getNextSibling();
                }
                break;
            }
        }
        child = child.getNextSibling();
    }
    if (!foundDocument)
        throw cdr::Exception(L"CdrDocXml element for document not found");

    // Go get the schema for the document's type.
    std::string query = "SELECT xml_schema FROM doc_type WHERE name = ?";
    cdr::db::PreparedStatement select = conn.prepareStatement(query);
    select.setString(1, docTypeName);
    cdr::db::ResultSet rs = select.executeQuery();
    if (!rs.next())
        throw cdr::Exception(L"Unknown document type", docTypeName);
    cdr::String schemaString = rs.getString(1);

    // Parse the schema
    cdr::dom::Parser parser;
    parser.parse(schemaString);
    cdr::xsd::Schema schema(parser.getDocument().getDocumentElement());

    // Use the schema to validate the XML portion of the document
    cdr::xsd::Element schemaElement = schema.getTopElement();
    if (schemaElement.getName() != docXmlElement.getNodeName()) {
        cdr::String err;
        err = cdr::String(L"Wrong element at top of document XML: ")
            + L" (expected "
            + schemaElement.getName()
            + L")";
        throw cdr::Exception(err.c_str());
    }
    const cdr::xsd::Type& elementType = *schemaElement.getType(schema);
    validateElement(docXmlElement, elementType, schema, errors);
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
            response += L"    <Err>" + *i++ + L"</Err>\n";
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
    std::string query = "UPDATE document"
                        "   SET val_status = ?,"
                        "       val_date = GETDATE()"
                        " WHERE id = ?";
    cdr::db::PreparedStatement update = conn.prepareStatement(query);
    update.setString(1, status);
    update.setInt(2, id); 
    update.executeQuery();
}

/**
 * Verify that the required attributes are present. and that no
 * attributes are present which are undefined for this element
 * type or which do not meet the type requirements for the
 * attribute.
 *
 *  @return     <code>true</code> if element is denormalized.
 */
bool validateAttributes(
        cdr::dom::Element&              element, 
        const cdr::xsd::ComplexType&    type,
        cdr::xsd::Schema&               schema,
        cdr::StringList&                errors) 
{
    // Look for attributes expected for this element type.
    bool isDenormalized = false;
    cdr::xsd::AttrEnum attrEnum = type.getAttributes();
    while (attrEnum != type.getAttrEnd()) {
        cdr::xsd::Attribute* attr = *attrEnum++;
        cdr::String attrName      = attr->getName();
        cdr::String attrVal       = element.getAttribute(attrName.c_str());
        if (attrName == L"cdr::link")
            isDenormalized = true;
        if (attrVal.empty()) {
            if (!attr->isOptional()) {
                cdr::String err = cdr::String(L"Missing required attribute ")
                                + attr->getName()
                                + L" in element "
                                + cdr::String(element.getNodeName());
                errors.push_back(err);
            }
        }
        else {

            // Make sure the attribute has been assigned a type.
            const cdr::xsd::Type* attrType = attr->getType(schema);
            if (!attrType) {
                cdr::String err = cdr::String(L"Attribute ")
                                + attrName
                                + L" has not been assigned a type";
                errors.push_back(err);
                continue;
            }

            // Make sure the type is a simple type.
            const cdr::xsd::SimpleType* simpleType =
                dynamic_cast<const cdr::xsd::SimpleType*>(attrType);
            if (!simpleType) {
                cdr::String err = cdr::String(L"Attribute ")
                                + attrName
                                + L" does not have a simple type";
                errors.push_back(err);
            }
            else
                validateSimpleType(element.getNodeName(),
                                   attr->getName(),
                                   attrVal,
                                   *simpleType,
                                   errors);
        }
    }

    // Check for attributes which weren't expected for this element type.
    cdr::dom::NamedNodeMap attrs = element.getAttributes();
    int nAttrs = attrs.getLength();
    for (int i = 0; i < nAttrs; ++i) {
        cdr::dom::Node  attr = attrs.item(i);
        cdr::String     name = attr.getNodeName();
        if (!type.hasAttribute(name)) {
            cdr::String err = cdr::String(L"Unexpected attribute ")
                            + name
                            + L"='"
                            + cdr::String(attr.getNodeValue())
                            + L"' in element "
                            + cdr::String(element.getNodeName());
            errors.push_back(err);
        }
    }
    return isDenormalized;
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
        validateSimpleType(docElement.getNodeName(), 
                           L"",
                           cdr::dom::getTextContent(docElement),
                           *simpleType,
                           errors);
    else {
        bool denormalized = validateAttributes(docElement, 
                                               *complexType, 
                                               schema,
                                               errors);
        if (denormalized)
            return;
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
    bool result = re.match(value);
    return result;
}

/**
 * Builds string to identify element (and possibly attribute) containing
 * text value, suitable for concatenating to error message.
 */
cdr::String identifyTextValue(
        const cdr::String&              elemName, 
        const cdr::String&              attrName)
{
    if (attrName.empty())
        return cdr::String(L"' in element ") + elemName;
    else
        return cdr::String(L"' in attribute ") + attrName
                                              + L" of element "
                                              + elemName;
}

/**
 * Reports any errors found with a date value.
 */
void validateDate(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^\\d\\d\\d\\d-\\d\\d-\\d\\d$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid date value: '")
                        + val
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Reports any errors found with a time value.
 */
void validateTime(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] =
        L"^\\d\\d:\\d\\d((:\\d\\d)(\\.\\d+)?)?( ?[-+]\\d{1,2}:\\d{2})?$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid time value: '")
                        + val
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Reports any errors found with a decimal value.
 */
void validateDecimal(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^[-+]?\\d+(\\.\\d*)?|\\d*\\.\\d+$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid decimal value: '")
                        + val
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
    int precision = t.getPrecision();
    int scale     = t.getScale();
    if (precision != -1 || scale != -1) {
        int valPrecision       = 0;
        int valScale           = 0;

        // Skip past leading sign.
        size_t i = 0;
        if (i < val.size() && (val[i] == L'-' || val[i] == L'+'))
            ++i;

        // Skip past leading zeroes.
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
            cdr::String err = cdr::String(L"Invalid scale: '")
                            + val
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }

        // Check the precision.
        if (precision != -1 && valPrecision > precision) {
            cdr::String err = cdr::String(L"Invalid precision: '")
                            + val
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with an integer value.
 */
void validateInteger(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    static const wchar_t pattern[] = L"^[-+]?\\d+$";
    if (!matchPattern(pattern, val)) {
        cdr::String err = cdr::String(L"Invalid integer value: '")
                        + val
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }

    int precision = t.getPrecision();
    if (precision != -1) {

        int valPrecision       = 0;

        // Skip past leading sign.
        size_t i = 0;
        if (i < val.size() && (val[i] == L'-' || val[i] == L'+'))
            ++i;

        // Skip past leading zeroes.
        while (i < val.size() && val[i] == L'0')
            ++i;

        // You get at least 1 digit of precision even if the value is zero.
        if (i == val.size())
            valPrecision = 1;
        else
            valPrecision = val.size() - i;

        // Check the precision.
        if (valPrecision > precision) {
            cdr::String err = cdr::String(L"Invalid precision: '")
                            + val
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with a URI value.
 *
 * @see section 4 of RFC 2396.
 */
void validateUri(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              val, 
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{ 
    /*
     * The pattern is from RFC 2396, with parentheses only needed for 
     * extracting subexpressions removed.
     */
    const wchar_t* rfcPattern = 
        L"^([^:/?#]+:)?(//[^/?#]*)?[^?#]*(\?[^#]*)?(#.*)?$";

    /*
     * This is for a second pass to ensure that we stay within the character
     * set prescribed by the RFC.  These two patterns won't catch every
     * syntax error in URIs, but the BNF for the full grammar in RFC 2396
     * is beyond the reach of simple pattern-matching.
     */
    const wchar_t* charSetPattern = L"$[-A-Za-z0-9%_.!~*'()/;?:@&=$+,#]*$";
    if (!matchPattern(rfcPattern, val) || !matchPattern(charSetPattern, val)) {
        cdr::String err = cdr::String(L"Invalid URI value: '")
                        +  val
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Reports any errors found with a binary value.
 */
void validateBinary(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
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
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }
    }
    else {
        if (!matchPattern(base64Pattern, val)) {
            cdr::String err = cdr::String(L"Invalid base-64 encoding: '")
                            + val
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }
    }
}

/**
 * Reports any errors found with a date/time value.
 */
void validateTimeInstant(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
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
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Determines whether the value is less than any minimum specified.
 */
void checkMinInclusive(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{
    if (t.getMinInclusive().size() > 0) {
        switch (t.getBuiltinType()) {
        case cdr::xsd::SimpleType::DECIMAL:
            if (value.getFloat() >= t.getMinInclusive().getFloat())
                return;
            break;
        case cdr::xsd::SimpleType::INTEGER:
            if (value.getInt() >= t.getMinInclusive().getInt())
                return;
            break;
        default:
            if (value >= t.getMinInclusive())
                return;
            break;
        }
        cdr::String err = cdr::String(L"Value below minimum of ")
                        + t.getMinInclusive()
                        + L": '"
                        + value
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Determines whether the value is greater than any maximum specified.
 */
void checkMaxInclusive(
        const cdr::String&              elemName, 
        const cdr::String&              attrName, 
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     t,
        cdr::StringList&                errors) 
{
    if (t.getMaxInclusive().size() > 0) {
        switch (t.getBuiltinType()) {
        case cdr::xsd::SimpleType::DECIMAL:
            if (value.getFloat() <= t.getMaxInclusive().getFloat())
                return;
            break;
        case cdr::xsd::SimpleType::INTEGER:
            if (value.getInt() <= t.getMaxInclusive().getInt())
                return;
            break;
        default:
            if (value <= t.getMaxInclusive())
                return;
            break;
        }
        cdr::String err = cdr::String(L"Value below maximum of ")
                        + t.getMaxInclusive()
                        + L": '"
                        + value
                        + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }
}

/**
 * Checks the value of a text value against its simple type requirements.
 */
void validateSimpleType(
        const cdr::String&              elemName,
        const cdr::String&              attrName,
        const cdr::String&              value,
        const cdr::xsd::SimpleType&     simpleType,
        cdr::StringList&                errors)
{
    cdr::String err;
    cdr::xsd::SimpleType::BuiltinType builtinType =
        simpleType.getBuiltinType();
    switch (simpleType.getBuiltinType()) {
    case cdr::xsd::SimpleType::STRING:
        // Nothing special to do for plain string.
        break;
    case cdr::xsd::SimpleType::DATE:
        validateDate(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::TIME:
        validateTime(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::DECIMAL:
        validateDecimal(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::INTEGER:
        validateInteger(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::URI:
        validateUri(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::BINARY:
        validateBinary(elemName, attrName, value, simpleType, errors);
        break;
    case cdr::xsd::SimpleType::TIME_INSTANT:
        validateTimeInstant(elemName, attrName, value, simpleType, errors);
        break;
    default:
        err = cdr::String(L"Unrecognized base type for '")
            + value
            + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }

    // Check the value length.
    int valueLen = value.size();
    int typeLen  = simpleType.getLength();
    int minLen   = simpleType.getMinLength();
    int maxLen   = simpleType.getMaxLength();
    if (typeLen != -1 && valueLen != typeLen
    ||  minLen  != -1 && valueLen <  minLen
    ||  maxLen  != -1 && valueLen >  maxLen) {
        err = cdr::String(L"Invalid length: '")
            + value
            + identifyTextValue(elemName, attrName);
        errors.push_back(err);
    }

    // Check pattern constraints.
    const cdr::StringVector& patterns = simpleType.getPatterns();
    if (patterns.size() > 0) {
        bool foundMatch = false;
        for (size_t i = 0; !foundMatch && i < patterns.size(); ++i) {
            cdr::String pattern = cdr::String(L"^")
                                + patterns[i]
                                + L"$";
            foundMatch = matchPattern(pattern.c_str(), value);
        }
        if (!foundMatch) {
            err = cdr::String(L"Pattern constraints not matched for '")
                            + value
                            + identifyTextValue(elemName, attrName);
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
            cdr::String err = cdr::String(L"Invalid value: '")
                            + value
                            + identifyTextValue(elemName, attrName);
            errors.push_back(err);
        }
    }

    // Check for upper and lower bounds on the value space.
    checkMinInclusive(elemName, attrName, value, simpleType, errors);
    checkMaxInclusive(elemName, attrName, value, simpleType, errors);
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
    size_t pos = value.find_first_not_of(L" \t\r\n");
    if (pos != value.npos)
        errors.push_back(cdr::String(L"No text content allowed for element ")
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

        // Loop to end of occurrences which match.
        for (; n != 0; n = n.getNextSibling()) {
            
            // Skip nodes which aren't elements.
            if (n.getNodeType() != cdr::dom::Node::ELEMENT_NODE)
                continue;

            // See if this occurrence matches what we're looking for.
            occName = n.getNodeName();
            if (occName != e->getName())
                break;

            // Check for too many occurrences.
            if (++nOccs > e->getMaxOccs()) {
                cdr::String err = cdr::String(L"Too many occurrences "
                                  L"of element ")
                                + e->getName()
                                + L" within element "
                                + parentName;
                errors.push_back(err);
            }

            // Recursively check the element.
            validateElement(static_cast<cdr::dom::Element&>(n), 
                            *e->getType(schema), schema, errors);
        }

        // Check for too few occurrences.
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
