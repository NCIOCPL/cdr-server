/*
 * $Id: CdrXsd.cpp,v 1.9 2000-10-05 21:27:22 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2000/10/04 18:36:39  bkline
 * Fixed bug in type name lookup.
 *
 * Revision 1.7  2000/05/03 21:57:57  bkline
 * Fixed overlong line length.
 *
 * Revision 1.6  2000/05/03 15:22:08  bkline
 * Added lookup hash for attributes.
 *
 * Revision 1.5  2000/04/26 01:29:23  bkline
 * Added more lookup support, for finding a element's type when the element
 * is encountered in an unexpected place (so we can validate it anyway) and
 * for determining which elements are allowed in a MIXED content element.
 *
 * Revision 1.4  2000/04/16 22:45:15  bkline
 * Added #pragma to disable annoying warnings about truncated debugging
 * information.
 *
 * Revision 1.3  2000/04/12 14:25:34  bkline
 * Removed debugging output.
 *
 * Revision 1.2  2000/04/11 21:22:58  bkline
 * Fleshed out support for simple types; added true type inheritance.
 *
 * Revision 1.1  2000/04/11 14:15:56  bkline
 * Initial revision
 *
 */

// Eliminate annoying warnings about truncated debugging information.
#pragma warning(disable : 4786)

// System headers.
#include <climits>
#include <iostream>

// Project headers.
#include "CdrXsd.h"
#include "CdrRegEx.h"

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
 * Extracts document constraint information from the schema document.
 */
cdr::xsd::Schema::Schema(const cdr::dom::Node& schemaElement)
{
    topElement = 0;
    seedBuiltinTypes();
    cdr::String nodeName = schemaElement.getNodeName();
    if (nodeName != cdr::xsd::SCHEMA)
        throw cdr::Exception(L"Top-level element must be xsd:schema");
    cdr::dom::Node childNode = schemaElement.getFirstChild();
    while (childNode != 0) {
        if (childNode.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
            nodeName = childNode.getNodeName();
            if (nodeName == cdr::xsd::ELEMENT) {
                if (topElement)
                    throw cdr::Exception(
                        L"Only one top-level element allowed in schema");
                topElement = new cdr::xsd::Element(childNode);
            }
            else if (nodeName == cdr::xsd::COMPLEX_TYPE)
                registerType(new cdr::xsd::ComplexType(*this, childNode));
            else if (nodeName == cdr::xsd::SIMPLE_TYPE)
                registerType(new cdr::xsd::SimpleType(*this, childNode));
        }
        childNode = childNode.getNextSibling();
    }
}

/**
 * Initializes the type collection with the builtin simple types.
 * XXX This is a stub.  Needs real knowledge of what the semantics
 * are for these types.
 */
void cdr::xsd::Schema::seedBuiltinTypes()
{
    registerType(new cdr::xsd::SimpleType(cdr::xsd::STRING));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::DATE));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::TIME));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::DECIMAL));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::INTEGER));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::BINARY));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::URI));
    registerType(new cdr::xsd::SimpleType(cdr::xsd::TIME_INSTANT));
}

/**
 * Adds a new type to the map of types for the schema, first checking
 * for duplicate type names.
 */
void cdr::xsd::Schema::registerType(const cdr::xsd::Type* type)
{
    cdr::String name = type->getName();
    if (types.find(name) != types.end())
        throw cdr::Exception(L"Duplicate type name", name);
    types[name] = type;
}

/**
 * Remembers an element definition, so we can look up its type later (in case
 * we end up validating a record with misplaced elements).
 */
void cdr::xsd::Schema::registerElement(const cdr::String& name,
                                       const cdr::String& type)
{
    ElementTypeMap::const_iterator i = elements.find(name);
    if (i == elements.end())
        elements[name] = type;
    else {
        const cdr::String& oldName = i->second;
        if (type != oldName) {
            std::wstring err = std::wstring(L"Redefinition of element ")
                             + name
                             + L" from type "
                             + oldName
                             + L" to "
                             + type;
            throw cdr::Exception(err.c_str());
        }
    }
}

/**
 * Finds a type by name in the schema's type collection.
 */
const cdr::xsd::Type* cdr::xsd::Schema::lookupType(const cdr::String& n) const
{
    TypeMap::const_iterator i = types.find(n);
    if (i != types.end())
        return i->second;
    return 0;
}

