/*
 * $Id: CdrXsd.cpp,v 1.5 2000-04-26 01:29:23 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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
        if (name != oldName) {
            std::wstring err = std::wstring(L"Redefinition of element ")
                             + name
                             + L" from type "
                             + oldName
                             + L" to "
                             + name;
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
const cdr::xsd::Type* cdr::xsd::Node::resolveType(const cdr::xsd::Schema& schema)
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
    if (!base)
        throw cdr::Exception(L"Unknown base class", baseName);
    if (getName().size() == 0)
        throw cdr::Exception(L"Name missing for simple type");

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
                schema.registerElement(elementName, e->getTypeName());
                if (!hasElement(elementName))
                    elemNames.insert(elementName);
                elemList.push_back(e);
            }
            else if (nodeName == cdr::xsd::ATTRIBUTE)
                attrList.push_back(new cdr::xsd::Attribute(childNode));
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
