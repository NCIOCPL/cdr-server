/*
 * $Id: CdrXsd.h,v 1.1 2000-04-11 14:18:50 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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

namespace cdr {

    namespace xsd {

        // Schema constants
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

        // Forward declarations
        class Type;
        class Element;
        class Attribute;

        /**
         * An object of type <code>Schema</code> represents the types
         * and structure for an XML document type.
         */
        class Schema {
        public:
            Schema(const cdr::dom::Node&);
            ~Schema();
            const Type*         lookupType(const cdr::String&) const;
            Element&            getTopElement() const { return *topElement; }
        private:
            typedef std::map<cdr::String, const Type*> TypeMap;
            TypeMap             types;
            Element*            topElement;
            void                seedBuiltinTypes();
            void                registerType(const cdr::xsd::Type*);
        };

        // Aliases for container types.
        typedef std::list<Element*>             ElementList;
        typedef std::list<Attribute*>           AttributeList;
        typedef ElementList::const_iterator     ElemEnum;
        typedef AttributeList::const_iterator   AttrEnum;

        /**
         * Base class for schema elements and attributes.
         */
        class Node {
        public:
            Node() : type(0) {}
            cdr::String     getName() const { return name; }
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
        };

        /**
         * Base class for schema types, both simple and comples.  Uses
         * a dummy virtual function to make the runtime type information
         * work.
         */
        class Type {
        public:
            Type() {}
            cdr::String     getName() const { return name; }
        protected:
            void            setName(const cdr::String& n) { name = n; }
        private:
            cdr::String     name;
            virtual void    rttiDummy() const {}
        };

        /**
         * Elemental schema type.
         */
        class SimpleType : public Type {
        public:
            SimpleType(const cdr::dom::Node&);
            SimpleType(const cdr::String& n) { setName(n); }
            enum Encoding { HEX, BASE64 };
        private:
            SimpleType*         base;
            cdr::String         baseName;
            int                 minLength;
            int                 maxLength;
            int                 length;
            int                 precision;
            int                 scale;
            Encoding            encoding;
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
            ComplexType(const cdr::dom::Node&);
            ~ComplexType();
            enum ContentType { MIXED, ELEMENT_ONLY, TEXT_ONLY, EMPTY };
            ContentType     getContentType() const { return contentType; }
            ElemEnum        getElements() const { return elementList.begin(); }
            AttrEnum        getAttributes() const { return attributeList.begin(); }
            ElemEnum        getElemEnd() const { return elementList.end(); }
            AttrEnum        getAttrEnd() const { return attributeList.end(); }
            int             getElemCount() const { return elementList.size(); }
            int             getAttrCount() const { return attributeList.size(); }
        private:
            ElementList     elementList;
            AttributeList   attributeList;
            ContentType     contentType;
        };
    }
}

#endif