/**
 * Finds the type name for a specific element name.
 */
cdr::String cdr::xsd::Schema::lookupElementType(const cdr::String& name) const
{
    ElementTypeMap::const_iterator i = elements.find(name);
    return i != elements.end() ? i->second : L"";
}

/**
 * Cleans up contents of schema type collection and drops top element.
 */
cdr::xsd::Schema::~Schema()
{
    for (TypeMap::iterator i = types.begin(); i != types.end(); ++i)
        delete i->second;
    delete topElement;
}

/**
 * Finds this node's type in the collection of types.  Caches the answer
 * so it doesn't have to do the search more than once.
 */
const cdr::xsd::Type* 
cdr::xsd::Node::resolveType(const cdr::xsd::Schema& schema)
{
    if (!type)
        type = schema.lookupType(typeName);
    return type;
}

/**
 * Builds a new schema element object from its XML node in the schema
 * document.
 */
cdr::xsd::Element::Element(const cdr::dom::Node& dn)
{
    maxOccs = minOccs = 1;
    cdr::dom::NamedNodeMap attrs = dn.getAttributes();
    int nAttrs = attrs.getLength();
    for (int i = 0; i < nAttrs; ++i) {
        cdr::dom::Node attr = attrs.item(i);
        cdr::String attrName = attr.getNodeName();
        cdr::String attrValue = attr.getNodeValue();
        if (attrName == cdr::xsd::NAME)
            name = attrValue;
        else if (attrName == cdr::xsd::TYPE)
            typeName = attrValue;
        else if (attrName == cdr::xsd::MIN_OCCURS)
            minOccs = attrValue.getInt();
        else if (attrName == cdr::xsd::MAX_OCCURS) {
            if (attrValue == cdr::xsd::UNLIMITED)
                maxOccs = INT_MAX;
            else
                maxOccs = attrValue.getInt();
        }
    }
    if (name.size() == 0)
        throw cdr::Exception(L"Name missing for element");
    if (typeName.size() == 0)
        throw cdr::Exception(L"Type missing for element", name);
}

/**
 * Builds a new schema element object from the xsd:attribute node in the 
 * schema document.
 */
cdr::xsd::Attribute::Attribute(const cdr::dom::Node& dn)
{
    optional = false;
    cdr::dom::NamedNodeMap attrs = dn.getAttributes();
    int nAttrs = attrs.getLength();
    for (int i = 0; i < nAttrs; ++i) {
        cdr::dom::Node attr = attrs.item(i);
        cdr::String attrName = attr.getNodeName();
        cdr::String attrValue = attr.getNodeValue();
        if (attrName == cdr::xsd::NAME)
            name = attrValue;
        else if (attrName == cdr::xsd::TYPE)
            typeName = attrValue;
        else if (attrName == cdr::xsd::MIN_OCCURS)
            if (attrValue == L"0")
                optional = true;
    }
    if (name.size() == 0)
        throw cdr::Exception(L"Name missing for attribute");
    if (typeName.size() == 0)
        throw cdr::Exception(L"Type missing for attribute", name);
}

/**
 * Constructs a built-in simple type from "magic" knowledge of certain
 * fundamental ur-types.
 */
cdr::xsd::SimpleType::SimpleType(const cdr::String& n) 
{
    // Defaults.
    base        = 0;
    minLength   = 0;
    maxLength   = INT_MAX;
    length      = -1;
    precision   = -1;
    scale       = -1;
    encoding    = HEX;

    struct BuiltinTypeMapElement { 
        BuiltinTypeMapElement(const cdr::String& n, 
                              cdr::xsd::SimpleType::BuiltinType t) 
            : name(n), type(t) {}
        cdr::String name; 
        cdr::xsd::SimpleType::BuiltinType type; 
    };
    static BuiltinTypeMapElement map[] = {
        BuiltinTypeMapElement(cdr::xsd::STRING,         STRING       ),
        BuiltinTypeMapElement(cdr::xsd::DATE,           DATE         ),
        BuiltinTypeMapElement(cdr::xsd::TIME,           TIME         ),
        BuiltinTypeMapElement(cdr::xsd::DECIMAL,        DECIMAL      ),
        BuiltinTypeMapElement(cdr::xsd::INTEGER,        INTEGER      ),
        BuiltinTypeMapElement(cdr::xsd::BINARY,         BINARY       ),
        BuiltinTypeMapElement(cdr::xsd::URI,            URI          ),
        BuiltinTypeMapElement(cdr::xsd::TIME_INSTANT,   TIME_INSTANT )
    };

    for (size_t i = 0; i < sizeof map / sizeof *map; ++i) {
        if (n == map[i].name) {
            setName(n);
            builtinType = map[i].type;
            break;
        }
    }
    if (getName().size() == 0)
        throw cdr::Exception(L"Unrecognized builtin type", n);
}

