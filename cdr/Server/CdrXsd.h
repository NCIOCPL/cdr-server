/*
 * $Id: CdrXsd.h,v 1.6 2000-05-03 15:43:36 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2000/04/26 01:38:13  bkline
 * Added more lookup support for determining an arbitrary element's type
 * and for determining which elements are allowed in a MIXED content
 * element.
 *
 * Revision 1.4  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.3  2000/04/22 18:01:26  bkline
 * Fleshed out documentation comments.
 *
 * Revision 1.2  2000/04/11 21:23:40  bkline
 * Fleshed out support for simple types; added true type inheritance.
 *
 * Revision 1.1  2000/04/11 14:18:50  bkline
 * Initial revision
 */

#ifndef CDR_XSD_H_
#define CDR_XSD_H_

// System headers
#include <map>
#include <list>

// Project headers
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDom.h"

/**@#-*/

namespace cdr {
    namespace xsd {

/**@#+*/

    /** @pkg cdr.xsd */

        /**
         * Schema constants, used to recognize tokens found by the parser.
         */
        const wchar_t* const SCHEMA        = L"xsd:schema";
        const wchar_t* const ELEMENT       = L"xsd:element";
        const wchar_t* const ATTRIBUTE     = L"xsd:attribute";
        const wchar_t* const COMPLEX_TYPE  = L"xsd:complexType";
        const wchar_t* const SIMPLE_TYPE   = L"xsd:simpleType";
        const wchar_t* const MIN_LENGTH    = L"xsd:minLength";
        const wchar_t* const MAX_LENGTH    = L"xsd:maxLength";
        const wchar_t* const PATTERN       = L"xsd:pattern";
        const wchar_t* const ENUMERATION   = L"xsd:enumeration";
        const wchar_t* const STRING        = L"xsd:string";
        const wchar_t* const DATE          = L"xsd:date";
        const wchar_t* const TIME          = L"xsd:time";
        const wchar_t* const DECIMAL       = L"xsd:decimal";
        const wchar_t* const INTEGER       = L"xsd:integer";
        const wchar_t* const BINARY        = L"xsd:binary";
        const wchar_t* const URI           = L"xsd:uri";
        const wchar_t* const TIME_INSTANT  = L"xsd:timeInstant";
        const wchar_t* const MIN_INCLUSIVE = L"xsd:minInclusive";
        const wchar_t* const MAX_INCLUSIVE = L"xsd:maxInclusive";
        const wchar_t* const LENGTH        = L"xsd:length";
        const wchar_t* const PRECISION     = L"xsd:precision";
        const wchar_t* const SCALE         = L"xsd:scale";
        const wchar_t* const ENCODING      = L"xsd:encoding";
        const wchar_t* const NAME          = L"name";
        const wchar_t* const TYPE          = L"type";
        const wchar_t* const VALUE         = L"value";
        const wchar_t* const BASE          = L"base";
        const wchar_t* const MIN_OCCURS    = L"minOccurs";
        const wchar_t* const MAX_OCCURS    = L"maxOccurs";
        const wchar_t* const CONTENT       = L"content";
        const wchar_t* const TEXT_ONLY     = L"textOnly";
        const wchar_t* const ELEMENT_ONLY  = L"elementOnly";
        const wchar_t* const EMPTY         = L"empty";
        const wchar_t* const MIXED         = L"mixed";
        const wchar_t* const UNLIMITED     = L"*";
        const wchar_t* const HEX           = L"hex";
        const wchar_t* const BASE64        = L"base64";

        // Forward declarations.
        class Type;
        class Element;
        class Attribute;

        /**
         * Aliases for container types.  Provided for convenience (and as a
         * workaround for MSVC++ bugs).
         */
        typedef std::list<Element*>             ElementList;
        typedef std::list<Attribute*>           AttributeList;
        typedef ElementList::const_iterator     ElemEnum;
        typedef AttributeList::const_iterator   AttrEnum;
        typedef std::map<cdr::String, Element*> ElementMap;
        typedef std::map<cdr::String, 
                         cdr::String>           ElementTypeMap;

