/*
 * $Id: CdrXsd.h,v 1.18 2005-03-15 01:09:22 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.17  2002/11/25 21:17:33  bkline
 * Finished custom validation support.
 *
 * Revision 1.16  2002/09/02 14:06:48  bkline
 * Plugged memory leak in Schema constructor.
 *
 * Revision 1.15  2002/08/27 17:14:18  bkline
 * Fixed bug in code to get list of elements which can contain a certain
 * attribute; used this to fix code to get list of linking elements for
 * a document type.
 *
 * Revision 1.14  2001/09/19 18:44:13  bkline
 * Added ID/IDREF support, as well as methods for checking for the
 * presence of an attribute.
 *
 * Revision 1.13  2001/05/16 15:50:51  bkline
 * Added getValidValueSets() method to Schema class.  Fixed uninitialized
 * pointer bug in SimpleContent class.
 *
 * Revision 1.12  2001/05/03 18:46:59  bkline
 * Moved in code from ParseSchema.cpp.
 *
 * Revision 1.11  2001/01/17 21:53:37  bkline
 * Added general support for groups, sequences, choices & includes.
 *
 * Revision 1.10  2000/12/28 13:32:33  bkline
 * Added support for choice.
 *
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
#include "CdrDbConnection.h"
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
        const wchar_t* const SCHEMA        = L"schema";

        /**
         * Tag for recursively included schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const INCLUDE       = L"include";

        /**
         * Attribute name for location of recursively included schema 
         * document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const SCHEMA_LOC    = L"schemaLocation";

        /**
         * Tag for element-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ELEMENT       = L"element";

        /**
         * Tag for key-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const KEY           = L"key";

        /**
         * Tag for keyref-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const KEYREF        = L"keyref";

        /**
         * Tag for restriction-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const RESTRICTION   = L"restriction";

        /**
         * Tag for extension-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const EXTENSION     = L"extension";

        /**
         * Tag for selector-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const SELECTOR      = L"selector";

        /**
         * Tag for field-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const FIELD         = L"field";

        /**
         * Tag for choice-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const CHOICE        = L"choice";

        /**
         * Tag for sequence-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const SEQUENCE      = L"sequence";

        /**
         * Tag for group-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const GROUP         = L"group";

        /**
         * Attribute name for references to named groups in a Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const REF           = L"ref";

        /**
         * Tag for attribute-specification element in Schema document.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ATTRIBUTE     = L"attribute";

        /**
         * Tag for Schema element specifying a user-defined complex type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const COMPLEX_TYPE  = L"complexType";

        /**
         * Tag for Schema element specifying a user-defined simple type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const SIMPLE_TYPE   = L"simpleType";
        
        /**
         * Tag for Schema element specifying a text-only complex type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const SIMPLE_CONTENT= L"simpleContent";

        /**
         * Tag for Schema element which serves as the parent wrapper
         * for documentation and appInfo child elements.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const ANNOTATION = L"annotation";

        /**
         * Tag for Schema element which provides application-specific
         * processing information.  In the CDR, this element is used
         * to specify Schematron-like constraints which cannot be
         * represented by version 1.0 of XML Schema.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         */
        const wchar_t* const APP_INFO = L"appInfo";

        /**
         * Tag for Schema element adding a constraing on the minimum length of
         * valid data values.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MIN_LENGTH    = L"minLength";
        
        /**
         * Tag for Schema element adding a constraing on the maximum length of
         * valid data values.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const MAX_LENGTH    = L"maxLength";
        
        /**
         * Tag for Schema element adding a regular-expression pattern used
         * for specifying valid value sets.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const PATTERN       = L"pattern";
        
        /**
         * Tag for Schema element adding a specific value to a set of
         * enumerated valid values for a user-defined simple data type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const ENUMERATION   = L"enumeration";
        
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
        const wchar_t* const STRING        = L"string";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Date type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const DATE          = L"date";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Time type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const TIME          = L"time";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Decimal type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const DECIMAL       = L"decimal";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Integer type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const INTEGER       = L"integer";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in Binary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         *  @note   XXX Remove after compatibility period.
         */
        const wchar_t* const BINARY        = L"binary";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in hexBinary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const HEXBIN        = L"hexBinary";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in base64Binary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const BASE64BIN     = L"base64Binary";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in URI type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const URI           = L"anyURI";
        
        /**
         * Obsolete value for <code>base</code> attribute, indicating 
         * derivation of a user-defined simple type from the built-in 
         * TimeInstant type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         *  @note   XXX Remove after compatibility period.
         */
        const wchar_t* const TIME_INSTANT  = L"timeInstant";

        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in dataTime type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         *  @note   XXX Remove after compatibility period.
         */
        const wchar_t* const DATE_TIME     = L"dateTime";

        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in NMTOKEN type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const NMTOKEN       = L"NMTOKEN";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in ID type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const ID            = L"ID";
        
        /**
         * Value for <code>base</code> attribute, indicating derivation of a
         * user-defined simple type from the built-in IDREF type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const IDREF         = L"IDREF";
        
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
        const wchar_t* const MIN_INCLUSIVE = L"minInclusive";

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
        const wchar_t* const MAX_INCLUSIVE = L"maxInclusive";

        /**
         * Tag for the element used to specify the exact length in characters
         * (for string-based types) or bytes (for binary-derived types).
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const LENGTH        = L"length";

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
        const wchar_t* const PRECISION     = L"totalDigits";

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
        const wchar_t* const SCALE         = L"fractionDigits";

        /**
         * Tag for the element used to specify the encoding allowed (Hex or
         * Base64) for values of a type derived from the built-in Binary type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         *  @note   XXX Remove after compatibility period.
         */
        const wchar_t* const ENCODING      = L"encoding";

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
         * Name of the attribute which specifies the location of a
         * <code>key</code> or <code>keyref</code> in a <code>selector</code>
         * or <code>field</code> element.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const XPATH         = L"xpath";

        /**
         * Name of the attribute which specifies the <code>key</code> to which
         * a <code>keyref</code> refers.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         */
        const wchar_t* const REFER         = L"refer";

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
         *  @note   XXX Remove after compatibility period.
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
        const wchar_t* const UNLIMITED     = L"unbounded";

        /**
         * Value of the <code>encoding</code> attribute specifying hex
         * encoding for a <code>binary</code> data type.
         *
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-1">
         *          XML Schema Part 1: Structures</A>
         *  @see    <A HREF="http://www.w3.org/TR/xmlschema-2">
         *          XML Schema Part 2: Datatypes</A>
         *  @note   XXX Remove after compatibility period.
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
         *  @note   XXX Remove after compatibility period.
         */
        const wchar_t* const BASE64        = L"base64";

        /** 
         * Forward declarations.
         */
        class Type;
        class Element;
        class Attribute;
        class Node;
        class Group;
        class Key;
        class KeyRef;

        /**
         * Aliases for container types.  Provided for convenience (and as a
         * workaround for MSVC++ bugs).
         */
        typedef std::list<Node*>                        NodeList;
        typedef NodeList::const_iterator                NodeEnum;
        typedef std::map<cdr::String, Attribute*>       AttrSet;
        typedef AttrSet::const_iterator                 AttrEnum;
        typedef StringList::const_iterator              StringEnum;
        typedef std::list<Key*>                         KeyList;
        typedef std::list<KeyRef*>                      KeyRefList;
        typedef std::map<cdr::String, 
                         const cdr::StringSet*>         ValidValueSets;

        /**
         * Custom validation rule assertion.
         */
        class Assertion {

        public:

            /**
             * Creates a validation rule assertion from its XML element.
             */
            Assertion(const cdr::dom::Element& e);

            /**
             * Accessor method for retrieving the XPath test which
             * must be satisfied in order for the document to be
             * considered valid.
             *
             *  @return         XPath condition which is asserted.
             */
            cdr::String getTest() const { return test; }

            /**
             * Accessor method for retrieving the error message which
             * will be returned if the assertion fails.
             *
             *  @return         error message string.
             */
            cdr::String getMessage() const { return message; }

        private:

            cdr::String test;
            cdr::String message;
        };

        /**
         * Custom validation rule.
         */
        class Rule {

        public:

            /**
             * Creates a rule from its XML element node.
             *
             *  @param  e       reference to XML element node for a
             *                  custom validation rule.
             */
            Rule(const cdr::dom::Element& e);

            /**
             * Accessor method for obtaining a copy of the 
             * string for the context in which a rule is to be
             * applied.  The context is specified as an XSLT
             * exression, typically the name of an element.
             *
             *  @return         reference to string for rule's test.
             */
            cdr::String getContext() const { return context; }

            /**
             * Accessor method for obtaining a copy of the assertions
             * used for this rule.
             *
             *  @return         copy of the vector of the rule's assertions.
             */
            std::vector<Assertion> getAssertions() const { return assertions; }

        private:

            cdr::String             context;
            std::vector<Assertion>  assertions;
        };

        /**
         * A set of custom rules to be applied to a document.  Only
         * one rule in the set will be applied to any given element
         * node in the document, using the XSLT algorithms for finding
         * the closest match between the XSLT expression for the rule's
         * context and the context in which the element is found in the
         * document.
         */
        class RuleSet {

        public:

            /**
             * Creates a custom rule set from its XML element node.
             *
             *  @param  e       reference to XML element node for
             *                  a set of custom rules.
             *  @param  n       reference to name of set.
             */
            RuleSet(const cdr::dom::Element& e, const cdr::String& n);

            RuleSet() {}

            /**
             * Accessor method for obtaining the name of the rule set
             * (which may be empty).
             */
            cdr::String getName() const { return name; }

            /**
             * Accessor method for obtaining the vector of rules used
             * for this set.
             */
            std::vector<Rule> getRules() const { return rules; }

            /**
             * Method to get the XSLT transformation script for
             * validating a document against the assertions in the
             * set's rules.
             */
            cdr::String getXslt() const;

            /**
             * Merges additional rules into a rule set, so we can 
             * have parts of the same set defined in separate schema
             * files, which get included for a given document type.
             */
            void addRules(const cdr::dom::Element& e);

        private:

            cdr::String         name;
            std::vector<Rule>   rules;
            mutable cdr::String xslt;
        };

        /**
         * Common base class for <code>Key</code> and <code>KeyRef</code>
         * classes.
         */
        class KeyOrKeyRef {

        public:

            /**
             * Accessor method for <code>Key</code>'s or <code>KeyRef</code>'s 
             * name.
             *
             *  @return         string containing name of this key or keyref.
             */
            cdr::String getName() const { return name; }

            /**
             * Accessor method for name of <code>Key</code>'s or
             * <code>KeyRef</code>'s parent element.
             *
             *  @return         string containing name of this key or keyref's
             *                  parent element.
             */
            cdr::String getParent() const { return parent; }

            /**
             * Accessor method for <code>Key</code> or <code>KeyRef</code>'s 
             * selector.
             *
             *  @return         string containing selector for this key or 
             *                  keyref.
             */
            cdr::String getSelector() const { return selector; }

            /**
             * Accessor method for <code>Key</code> or <code>KeyRef</code>'s 
             * list of fields.
             *
             *  @return         string containing name of this key or keyref.
             */
            StringEnum getFields() const { return fields.begin(); }

            /**
             * Accessor method for value which terminates an enumeration loop
             * through the list of fields for this key or keyref.
             *
             *  @return     enumerator indicating that the end of the list
             *              of fields has been passed.
             */
            StringEnum getFieldsEnd() const { return fields.end(); }

        protected:

            cdr::String name;
            cdr::String parent;
            cdr::String selector;
            StringList  fields;
        };

        /**
         * An object of type <code>Key</code> holds the information used to
         * find the attributes in the document type which uniquely identify
         * elements in documents of this type and which can be referred to by
         * keyref attributes.  Note that the selectors for keys and keyrefs in
         * this implementation are restricted to naming the child element of
         * the parent in which the key or keyref is defined, and the fields
         * must identify an attribute of that child element.  We will pick up
         * much more generality for free when we replace this implementation
         * with a third-party implementation.
         */
        class   Key : public KeyOrKeyRef {

        public:

            /**
             * Builds a key object indicating the path and attribute used 
             * to hold the keys for a schema.
             */
            Key(const cdr::dom::Node& node, const cdr::String& parent);

        };

        /**
         * An object of type <code>KeyRef</code> holds the information used to
         * find the attributes in the document type which refer to keyed
         * elements in documents of this type.  Note that in this
         * implementation this information is used solely to identify ID and
         * IDREF attributes in the DTD generated from the Schema.  No attempt
         * is made to validate key-keyref relationship during schema
         * validation.
         */
        class   KeyRef : public KeyOrKeyRef {

        public:

            /**
             * Builds a keyref object indicating the path and attribute used 
             * to hold the keys for a schema.
             */
            KeyRef(const cdr::dom::Node& node, const cdr::String& parent);

            /**
             * Accessor method for name of <code>Key</code> to which this
             * <code>KeyRef</code> refers.
             *
             *  @return         string containing name of this key reference.
             */
            cdr::String getRefer() const { return refer; }

        private:

            cdr::String refer;
        };

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
            Schema(const cdr::dom::Node& node, cdr::db::Connection* conn = 0);

            /**
             * Releases resources allocated for a schema validation object.
             */
            ~Schema() { cleanup(); }

            /**
             * Find a <code>Type</code> object from its name, using the
             * <code>TypeMap</code> of names to types.
             *
             *  @param  name    name assigned to the type.
             *  @return         pointer to type object if found; otherwise
             *                  <code>NULL</code>.
             */
            const Type*         lookupType(const cdr::String& name) const;

            /**
             * Find a <code>Group</code> object from its name, using the
             * <code>GroupMap</code> of names to types.
             *
             *  @param  name    name assigned to the group.
             *  @return         pointer to group object if found; otherwise
             *                  <code>NULL</code>.
             */
            Group*              lookupGroup(const cdr::String& name) const;

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
            Element&            getTopElement() const { return *topElement; }

            /**
             * Remembers the name of the type used for this element.
             *
             *  @param  eName   string containing the name of the element.
             *  @param  tName   string containing the name of the element's
             *                  type.
             */
            void                registerElement(const cdr::String& eName,
                                                const cdr::String& tName);

            /**
             * Remember an object which needs to be freed by the destructor.
             *
             *  @param  node    address of object which will need to be freed
             *                  by the Schema destructor.
             */
            void                registerObject(Node* node);

            /**
             * Remeber a key defined by the schema.
             *
             *  @param  k       address of <code>Key</code> object to
             *                  remember.
             */
            void                addKey(Key* k) { keyList.push_back(k); }

            /**
             * Remeber a keyref defined by the schema.
             *
             *  @param  k       address of <code>KeyRef</code> object to
             *                  remember.
             */
            void                addKeyRef(KeyRef* r) 
                                { keyRefList.push_back(r); }

            /**
             * Generates a DTD for the schema.
             *
             *  @param  schemaFilename
             *                  name of the file from which the schema
             *                  was parsed (top-level file only); used
             *                  solely for comment at the top of the DTD.
             *  @return         string containing the DTD.
             */
            cdr::String         makeDtd(const cdr::String& schemaFilename);

            /**
             * Extracts lists of valid values for elements and attributes
             * found in this schema.  The key represents an element name by
             * itself or an attribute for an element in the form
             * element@attribute.
             *
             *  @param  list    caller's list object to be populated.
             */
            void                getValidValueSets(ValidValueSets& list) const;

            /**
             * Reports whether a given element in the schema has a particular
             * attribute.
             *
             *  @param  elemName    name of element to check.
             *  @param  attrName    name of attribute to check.
             *  @return             <code>true</code> if the element can have
             *                      the attribute named in the second
             *                      parameter; otherwise <code>false</code>.
             */
            bool                hasAttribute(const cdr::String& elemName,
                                             const cdr::String& attrName)
                                             const;

            /**
             * Extract a list of elements which can have a particular
             * attribute.
             *
             *  @param  attrName    name of attribute to check.
             *  @param  elemList    list of element names to be populated.
             */
            void                elemsWithAttr(const cdr::String& attrName,
                                              cdr::StringList& elemList) const;

            /**
             * Accessor method to retrieve the collections of custom
             * validation rules used for this schema.
             */
            const std::map<String, RuleSet>& getRuleSets() const { 
                return ruleSets; 
            }

            /**
             * Look up a rule set by name.
             */
            RuleSet* getRuleSet(const cdr::String& name);

        private:

            /**
             * Map of type names to the corresponding <code>Type</code>
             * objects.
             */
            typedef std::map<cdr::String, Type*> TypeMap;

            /**
             * Map of group names to the corresponding <code>Group</code>
             * objects.
             */
            typedef std::map<cdr::String, Group*> GroupMap;

            /**
             * Map of element names to name of corresponding type.
             */
            typedef std::map<cdr::String, cdr::String> ElemTypeMap;

            /**
             * Registry of types used in the Schema, indexed by type name.
             */
            TypeMap             types;

            /**
             * Named groups defined by this schema.
             */
            GroupMap            groups;

            /**
             * Map from element name to name of type used by the element.
             */
            ElemTypeMap         elements;

            /**
             * List of all nodes which need to be freed by the destructor.
             */
            NodeList            nodeList;

            /**
             * Element forming the top of the DOM tree for this schema.
             */
            Element*            topElement;

            /**
             * List of all keys found in the schema.
             */
            KeyList             keyList;

            /**
             * List of all keyrefs found in the schema.
             */
            KeyRefList          keyRefList;

            /**
             * Releases resources allocated for this Schema object.
             * Split out here so it can be invoked directly by the
             * constructor when an exception is thrown.  Relies
             * on registerObject() to remember what needs to be
             * deleted.
             */
            void cleanup();

            /**
             * Registers the built-in Schema types supported by this
             * implementation.
             */
            void                seedBuiltinTypes();

            /**
             * Plugs a new type in to the type map, which indexes known types
             * by name.
             */
            void                registerType(cdr::xsd::Type*);

            /**
             * Plugs a new group in to the group map, which indexes known
             * groups by name.
             */
            void                registerGroup(cdr::xsd::Group*);

            /**
             * Loads an included schema document and recursively calls
             * parseSchema.
             */
            void                includeSchema(const cdr::dom::Node&,
                                              cdr::db::Connection*);

            /**
             * Extracts schema information from XML document.  Called
             * recursively by constructor for included schema documents.
             */
            void                parseSchema(const cdr::dom::Node&,
                                            cdr::db::Connection*);

            /**
             * Replace GroupRef pointers with pointers to the Group objects
             * being referenced.
             */
            void                resolveGroupRefs();

            /**
             * Attaches ID types to attributes identified as keys by the
             * schema.
             */
            void                resolveKeys();

            /**
             * Attaches IDREF types to attributes identified as keyrefs by the
             * schema.
             */
            void                resolveKeyRefs();

            /**
             * Common processing used by <code>resolveKeys()</code> and
             * <code>resolveKeyRefs()</code>.
             *
             *  @param  k       address of Key or KeyRef object.
             *  @param  type    "ID" or "IDREF"
             */
            void                resolveKeyOrKeyRef(KeyOrKeyRef* key, 
                                                   const cdr::String& type);
            /**
             * Writes a DTD element definition to the specified stream.
             *
             *  @param  element reference to <code>Element</code> object
             *                  to be defined.
             *  @param  os      reference to stream to which definition
             *                  is to be written.
             *  @param  isTopElement
             *                  <code>true</code> if element is the
             *                  top-level element for the document type.
             */
            void                writeDtdElement(Element& element,
                                                std::wostream& os,
                                                bool isTopElement = false);

            /**
             * Writes the DTD declarations for child elements to the specified
             * stream.
             *
             *  @param  declaredElems
             *                  list of DTD elements which have already been
             *                  declared.
             *  @param  n       address of <code>Node</code> representing
             *                  an element or group of elements to be
             *                  declared.
             *  @param  os      reference to stream to which declarations
             *                  are to be written.
             */
            void declareDtdChildElements(cdr::StringSet& declaredElems,
                                         const Node* n,
                                         std::wostream& os);

            /**
             * Recursively examines the current element and its children to
             * see which have cdr:id attributes.
             *
             *  @param  elem        reference to current element.
             *  @param  attrName    name of attribute we're interested in.
             *  @param  elemList    reference to list we're building of 
             *                      elements which have cdr:id attributes.
             *  @param  checked     set to prevent processing the same 
             *                      element multiple times.
             */
            void checkElementForAttribute(cdr::xsd::Element& elem,
                                          const cdr::String& attrName,
                                          cdr::StringList& elemList,
                                          cdr::StringSet& checked) const;

            /**
             * Examine content for a complex type, looking for elements.  
             * Content can be a group, a sequence, a choice, or an element.
             *
             *  @param  node        address of schema node.
             *  @param  attrName    name of attribute we're interested in.
             *  @param  elemList    reference to list we're building of 
             *                      elements which have cdr:id attributes.
             *  @param  checked     set to prevent processing the same 
             *                      element multiple times.
             */
            void checkContentForAttribute(const cdr::xsd::Node* node,
                                          const cdr::String& attrName,
                                          cdr::StringList& elemList,
                                          cdr::StringSet& checked) const;

            /**
             * Directory to be used for locating included schema subdocuments
             * when the filesystem is being used.
             */
            static const std::string   schemaDir;

            /**
             * Vector of rule sets used for custom validation.
             */
            std::map<String, RuleSet>  ruleSets;

            /**
             * Parse the annotation element, looking for rule sets.
             */
            void extractRuleSets(const cdr::dom::Node& node);
        };

        /**
         * Common base class for objects which make up a complex type.
         * Nodes can be named (for example, groups, elements, or attributes)
         * or unnamed (anonymous sequences and choices).
         */
        class Node {

        public:

            /**
             * Destructor is virtual so the correct derived class's destructor
             * will be invoked at cleanup time.
             */
            virtual ~Node() {}
        };

        /**
         * Common base class for named components of a complex type (e.g.,
         * groups, elements, or attributes).
         */
        class NamedNode : public Node {

        public:

            /**
             * Destructor is virtual so the correct derived class's destructor
             * will be invoked at cleanup time.
             */
            virtual ~NamedNode() {}

            /**
             * Accessor method for <code>NamedNode</code>'s name.
             *
             *  @return         string containing name of this element or
             *                  attribute.
             */
            cdr::String     getName() const { return name; }

        protected:

            /**
             * String containing the name of the element or attribute.
             */
            cdr::String     name;
        };

        /**
         * Base class for attributes and elements.
         */
        class NamedAndTypedNode : public NamedNode {

        public:

            /**
             * Destructor is virtual so the correct derived class's destructor
             * will be invoked at cleanup time.
             */
            virtual ~NamedAndTypedNode() {}

            /**
             * Accessor method for <code>Node</code>'s type's name.
             *
             *  @return         string containing name of this element or
             *                  attribute's type.
             */
            cdr::String     getTypeName() const { return typeName; }

            /**
             * Finds the <code>Type</code> object attached to this element or
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
            NamedAndTypedNode() : type(0) {}

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
             * once for this <code>NamedAndTypedNode</code>.
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
         * Base class for classes with minimum/maximum occurrence limits.
         */
        class CountConstrained {

        public:

            /**
             * Accessor method for maximum number of occurrences allowed for
             * this node.
             *
             *  @return             integer representing maximum number of
             *                      occurrences allowed for this node.
             */
            int             getMaxOccs() const { return maxOccs; }

            /**
             * Accessor method for minimum number of occurrences required for
             * this node.
             *
             *  @return             integer representing minimum number of
             *                      occurrences required for this node.
             */
            int             getMinOccs() const { return minOccs; }

        protected:

            /**
             * Maximum number of occurrences allowed for this node.
             */
            int             maxOccs;

            /**
             * Minimum number of occurrences required for this node.
             */
            int             minOccs;
        };

        /**
         * Schema element.  Can be top-level element for the schema's
         * document type, or a possible choice for one of the positions in the
         * sequence or choice of elements for a complex type.
         */
        class Element : public NamedAndTypedNode, public CountConstrained {

        public:

            /**
             * Extracts the specification of the requirements for this element
             * from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this choice or sequence.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this element.
             */
            Element(Schema& s, const cdr::dom::Node& n);

            /**
             * Clean up resources used by this object.
             */
            ~Element() {}
        };

        /**
         * Attribute attached to a schema element.
         */
        class Attribute : public NamedAndTypedNode {

        public:

            /**
             * Extracts the specification of the requirements for this
             * attribute from its schema node.
             * 
             *  @param  s           reference to schema object which uses 
             *                      this choice or sequence.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this attribute.
             */
            Attribute(Schema& s, const cdr::dom::Node& n);

            /**
             * Clean up resources used by this object.
             */
            ~Attribute() {}

            /**
             * Accessor method for reporting whether this attribute can be
             * omitted.
             *
             *  @return             <code>true</code> if the attribute
             *                      can be omitted for this element.
             */
            bool            isOptional() const { return optional; }

            /**
             * Accessor method for retrieving the string to be used in a DTD
             * to identify the attribute's type (CDATA, ID, IDREF).
             *
             *  @return             CDATA, ID or IDREF
             */
            cdr::String     getDtdType() const { return dtdType; }

            /**
             * Method to set the dtdType based on a key or keyref element in
             * the schema.
             *
             *  @param  type        CDATA, ID or IDREF (string)
             */
            void            setDtdType(const cdr::String& type) // const
                            { dtdType = type; }

        private:

            /**
             * Flag recording whether this element can be omitted.
             */
            bool            optional;

            /**
             * String used to identify the attribute type in a DTD
             * (CDATA, ID, or IDREF currently supported).
             */
            cdr::String     dtdType;
        };

        /**
         * Base class for Choice and Sequence classes, whose implementation is
         * identical but whose semantics differ.
         */
        class ChoiceOrSequence : public Node, public CountConstrained {

        public:

            /**
             * Clean up resources used by this object.
             */
            ~ChoiceOrSequence() {}

            /**
             * Accessor method for the elements contained in this object.
             *
             *  @return     enumerator for walking through the set of nodes.
             */
            NodeEnum        getNodes() const { return nodeList.begin(); }

            /**
             * Accessor method for value which terminates an enumeration loop
             * through the elements available for this choice.
             *
             *  @return     enumerator indicating that the end of the list
             *              of <code>Node</code> pointers has been passed.
             */
            NodeEnum        getListEnd() const { return nodeList.end(); }

            /**
             * Replace GroupRef pointers with pointers to the Group objects
             * being referenced.
             */
            void            resolveGroupRefs(const Schema&);

            /**
             * Report the number of nodes in the node list.
             */
            int             getNodeCount() const { return nodeList.size(); }

        protected:

            /**
             * Extracts the specification of the requirements for this choice
             * or sequence from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this choice or sequence.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this choice or sequence.
             */
            ChoiceOrSequence(Schema& s, const cdr::dom::Node& n);

        private:

            /**
             * Set of available nodes which can be selected for this
             * choice.
             */
            NodeList        nodeList;

            /**
             * Flag to prevent infinite recursion by resolveGroupRefs().
             */
            bool            groupRefsResolved;
        };

        /**
         * Set of alternate nodes, any one of which is acceptable in a
         * given context.
         */
        class Choice : public ChoiceOrSequence {

        public:

            /**
             * Extracts the specification of the requirements for this
             * choice from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this choice.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this choice.
             */
            Choice(Schema& s, const cdr::dom::Node& n) :
                ChoiceOrSequence(s, n) {}

            /**
             * Clean up resources used by this object.
             */
            ~Choice() {}
        };

        /**
         * List of objects which are expected to occur in a given order,
         * if at all.
         */
        class Sequence : public ChoiceOrSequence {

        public:

            /**
             * Extracts the specification of the requirements for this
             * sequence from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this element sequence.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this sequence.
             */
            Sequence(Schema& s, const cdr::dom::Node& n) :
                ChoiceOrSequence(s, n) {}

            /**
             * Clean up resources used by this object.
             */
            ~Sequence() {}
        };

        /**
         * Named sequence or choice.
         */
        class Group : public NamedNode {

        public:

            /**
             * Extracts the specification of the requirements for this
             * group from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this group.
             *  @param  n           reference to DOM node holding validation
             *                      requirements for this group.
             */
            Group(Schema& s, const cdr::dom::Node& n);

            /**
             * Clean up resources used by this object.
             */
            ~Group() {}

            /**
             * Accessor method for Group's Sequence.
             *
             *  @return             address of Sequence comprising this
             *                      named group, if appropriate; NULL if
             *                      group contains a Choice instead of a
             *                      Sequence.
             */
            const Sequence* getSequence() const;

            /**
             * Accessor method for Group's Choice.
             *
             *  @return             address of Choice comprising this
             *                      named group, if appropriate; NULL if
             *                      group contains a Sequence instead of a
             *                      Choice.
             */
            const Choice*   getChoice() const;

            /**
             * Accessor method for getting sequence or choice when it isn't
             * significant which of these is represented by the named group.
             *
             *  @return             address of choice or sequence object
             *                      which this group represents.
             */
            const ChoiceOrSequence* getContent() const {return node; }

            /**
             * Replace GroupRef pointers with pointers to the Group objects
             * being referenced.
             */
            void            resolveGroupRefs(const Schema&);

        private:

            /**
             * Address of Sequence or Choice comprising this Group.
             */
            ChoiceOrSequence*       node;
        };

        /**
         * Unresolved reference to a named group.
         */
        class GroupRef : public NamedNode {

        public:

            /**
             * Extracts the reference to a named group from its schema node.
             *
             *  @param  s           reference to schema object which uses 
             *                      this group.
             *  @param  n           reference to DOM node holding this group
             *                      reference.
             */
            GroupRef(Schema& s, const cdr::dom::Node& n);

            /**
             * Clean up resources used by this object.
             */
            ~GroupRef() {}
        };

        /**
         * Base class for schema types, both simple and comples.
         */
        class Type : public Node {

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
             * Clean up resources used by this object.
             */
            ~SimpleType() {}

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
             * XXX remove BINARY and TIME_INSTANT after compatibility period.
             */
            enum BuiltinType { STRING, DATE, TIME, DECIMAL, INTEGER,
                               URI, BINARY, TIME_INSTANT, DATE_TIME, NMTOKEN,
                               HEXBIN, BASE64BIN, ID, IDREF };

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
         * Content of a text-only element with attributes.  During the initial
         * pass of parsing the schema, we store the type name taken from the
         * 'base' attribute.  When we've gathtered together all the types, we
         * then resolve this name to the simple type to which it refers.
         */
        class SimpleContent : public NamedNode {

        public:

            /**
             * Starts out by just knowing the simple type's name.
             *
             *  @param  n           name of the simple type for the text
             *                      content of elements of this type.
             *  @param  s           address of the schema (we'll need
             *                      this to look up the simple type).
             */
            SimpleContent(const cdr::String& n, const Schema* s) 
                         : simpleType(0), schema(s) { name = n; }

            /**
             * Accessor method to obtain the type used to constrain the text
             * content of elements of this type.
             *
             *  @return             address of <code>SimpleType</code>
             */
            const SimpleType*   getSimpleType() const { return resolveType(); }

        private:

            /**
             * The first time this method is invoked it performs a lookup from
             * the name of the simple type to find the address of the simple
             * type's object, cacheing the result in <code>simpleType</code>
             * for future calls.
             */
            const SimpleType*   resolveType() const {
                if (!simpleType) simpleType = 
                    dynamic_cast<const SimpleType*>(schema->lookupType(name));
                if (!simpleType)
                    throw cdr::Exception(L"Base type not found "
                                         L"for simpleType ", name);
                return simpleType;
            }

            const SimpleType mutable* simpleType;
            const Schema*             schema;
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
            ~ComplexType() {}

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
             * type.
             *
             *  @return     address of object for the sequence, choice,
             *              group, or simple text type which makes up the 
             *              content for this type.
             */
            const Node*     getContent() const { return content; }

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
             * through the attributes expected for this complex type.
             *
             *  @return     enumerator indicating that the end of the set
             *              of <code>Attribute</code> pointers has been passed.
             */
            AttrEnum        getAttrEnd() const { return attrSet.end(); }

            /**
             * Accessor method for the number of <code>Attribute</code>s
             * defined for this complex type (not the number of attribute
             * instances).
             *
             *  @return     integer representing the number of attributes
             *              defined for this type.
             */
            size_t          getAttrCount() const { return attrSet.size(); }

            /**
             * Accessor method for determining whether an particular
             * <code>Attribute</code> is defined for this complex type.
             *
             *  @param name string containing the name of the
             *              <code>Attribute</code>
             *  @return     <code>true</code> if the specified attribute
             *              has been defined for this complex type.
             */
            bool            hasAttribute(const cdr::String& name) const
                { return attrSet.find(name) != getAttrEnd(); }

            /**
             * Replace GroupRef pointers with pointers to the Group objects
             * being referenced.
             */
            void            resolveGroupRefs(const Schema&);

            /**
             * Replaces the string used by a DTD to identify the type of an
             * attribute.
             *
             *  @param  attrName        name of the attribute whose DTD type
             *                          is to be set.
             *  @param  typeName        new DTD type name for the attribute.
             */
            void            setAttrDtdType(const cdr::String& attrName,
                                           const cdr::String& typeName) const;

        private:

            /**
             * Tree representing elements expected for this type.
             */
            Node*           content;

            /**
             * List of <code>Attribute</code>s defined for this type.
             */
            AttrSet         attrSet;

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
                StringList&                errors,
                cdr::db::Connection*       conn = 0);

        /**
         * Determine whether the string matches the NMTOKEN production of
         * http://www.w3.org/TR/2000/REC-xml-20001006:
         *      Nmtoken     ::=    (NameChar)+ 
         *      NameChar    ::=     Letter | Digit | '.' | '-' | '_' | ':' |
         *                          CombiningChar | Extender 
         * This function is exported because it is needed elsewhere than in
         * the validation module (specifically, the ParseSchema program uses
         * it when converting an XML Schema to a DTD).
         * 
         *  @param  s                   reference to string value to be
         *                              matched against NMTOKEN production.
         *  @return                     <code>true</code> iff value matches
         *                              NMTOKEN production.
         */
        bool isNMToken(const cdr::String& s);
    }
}

#endif