/**
 * Builds a new schema element object from the xsd:simpleType node in the 
 * schema document.
 */
cdr::xsd::SimpleType::SimpleType(const cdr::xsd::Schema& schema,
                                 const cdr::dom::Node& dn)
{
    base = 0;
    cdr::dom::NamedNodeMap attrs = dn.getAttributes();
    int nAttrs = attrs.getLength();
    for (int i = 0; i < nAttrs; ++i) {
        cdr::dom::Node attr = attrs.item(i);
        cdr::String attrName = attr.getNodeName();
        cdr::String attrValue = attr.getNodeValue();
        if (attrName == cdr::xsd::NAME)
            setName(attrValue);
        else if (attrName == cdr::xsd::BASE) {
            baseName = attrValue;
            base = dynamic_cast<const cdr::xsd::SimpleType*>
                (schema.lookupType(baseName));
        }
    }
    if (getName().size() == 0)
        throw cdr::Exception(L"Name missing for simple type");
    if (baseName.empty())
        throw cdr::Exception(L"Base type not specified for simple type",
                getName());
    if (!base)
        throw cdr::Exception(L"Unknown base class", baseName);

    minLength   = base->minLength;
    maxLength   = base->maxLength;
    length      = base->length;
    precision   = base->precision;
    scale       = base->scale;
    encoding    = base->encoding;
    builtinType = base->builtinType;

    cdr::dom::Node childNode = dn.getFirstChild();
    while (childNode != 0) {
        int nodeType = childNode.getNodeType();
        if (nodeType == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String nodeName = childNode.getNodeName();
            cdr::dom::Element& e = static_cast<cdr::dom::Element&>(childNode);
            cdr::String value = e.getAttribute(cdr::xsd::VALUE);
            if (nodeName == cdr::xsd::ENUMERATION)
                enumSet.insert(value);
            else if (nodeName == cdr::xsd::MIN_INCLUSIVE)
                minInclusive = value;
            else if (nodeName == cdr::xsd::MAX_INCLUSIVE)
                maxInclusive = value;
            else if (nodeName == cdr::xsd::MIN_LENGTH)
                minLength = value.getInt();
            else if (nodeName == cdr::xsd::MAX_LENGTH)
                maxLength = value.getInt();
            else if (nodeName == cdr::xsd::LENGTH)
                length = value.getInt();
            else if (nodeName == cdr::xsd::PATTERN)
                patterns.push_back(value);
            else if (nodeName == cdr::xsd::PRECISION)
                precision = value.getInt();
            else if (nodeName == cdr::xsd::SCALE)
                scale = value.getInt();
            else if (nodeName == cdr::xsd::ENCODING) {
                if (value == cdr::xsd::HEX)
                    encoding = HEX;
                else if (value == cdr::xsd::BASE64)
                    encoding = BASE64;
                else
                    throw cdr::Exception(L"Illegal encoding value", value);
            }
        }
        childNode = childNode.getNextSibling();
    }
}

/**
 * Builds a new schema element object from the xsd:complexType node in the 
 * schema document.
 */
