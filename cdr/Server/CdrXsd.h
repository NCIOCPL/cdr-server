/*
 * $Id: CdrXsd.h,v 1.10 2000-12-28 13:32:33 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.9  2000/10/05 21:23:55  bkline
 * Added validateDocAgainstSchema (lower level than the one tied to the
 * database and the server).
 *
 * Revision 1.8  2000/05/04 01:15:54  bkline
 * Fixed some unclosed comments.
 *
 * Revision 1.7  2000/05/03 21:46:35  bkline
 * More ccdoc comments.
 *
 * Revision 1.6  2000/05/03 15:43:36  bkline
 * Added hasAttribute() method.
 *
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
         * Tag for top-level element in validation Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const SCHEMA        = L"xsd:schema";

        /**
         * Tag for element-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ELEMENT       = L"xsd:element";

        /**
         * Tag for choice-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const CHOICE        = L"xsd:choice";

        /**
         * Tag for attribute-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ATTRIBUTE     = L"xsd:attribute";

        /**
         * Tag for Schema element specifying a user-defined complex type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const COMPLEX_TYPE  = L"xsd:complexType";

        /**
         * Tag for Schema element specifying a user-defined simple type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const SIMPLE_TYPE   = L"xsd:simpleType";
        
        /**
         * Tag for Schema element adding a constraing on the minimum length of
         * valid data values.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MIN_LENGTH    = L"xsd:minLength";
        
        /**
         * Tag for Schema element adding a constraing on the maximum length of
         * valid data values.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MAX_LENGTH    = L"xsd:maxLength";
        
        /**
         * Tag for Schema element adding a regular-expression pattern used
         * for specifying valid value sets.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const PATTERN       = L"xsd:pattern";
        
        /**
         * Tag for Schema element adding a specific value to a set of
         * enumerated valid values for a user-defined simple data type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const ENUMERATION   = L"xsd:enumeration";
        
        /**
         * Attribute name identifying base type for a derived user-defined
         * type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const BASE          = L"base";

        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in String type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const STRING        = L"xsd:string";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Date type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const DATE          = L"xsd:date";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Time type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const TIME          = L"xsd:time";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Decimal type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const DECIMAL       = L"xsd:decimal";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Integer type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const INTEGER       = L"xsd:integer";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Binary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const BINARY        = L"xsd:binary";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in URI type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const URI           = L"xsd:uri";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in TimeInstant type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const TIME_INSTANT  = L"xsd:timeInstant";

        /**
         * Tag for the element used to specify the inclusive lower bound of
         * the range for valid values for elements and attributes of this
         * type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MIN_INCLUSIVE = L"xsd:minInclusive";

        /**
         * Tag for the element used to specify the inclusive upper bound of
         * the range for valid values for elements and attributes of this
         * type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MAX_INCLUSIVE = L"xsd:maxInclusive";

        /**
         * Tag for the element used to specify the exact length in characters
         * (for string-based types) or bytes (for binary-derived types).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const LENGTH        = L"xsd:length";

        /**
         * Tag for the element used to specify the allowable number of
         * significant digits in a value of a type derived from the built-in 
         * Decimal or Integer types.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const PRECISION     = L"xsd:precision";

        /**
         * Tag for the element used to specify the allowable number of
         * significant digits following the decimal point in a value of a type
         * derived from the built-in Decimal type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const SCALE         = L"xsd:scale";

        /**
         * Tag for the element used to specify the encoding allowed (Hex or
         * Base64) for values of a type derived from the built-in Binary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const ENCODING      = L"xsd:encoding";

        /**
         * Name of the attribute which specifies the identifier associated
         * with a new element, attribute, or type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const NAME          = L"name";

        /**
         * Name of the attribute which specifies the simple or complex type
         * which elements or attributes must conform to in order to be valid.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const TYPE          = L"type";

        /**
         * Name of the attribute which specifies the value for a constraint
         * added to a user-defined simple type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const VALUE         = L"value";

        /**
         * Name of the attribute which specifies the minimum number of
         * occurrences required for this element.  The default value for this
         * attribute is '1'.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const MIN_OCCURS    = L"minOccurs";

        /**
         * Name of the attribute which specifies the maximum number of
         * occurrences allowed for this element.  The default value for this
         * attribute is '1'.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const MAX_OCCURS    = L"maxOccurs";

        /**
         * Name of the attribute which specifies the content model (Mixed,
         * ElementOnly, TextOnly, or Empty) for elements of this type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const CONTENT       = L"content";

        /**
         * Value of the <code>content</code> attribute indicating TextOnly
         * content (i.e., no sub-elements are permitted).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const TEXT_ONLY     = L"textOnly";

        /**
         * Value of the <code>content</code> attribute indicating ElementOnly
         * content (i.e., no non-blank text content is permitted as a direct
         * node of the element).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ELEMENT_ONLY  = L"elementOnly";

        /**
         * Value of the <code>content</code> attribute indicating Empty
         * content (i.e., neither sub-elements, nor non-blank text content is
         * permitted in elements of this type).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const EMPTY         = L"empty";

        /**
         * Value of the <code>content</code> attribute indicating Mixed
         * content (i.e., both text and element together).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const MIXED         = L"mixed";

        /**
         * Value of the <code>maxOccurs</code> attribute indicating that
         * unlimited occurrences of the element are permitted.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const UNLIMITED     = L"*";

        /**
         * Value of the <code>encoding</code> attribute specifying hex
         * encoding for a <code>binary</code> data type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const HEX           = L"hex";

        /**
         * Value of the <code>encoding</code> attribute specifying base64
         * encoding for a <code>binary</code> data type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const BASE64        = L"base64";

        /** 
         * Forward declarations.
         */
        class Type;
        class Elem;
        class Attr;
        class ElemChoice;

        /**
         * Aliases for container types.  Provided for convenience (and as a
         * workaround for MSVC++ bugs).
         */
        typedef std::list<ElemChoice*>          ElemSeq;
        typedef std::map<cdr::String, Elem*>    ElemSet;
        typedef std::map<cdr::String, Attr*>    AttrSet;
        typedef ElemSeq::const_iterator         ChoiceEnum;
        typedef ElemSet::const_iterator         ElemEnum;
        typedef AttrSet::const_iterator         AttrEnum;
        typedef std::map<cdr::String, 
                         cdr::String>           ElemTypeMap;

        /**
         * An object of type <code>Schema</code> represents the types
         * and structure for an XML document type.
         */
        class Schema {

        public:

            /**
             * Constructs a new <code>Schema</code> object from the top node
             * of the parsed DOM tree representing the validation constraints
             * to be applied to CDR documents of the type represented by this
             * schema.
             */
            Schema(const cdr::dom::Node& node);

            /**
             * Releases resources allocated for a schema validation object.
             */
            ~Schema();

            /**
             * Find a <code>Type</code> object from its name, using the
             * <code>ElemTypeMap</code> of names to types.
             *
             *  @param  name    name assigned to the type.
             *  @return         pointer to type object if found; otherwise
             *                  <code>NULL</code>.
             */
            const Type*         lookupType(const cdr::String& name) const;

            /**
             * Finds the name of the type used to validate a particular
             * element.
             *
             *  @param  e       string containing the name of the element.
             *  @return         string containing the name of the element's
             *                  type.
             */
            cdr::String         lookupElementType(const cdr::String& e) const;

            /**
             * Returns top-level element for the Schema's DOM tree.
             *
             *  @return         reference to schema's top element.
             */
            Elem&               getTopElement() const { return *topElement; }

            /**
             * Remembers the name of the type used for this element.
             *
             *  @param  eName   string containing the name of the element.
             *  @param  tName   string containing the name of the element's
             *                  type.
             */
            void                registerElement(const cdr::String& eName,
                                                const cdr::String& tName);

        private:

            /**
             * Map of type names to the corresponding <code>Type</code>
             * objects.
             */
            typedef std::map<cdr::String, const Type*> TypeMap;

            /**
             * Registry of types used in the Schema, indexed by type name.
             */
            TypeMap             types;

            /**
             * Map from element name to name of type used by the element.
             */
            ElemTypeMap         elements;

            /**
             * Element forming the top of the DOM tree for this schema.
             */
            Elem*               topElement;

            /**
             * Registers the built-in Schema types supported by this
             * implementation.
             */
            void                seedBuiltinTypes();

            /**
             * Plugs a new type in to the type map, which indexes known types
             * by name.
             */
            void                registerType(const cdr::xsd::Type*);
        };

        /**
         * Common base class for schema elements and attributes.
         */
        class Node {

        public:
            /**
             * Destructor is virtual so the correct derived class's destructor
             * will be invoked at cleanup time.
             */
            virtual ~Node() {}

            /**
             * Accessor method for <code>Node</code>'s name.
             *
             *  @return         string containing name of this element or
             *                  attribute.
             */
            cdr::String     getName() const { return name; }

            /**
             * Accessor method for <code>Node</code>'s type's name.
             *
             *  @return         string containing name of this element or
             *                  attribute's type.
             */
            cdr::String     getTypeName() const { return typeName; }

            /**
             * Find's the <code>Type</code> object attached to this element or
             * attribute.  Hands the work off to <code>resolveType()</code>,
             * which looks up the type based on its name if this is the first
             * call for this <code>Node</code>.
             *
             *  @param  s       reference to <code>Schema</code> object, which
             *                  has the registry of types used to find the
             *                  type by name.
             *  @return         address of <code>Type</code> object assigned
             *                  to this element or attribute.
             */
            const Type*     getType(const Schema& s) { 
                return resolveType(s); 
            }

        protected:

            /**
             * Creates an uninitialized <code>Node</code> object.  
             * Construction is completed by the derived class.  Objects
             * of the base class cannot be created.
             */
            Node() : type(0) {}

            /**
             * String containing the name of the element or attribute.
             */
            cdr::String     name;

            /**
             * String containing the name of the type assigned to the element
             * or attribute.
             */
            cdr::String     typeName;

        private:

            /**
             * Pointer to the <code>Type</code> object associated with this
             * element or attribute.  Used to cache the results of
             * <code>resolveType</code> so the lookup only has to be performed
             * once for this <code>Node</code>.
             */
            const Type*     type;

            /**
             * Finds this <code>Node</code>'s type in the registry maintained
             * by the schema.  It is necessary to delay type name resolution
             * because it is possible for a schema to refer to a type before
             * it has been defined.
             *
             *  @param  s           reference to <code>Schema</code> object
             *                      which maintains the registry of types used
             *                      to match a name to a <code>Type</code>.
             *  @return             address of <code>Type</code> object 
             *                      assigned to this element or attribute.
             */
            const Type*     resolveType(const Schema&);
        };

        /**
         * Schema element.  Can be top-level element for the schema's
         * document type, or a possible choice for one of the positions in the
         * sequence of elements for a complex type.
         */
        class Elem : public Node {

        public:

            /**
             * Extracts the specification of the requirements for this element
             * for its schema node.
             *
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this element.
             */
            Elem(const cdr::dom::Node& n);

            /**
             * Accessor method for maximum number of occurrences allowed for
             * this element.
             *
             *  @return             integer representing maximum number of
             *                      occurrences allowed for this element.
             */
            int             getMaxOccs() const { return maxOccs; }

            /**
             * Accessor method for minimum number of occurrences required for
             * this element.
             *
             *  @return             integer representing minimum number of
             *                      occurrences required for this element.
             */
            int             getMinOccs() const { return minOccs; }

        private:

            /**
             * Maximum number of occurrences allowed for this element.
             */
            int             maxOccs;

            /**
             * Minimum number of occurrences required for this element.
             */
            int             minOccs;
        };

        /**
         * Attribute attached to a schema element.
         */
        class Attr : public Node {

        public:

            /**
             * Extracts the specification of the requirements for this
             * attribute for its schema node.
             * 
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this attribute.
             */
            Attr(const cdr::dom::Node& n);

            /**
             * Accessor method for reporting whether this attribute can be
             * omitted.
             *
             *  @return             <code>true</code> if the attribute
             *                      can be omitted for this element.
             */
            bool            isOptional() const { return optional; }

        private:

            /**
             * Flag recording whether this element can be omitted.
             */
            bool            optional;
        };

        /**
         * Set of alternate elements, any one of which is acceptable in a
         * given context.
         */
        class ElemChoice {

        public:

            /**
             * Extracts the specification of the requirements for this choice
             * for its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this element choice.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this choice.
             */
            ElemChoice(Schema& s, const cdr::dom::Node& n);

            /**
             * Creates a special-case "choice" which contains only a single
             * element.
             *
             *  @param  e           address of only element available for
             *                      this "choice."
             */
            ElemChoice(Elem* e);

            /**
             * Cleans up the list of elements.
             */
            ~ElemChoice();

            /**
             * Looks up an element by name in this set of element choices.
             *
             *  @param  name        reference to string containing element's
             *                      name.
             *  @return             pointer to Elem object, if found;
             *                      otherwise <code>NULL</code>.
             */
            Elem*           find(const cdr::String& name) const;

            /**
             * Inserts a new element into the set of choices.
             *
             *  @param  name        reference to string containing element's
             *                      name.
             *  @param  elem        address of object representing element.
             *  @exception          cdr::Exception if element already exists
             *                      in this choice.
             */
            void            add(const cdr::String& n, Elem* e);

            /**
             * Accessor method for the number of <code>Elem</code>s defined
             * for this choice.
             *
             *  @return     integer representing the size of the set
             *              of <code>Elem</code> pointers.
             */
            size_t          getElemCount() const { return elemSet.size(); }

            /**
             * Accessor method for maximum number of occurrences allowed for
             * this choice.
             *
             *  @return             integer representing maximum number of
             *                      occurrences allowed for this choice.
             */
            int             getMaxOccs() const { return maxOccs; }

            /**
             * Accessor method for minimum number of occurrences required for
             * this choice.
             *
             *  @return             integer representing minimum number of
             *                      occurrences required for this choice.
             */
            int             getMinOccs() const { return minOccs; }

            /**
             * Accessor method for the elements contained in this choice.
             *
             *  @return     enumerator for walking through the set of element
             *              choices.
             */
            ElemEnum        getElements() const { return elemSet.begin(); }

            /**
             * Accessor method for value which terminates an enumeration loop
             * through the elements available for this choice.
             *
             *  @return     enumerator indicating that the end of the list
             *              of <code>Elem</code> pointers has been passed.
             */
            ElemEnum        getElemEnd() const { return elemSet.end(); }

        private:

            /**
             * Set of available elements which can be selected for this
             * choice.
             */
            ElemSet         elemSet;

            /**
             * Maximum number of occurrences allowed for this choice.
             */
            int             maxOccs;

            /**
             * Minimum number of occurrences required for this choice.
             */
            int             minOccs;
        };

        /**
         * Base class for schema types, both simple and comples.
         */
        class Type {

        public:

            /**
             * Destructor is virtual so the correct derived class's destructor
             * will be invoked at cleanup time.
             */
            virtual ~Type() {}

            /**
             * Accessor method for the name of this <code>Type</code>.
             *
             *  @return         string containing name of this type.
             */
            cdr::String     getName() const { return name; }

        protected:

            /**
             * Creates an uninitialized <code>Type</code> object.  
             * Construction is completed by the derived class.  Objects
             * of the base class cannot be created.
             */
            Type() {}

            /**
             * Invoked by the derived class constructor to establish the name
             * for the type.
             */
            void            setName(const cdr::String& n) { name = n; }

        private:

            /**
             * String containing the name of this type.
             */
            cdr::String     name;
        };

        /**
         * Elemental schema type.
         */
        class SimpleType : public Type {

        public:

            /**
             * Builds a new schema element object from the xsd:simpleType
             * node in the schema document.
             *
             *  @param  schema      reference to <code>Schema</code> object,
             *                      used for finding the base type of a new
             *                      simple type.
             *  @param  dn          node from the schema document describing 
             *                      a new simple type.
             */
            SimpleType(const Schema& schema, const cdr::dom::Node& dn);

            /**
             * Constructs a built-in simple type from "magic" knowledge of
             * certain fundamental ur-types.
             *
             *  @param n        string containing name of a built-in type.
             */
            SimpleType(const cdr::String& n);

            /**
             * Enumerated type representing the allowable encoding for a
             * binary simple type.
             */
            enum Encoding { 
                
                /**
                 * Binary encoding allowing pairs of Hexadecimal digits,
                 * separated by optional whitespace between the pairs.
                 */
                HEX, 
                
                /**
                 * Binary encoding using a limited subset of the ASCII
                 * character set.
                 */
                BASE64 
            };

            /**
             * Token representing the ultimate base for a simple type.
             */
            enum BuiltinType { STRING, DATE, TIME, DECIMAL, INTEGER,
                               URI, BINARY, TIME_INSTANT };

            /**
             * Accessor method for the name of the base type for this
             * <code>SimpleType</code> object.
             *
             *  @return         string containing name of this type base type.
             */
            cdr::String         getBaseName() const { return baseName; }

            /**
             * Accessor method for the minimum length required for values of
             * this type.
             *
             *  @return         integer representing minimum required length
             *                  for values of this type.
             */
            int                 getMinLength() const { return minLength; }

            /**
             * Accessor method for the maximum length allowed for values of
             * this type.
             *
             *  @return         integer representing maximum allowed length
             *                  for values of this type.
             */
            int                 getMaxLength() const { return maxLength; }

            /**
             * Accessor method for the exact length required for values of
             * this type.
             *
             *  @return         integer representing exact length required
             *                  for values of this type; <code>-1</code> if 
             *                  no exact length requirements are imposed 
             *                  on values of this type.
             */
            int                 getLength() const { return length; }

            /**
             * Accessor method for precision requirements imposed on decimal
             * and integer types.
             *
             *  @return         integer representing maximum number of 
             *                  significant digits allowed for decimal or
             *                  integer types; if this type is not based on a
             *                  numeric type the return value is meaningless.
             */
            int                 getPrecision() const { return precision; }

            /**
             * Accessor method for scale requirements imposed on decimal
             * types.
             *
             *  @return         integer representing maximum number of 
             *                  significant digits allowed following the
             *                  decimal point for decimal types; if this 
             *                  type is not based on a decimal type the 
             *                  return value is meaningless.
             */
            int                 getScale() const { return scale; }

            /**
             * Accessor method for encoding requirements imposed on binary
             * types.
             *
             *  @return         <code>BASE64</code> or <code>HEX</code> if
             *                  this type is derived from a binary type;
             *                  otherwise the value is meaningless.
             */
            Encoding            getEncoding() const { return encoding; }

            /**
             * Accessor method for the type from which this type was
             * ultimately derived.
             *
             *  @return         token representing ultimate base type.
             */
            BuiltinType         getBuiltinType() const { return builtinType; }

            /**
             * Accessor method for inclusive lower bound of the valid values
             * allowed for this type.
             *
             *  @return         string representing inclusive lower bound of
             *                  this type's valid value space.
             */
            cdr::String         getMinInclusive() const { return minInclusive; }

            /**
             * Accessor method for inclusive upper bound of the valid values
             * allowed for this type.
             *
             *  @return         string representing inclusive upper bound of
             *                  this type's valid value space.
             */
            cdr::String         getMaxInclusive() const { return maxInclusive; }

            /**
             * Accessor method for the enumerated set of values allowed for
             * this type.
             *
             *  @return         reference to enumerated set of valid values
             *                  for this type.
             */
            const
            cdr::StringSet&     getEnumSet() const { return enumSet; }

            /**
             * Accessor method for regular expression patterns used to
             * validate values of this type.
             *
             *  @return         reference to vector of strings representing
             *                  patterns to be used in validating occurrences
             *                  of this type.
             */
            const
            cdr::StringVector&  getPatterns() const { return patterns; }

        private:

            /**
             * Pointer to type from which this type was derived.
             */
            const SimpleType*   base;

            /**
             * Name of the type from which this type was derived.
             */
            cdr::String         baseName;

            /**
             * Minimum required length for values of this type.  Defaults to
             * zero.
             */
            int                 minLength;

            /**
             * Maximum allowable length for values of this type.  Defaults to
             * <code>INT_MAX</code>.
             */
            int                 maxLength;

            /**
             * Exact length require for values of this type.  Defaults to
             * <code>-1</code>, which indicates that no exact length
             * requirements are imposed on values of this type.
             */
            int                 length;

            /**
             * Maximum number of significant digits allowed for decimal or
             * integer values of this type.  Defaults to <code>-1</code>, 
             * which indicates that no precision limits are imposed on values
             * of this type.
             */
            int                 precision;

            /**
             * Maximum number of significant digits allowed after the decimal
             * point for decimal values of this type.  Defaults to
             * <code-1</code>, which indicates that no scale limits are
             * imposed on values of this type.
             */
            int                 scale;

            /**
             * Encoding required for binary values of this type.  Either
             * <code>HEX</code> or <code>BASE64</code>.
             */
            Encoding            encoding;

            /**
             * Token representing builtin type on which this type is
             * ultimately based.
             */
            BuiltinType         builtinType;

            /**
             * Inclusive lower bound for valid values of this type.
             */
            cdr::String         minInclusive;

            /**
             * Inclusive upper bound for valid values of this type.
             */
            cdr::String         maxInclusive;

            /**
             * Set of enumerated valid values for this type.
             */
            cdr::StringSet      enumSet;

            /**
             * Vector of regular expression patterns used to validate values
             * of this type.
             */
            cdr::StringVector   patterns;
        };

        /**
         * Complex schema type.
         */
        class ComplexType : public Type {

        public:

            /**
             * Constructs a new <code>Complextype</code> object based on the
             * DOM node containing its definition.
             *
             *  @param  schema      reference to <code>Schema</code> object,
             *                      used for registering the elements in 
             *                      the type.
             *  @param  node        reference to DOM node from the schema 
             *                      document describing a new complex type.
             */
            ComplexType(Schema& schema, const cdr::dom::Node& node);

            /**
             * Cleans up resources allocated for this type object.
             */
            ~ComplexType();

            /**
             * Enumerated type containing tokens representing the content
             * model for the complex type.
             */
            enum ContentType { MIXED, ELEMENT_ONLY, TEXT_ONLY, EMPTY };

            /**
             * Accessor method for the content model allowed for this type.
             *
             *  @return     token representing content model allowed for
             *              this complex type.
             */
            ContentType     getContentType() const { return contentType; }

            /**
             * Accessor method for the elements contained in this complex
             * type.  Each node in the sequence actually represents a choice
             * of elements which can appear at the node's position in the
             * sequence.  Most choices will have only one element in the set.
             *
             *  @return     enumerator for the list containing the element
             *              choices expected for this type.
             */
            ChoiceEnum      getElements() const { return elemSeq.begin(); }

            /**
             * Accessor method for the attributes contained in this complex
             * type.
             *
             *  @return     enumerator for the list containing the
             *              attributes expected for this type.
             */
            AttrEnum        getAttributes() const { return attrSet.begin();}

            /**
             * Accessor method for value which terminates an enumeration loop
             * through the elements expected for this complex type.
             *
             *  @return     enumerator indicating that the end of the list
             *              of <code>ElemChoice</code> pointers has been passed.
             */
            ChoiceEnum      getElemEnd() const { return elemSeq.end(); }

            /**
             * Accessor method for value which terminates an enumeration loop
             * through the attributes expected for this complex type.
             *
             *  @return     enumerator indicating that the end of the set
             *              of <code>Attr</code> pointers has been passed.
             */
            AttrEnum        getAttrEnd() const { return attrSet.end(); }

            /**
             * Accessor method for the number of <code>ElemChoice</code>s 
             * defined for this complex type (not the number of element 
             * instances).
             *
             *  @return     integer representing the size of the list
             *              of <code>ElemChoice</code> pointers.
             */
            size_t          getElemCount() const { return elemSeq.size(); }

            /**
             * Accessor method for the number of <code>Attr</code>s
             * defined for this complex type (not the number of attribute
             * instances).
             *
             *  @return     integer representing the number of attributes
             *              defined for this type.
             */
            size_t          getAttrCount() const { return attrSet.size(); }

#if 0
            /**
             * Accessor method for determining whether an particular
             * <code>Elem</code> is defined for this complex type.
             *
             *  @param      string containing the name of the
             *              <code>Elem</code>
             *  @return     <code>true</code> if the specified element
             *              has been defined for this complex type.
             */
            bool            hasElement(const cdr::String& name) const
                { return elemNames.find(name) != elemNames.end(); }
#endif
            /**
             * Accessor method for determining whether an particular
             * <code>Attr</code> is defined for this complex type.
             *
             *  @param      string containing the name of the
             *              <code>Attr</code>
             *  @return     <code>true</code> if the specified attribute
             *              has been defined for this complex type.
             */
            bool            hasAttribute(const cdr::String& name) const
                { return attrSet.find(name) != getAttrEnd(); }

        private:

            /**
             * List of <code>ElemChoice</code>s defined for this type.
             */
            ElemSeq         elemSeq;

            /**
             * List of <code>Attr</code>s defined for this type.
             */
            AttrSet         attrSet;
#if 0
            /**
             * Hashed set of element names used for determining whether a
             * particular element has been defined for this complex type
             * without scanning the entire element list.
             */
            cdr::StringSet  elemNames;

            /**
             * Hashed set of attribute names used for determining whether a
             * particular attribute has been defined for this complex type
             * without scanning the entire attribute list.
             */
            cdr::StringSet  attrNames;
#endif
            /**
             * Token identifying the content model for this complex type
             * (mixed, textOnly, etc.).
             */
            ContentType     contentType;
        };

        /**
         * Checks document against the schema for its document type, reporting 
         * any errors found in the caller's <code>Errors</code> vector.  This 
         * lower-level method is invoked by cdr::validateDocAgainstSchema,
         * which extracts the schema from the database.
         *
         *  @param  docElem             top level element for CdrDoc
         *  @param  schemaElem          top level element for schema
         *  @param  errors              vector of strings to be populated by
         *                              this function
         */
        extern void validateDocAgainstSchema(
                cdr::dom::Element&         docElem,
                cdr::dom::Element&         schemaElem,
                StringList&                errors);
    }
}

#endif