        /**
         * An object of type <code>Schema</code> represents the types
         * and structure for an XML document type.
         */
        class Schema {
        public:
            Schema(const cdr::dom::Node&);
            ~Schema();
            const Type*         lookupType(const cdr::String&) const;
            cdr::String         lookupElementType(const cdr::String&) const;
            Element&            getTopElement() const { return *topElement; }
            void                registerElement(const cdr::String&,
                                                const cdr::String&);
        private:
            typedef std::map<cdr::String, const Type*> TypeMap;
            TypeMap             types;
            ElementTypeMap      elements;
            Element*            topElement;
            void                seedBuiltinTypes();
            void                registerType(const cdr::xsd::Type*);
        };

        /**
         * Base class for schema elements and attributes.
         */
        class Node {
        public:
            Node() : type(0) {}
            virtual ~Node() {}
            cdr::String     getName() const { return name; }
            cdr::String     getTypeName() const { return typeName; }
            const Type*     getType(const Schema& s) { 
                return resolveType(s); 
            }
        protected:
            cdr::String     name;
            cdr::String     typeName;
        private:
            const Type*     type;
            const Type*     resolveType(const Schema&);
        };

        /**
         * Schema element.  Can be top-level element for the schema's
         * document type, or a member of the list of elements for a 
         * complex type.
         */
        class Element : public Node {
        public:
            Element(const cdr::dom::Node&);
            int             getMaxOccs() const { return maxOccs; }
            int             getMinOccs() const { return minOccs; }
        private:
            int             maxOccs;
            int             minOccs;
        };

        /**
         * Attribute attached to a schema element.
         */
        class Attribute : public Node {
        public:
            Attribute(const cdr::dom::Node&);
            bool                isOptional() const { return optional; }
        private:
            bool            optional;
        };

        /**
         * Base class for schema types, both simple and comples.
         */
        class Type {
        public:
            Type() {}
            virtual ~Type() {}
            cdr::String     getName() const { return name; }
        protected:
            void            setName(const cdr::String& n) { name = n; }
        private:
            cdr::String     name;
        };

        /**
         * Elemental schema type.
         */
        class SimpleType : public Type {
        public:
            SimpleType(const Schema&, const cdr::dom::Node&);
            SimpleType(const cdr::String& n);
            enum Encoding { HEX, BASE64 };
            enum BuiltinType { STRING, DATE, TIME, DECIMAL, INTEGER,
                               URI, BINARY, TIME_INSTANT };
            cdr::String         getBaseName() const { return baseName; }
            int                 getMinLength() const { return minLength; }
            int                 getMaxLength() const { return maxLength; }
            int                 getLength() const { return length; }
            int                 getPrecision() const { return precision; }
            int                 getScale() const { return scale; }
            Encoding            getEncoding() const { return encoding; }
            BuiltinType         getBuiltinType() const { return builtinType; }
            cdr::String         getMinInclusive() const { return minInclusive; }
            cdr::String         getMaxInclusive() const { return maxInclusive; }
            const
            cdr::StringSet&     getEnumSet() const { return enumSet; }
            const
            cdr::StringVector&  getPatterns() const { return patterns; }
        private:
            const SimpleType*   base;
            cdr::String         baseName;
            int                 minLength;
            int                 maxLength;
            int                 length;
            int                 precision;
            int                 scale;
            Encoding            encoding;
            BuiltinType         builtinType;
            cdr::String         minInclusive;
            cdr::String         maxInclusive;
            cdr::StringSet      enumSet;
            cdr::StringVector   patterns;
        };

        /**
         * Complex schema type.
         */
        class ComplexType : public Type {
        public:
            ComplexType(Schema&, const cdr::dom::Node&);
            ~ComplexType();
            enum ContentType { MIXED, ELEMENT_ONLY, TEXT_ONLY, EMPTY };
            ContentType     getContentType() const { return contentType; }
            ElemEnum        getElements() const { return elemList.begin(); }
            AttrEnum        getAttributes() const { return attrList.begin();}
            ElemEnum        getElemEnd() const { return elemList.end(); }
            AttrEnum        getAttrEnd() const { return attrList.end(); }
            int             getElemCount() const { return elemList.size(); }
            int             getAttrCount() const { return attrList.size(); }
            bool            hasElement(const cdr::String& name) const
                { return elemNames.find(name) != elemNames.end(); }
            bool            hasAttribute(const cdr::String& name) const
                { return attrNames.find(name) != attrNames.end(); }
        private:
            ElementList     elemList;
            AttributeList   attrList;
            cdr::StringSet  elemNames;
            cdr::StringSet  attrNames;
            ContentType     contentType;
        };
    }
}

#endif