cdr::xsd::ComplexType::ComplexType(cdr::xsd::Schema& schema,
                                   const cdr::dom::Node& dn)
{
    contentType = ELEMENT_ONLY;
    cdr::dom::NamedNodeMap attrs = dn.getAttributes();
    int nAttrs = attrs.getLength();
    for (int i = 0; i < nAttrs; ++i) {
        cdr::dom::Node attr = attrs.item(i);
        cdr::String attrName = attr.getNodeName();
        cdr::String attrValue = attr.getNodeValue();
        if (attrName == cdr::xsd::NAME)
            setName(attrValue);
        else if (attrName == cdr::xsd::CONTENT) {
            if (attrValue == cdr::xsd::TEXT_ONLY)
                contentType = TEXT_ONLY;
            else if (attrValue == cdr::xsd::EMPTY)
                contentType = EMPTY;
            else if (attrValue == cdr::xsd::MIXED)
                contentType = MIXED;
        }
    }
    cdr::dom::Node childNode = dn.getFirstChild();
    while (childNode != 0) {
        int nodeType = childNode.getNodeType();
        if (nodeType == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String nodeName = childNode.getNodeName();
            if (nodeName == cdr::xsd::ELEMENT) {
                cdr::xsd::Element* e = new cdr::xsd::Element(childNode);
                cdr::String elementName = e->getName();
                cdr::String elementTypeName = e->getTypeName();
                schema.registerElement(elementName, elementTypeName);
                if (!hasElement(elementName))
                    elemNames.insert(elementName);
                elemList.push_back(e);
            }
            else if (nodeName == cdr::xsd::ATTRIBUTE) {
                cdr::xsd::Attribute* a = new cdr::xsd::Attribute(childNode);
                cdr::String attrName = a->getName();
                if (!hasAttribute(attrName))
                    attrNames.insert(attrName);
                attrList.push_back(a);
            }
        }
        childNode = childNode.getNextSibling();
    }
    if (getName().size() == 0)
        throw cdr::Exception(L"Name missing for complex type");
}

#if 0
/**
 * Finds element, given its name.  Used for MIXED content type.
 */
cdr::xsd::Element* 
cdr::xsd::ComplexType::getElement(const cdr::String& name) const
{
    ElementMap::iterator i = elemMap.find(name);
    return i == elemMap.end() ? 0 : i->second;
}
#endif

/**
 * Cleans up the lists of elements and attributes which make up this type.
 */
cdr::xsd::ComplexType::~ComplexType()
{
    for (cdr::xsd::ElementList::iterator eli = elemList.begin(); 
         eli != elemList.end(); 
         ++eli)
        delete *eli;
    for (cdr::xsd::AttributeList::iterator ali = attrList.begin(); 
         ali != attrList.end(); 
         ++ali)
        delete *ali;
}

/**
 * Checks document against the schema for its document type, reporting any
 * errors found in the caller's <code>Errors</code> vector.  This method
 * is a lower-level call invoked by the overloaded version which extracts
 * the schema from the database.
 *
 *  @param  docElem             top level element for CdrDoc
 *  @param  schemaElem          top level element for schema
 *  @param  errors              vector of strings to be populated by this
 *                              function
 */
void cdr::xsd::validateDocAgainstSchema(
        cdr::dom::Element&         docElem,
        cdr::dom::Element&         schemaElem,
        cdr::StringList&           errors)
{
    // Parse the schema
    cdr::xsd::Schema schema(schemaElem);

    // Use the schema to validate the XML portion of the document
    cdr::xsd::Element schemaElement = schema.getTopElement();
    if (schemaElement.getName() != docElem.getNodeName()) {
        cdr::String err;
        err = cdr::String(L"Wrong element at top of document XML: ")
            + L" (expected "
            + schemaElement.getName()
            + L")";
        throw cdr::Exception(err.c_str());
    }
    const cdr::xsd::Type& elementType = *schemaElement.getType(schema);
    validateElement(docElem, elementType, schema, errors);
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
    cdr::String elemName = element.getNodeName();
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
                                + elemName;
                errors.push_back(err);
            }
        }
        else {

            // Make sure the attribute has been assigned a type.
            const cdr::xsd::Type* attrType = attr->getType(schema);
            if (!attrType) {
                cdr::String err = cdr::String(L"Attribute ")
                                + attrName
                                + L" in element "
                                + elemName
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
                                + L" in element "
                                + elemName
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
                            + elemName;
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
        else {
            const cdr::xsd::Type* type = schema.lookupType(typeName);
            if (!type) {
                cdr::String err = cdr::String(L"Unable to find type ")
                                + typeName
                                + L" for element "
                                + occName;
                errors.push_back(err);
            }
            else
                validateElement(static_cast<cdr::dom::Element&>(n),
                                *type, schema, errors);
        }
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
        if (childType)
            validateElement(static_cast<cdr::dom::Element&>(child),
                            *childType, schema, errors);
    }
}
