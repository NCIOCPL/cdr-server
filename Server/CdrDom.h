/*
 * Wrapper for XML document object model interface.
 */

#ifndef CDR_DOM_H_
#define CDR_DOM_H_

// Shut off silly Microsoft warnings.
#pragma warning(disable : 4290)

// System headers.
#include <string>
#include <iostream>

// Interface to Apache XML parsers.
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/Parser.hpp>
#include <xercesc/sax/SaxException.hpp>
#include <xercesc/sax/SaxParseException.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include "CdrString.h"

/**@#-*/

// Used only in special debugging builds of the software
extern void domLog(const char *);
extern void domLog(const char *, const cdr::String&);

namespace cdr {
    namespace dom {

/**@#+*/

        /** @pkg cdr.dom */

        // This is a wrapper around Xerces-C++ DOM API library.
        // Converts a subset of the new Xerces to make it backward
        //   compatible with the older xml4c library used in
        //   developing the CDR.
        // CDR programmers have the following options with respect
        //   to the use of this library:
        //
        //   1. Use it as is, extending it if necessary by importing
        //      additional Xerces-C++ functionality if and when it
        //      is needed.
        //
        //   2. Circumvent it by using Xerces-C++ directly.
        //
        // Our current plan is stick with method 1 - primarily in
        // order to maintain a single point of control from which we
        // can perform any specialized functions such as debugging
        // DOM calls.
        //
        // The primary function of the wrapper is to enable the use
        // of objects directly rather than through pointers - which is
        // the newer Xerces-C++ technique.  Our classes provide object
        // wrappers around Xerces-C++ pointers, with memory management
        // builtin to the object destructors where required.

        // Forward declarations
        class Node;
        class Element;
        class Document;

        /**
         * Unordered collection of nodes, accessible by name
         * or by ordinal index.
         */
        class NamedNodeMap {

        public:
            /**
             * Default constructor.
             */
            NamedNodeMap() { pNodeMap = 0; }

            /**
             * Construct a new <code>Node<code> object from a
             * DOMNamedNodeMap extracted from a Node.
             *
             * @param mapPtr  Pointer to Xerces object.
             */
            NamedNodeMap(XERCES_CPP_NAMESPACE::DOMNamedNodeMap *mapPtr) {
                pNodeMap = mapPtr;
            }

            /**
             * Destructor.
             * Nodes returned by parser are freed automatically
             *   when parser is destroyed.
             * Nodes created by program must be freed by us
             */
            ~NamedNodeMap() {}

            /**
             * Get a named item from the map.
             *
             * @param name  Name of node to retrieve.
             */
            cdr::dom::Node getNamedItem(cdr::String name);

            /**
             * Get item by ordinal index.  Index is not in document
             * order.
             *
             * @param item  Ordinal number of item.
             */
            cdr::dom::Node item(int index);

            /**
             * Get count of nodes in the map.
             */
            int getLength() {
                return pNodeMap ? (int) pNodeMap->getLength() : 0;
            }

        private:
            // Actual map returned from Xerces parser
            // It's a pointer
            XERCES_CPP_NAMESPACE::DOMNamedNodeMap *pNodeMap;

         };

        /**
         * Ordered collection of nodes, accessible by ordinal index.
         * NodeLists are created by Element::getElementsByTagName().
         */
        class NodeList {

        public:
            /**
             * Default constructor.
             */
            NodeList() { nodeList = 0; }

            /**
             * Construct a new <code>NodeList<code> object from a
             * Xerces DOMNodeList.
             *
             * @param nl    Pointer to Xerces object.
             */
            NodeList(XERCES_CPP_NAMESPACE::DOMNodeList *nl) {
                nodeList = nl;
            }

            /**
             * Destructor.
             * Objects returned by parser are freed automatically
             *   when parser is destroyed.
             */
            ~NodeList() {}

            /**
             * Get an item from the list.
             *
             * @param index  Position of node to retrieve.
             */
            cdr::dom::Node item(int index) const;
            //{
            //    return nodeList ? nodeList->item(index) : 0;
            //}

            /**
             * Get count of nodes in the list.
             */
            int getLength() const {
                return nodeList ? (int)nodeList->getLength() : 0;
            }

        private:
            // Actual list returned from Xerces parser
            // It's a pointer
            XERCES_CPP_NAMESPACE::DOMNodeList *nodeList;

         };

        /**
         * Primary datatype for the DOM model.
         */
        class Node {

        public:
            /**
             * Translations of Node type enumerations in Xerces
             */
            typedef short NodeType;

            static const NodeType ELEMENT_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::ELEMENT_NODE;
            static const NodeType ATTRIBUTE_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::ATTRIBUTE_NODE;
            static const NodeType TEXT_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::TEXT_NODE;
            static const NodeType CDATA_SECTION_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::CDATA_SECTION_NODE;
            static const NodeType ENTITY_REFERENCE_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::ENTITY_REFERENCE_NODE;
            static const NodeType ENTITY_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::ENTITY_NODE;
            static const NodeType PROCESSING_INSTRUCTION_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::PROCESSING_INSTRUCTION_NODE;
            static const NodeType COMMENT_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::COMMENT_NODE;
            static const NodeType DOCUMENT_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::DOCUMENT_NODE;
            static const NodeType DOCUMENT_TYPE_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::DOCUMENT_TYPE_NODE;
            static const NodeType DOCUMENT_FRAGMENT_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::DOCUMENT_FRAGMENT_NODE;
            static const NodeType NOTATION_NODE =
                 XERCES_CPP_NAMESPACE::DOMNode::NOTATION_NODE;

            /**
             * Default constructor.
             */
            Node() {
                pNode = 0;
            }

            /**
             * Construct a new <code>Node<code> object from a node
             * produced by the XercesDOMParser that can be
             * used in the CDR manner as an allocated object instead
             * of the newer Xerces way as a pointer.
             *
             * @param nodePtr  Pointer to Xerces node returned by parser.
             */
            Node(XERCES_CPP_NAMESPACE::DOMNode *nodePtr) {
                pNode = nodePtr;
            }

            /**
             * Construct a Node object from a Xerces Element pointer.
             *
             * @param elemPtr  Pointer to Xerces Element.
             */
            Node(XERCES_CPP_NAMESPACE::DOMElement *elemPtr) {
                pNode = static_cast<XERCES_CPP_NAMESPACE::DOMNode *>(elemPtr);
            }

            /**
             * Construct a Node object from a cdr::dom::Element.
             *
             * @param elem     Our Element wrapper.
             */
            Node(cdr::dom::Element& elem);

            /**
             * Construct a Node object from a Xerces Document pointer.
             *
             * @param elemPtr  Pointer to Xerces Element.
             */
            Node(XERCES_CPP_NAMESPACE::DOMDocument *docPtr) {
                pNode =
                    static_cast<XERCES_CPP_NAMESPACE::DOMDocument *>(docPtr);
            }

            /**
             * Destructor.
             * Nodes returned by parser are freed automatically
             *   when parser is destroyed.
             * Nodes created by program must be freed by us
             */
            ~Node() {}

            /**
             * Get first child as a Node.
             */
            Node getFirstChild() const {
                return Node(pNode->getFirstChild());
            }

            /**
             * Get right sibling of this node.
             */
            Node getNextSibling() const {
                return Node(pNode->getNextSibling());
            }

            /**
             * Get an unordered collection of attributes of the node.
             *
             * @return  NamedNodeMap of attributes, or 0.
             */
            cdr::dom::NamedNodeMap getAttributes() const {
                return cdr::dom::NamedNodeMap(pNode->getAttributes());
            }

            /**
             * Get the name of the current node.
             */
            cdr::String getNodeName() const {
                return (cdr::String(pNode->getNodeName()));
            }

            /**
             * Get the string value of the current node.
             */
            cdr::String getNodeValue() const;

            /**
             * Get the type, one of the enumerated constants.
             */
            short getNodeType() const {
                return pNode->getNodeType();
            }

            /**
             * Get a string containing the catenation of the text
             * of this node and any sub-element nodes, but not including
             * processing instructions.
             *
             * Now works on attribute nodes as well as element nodes.
             *
             * See also external function getTextContent, which is a
             * wrapper for this.
             */
            cdr::String getTextContent() const {
                return (cdr::String(pNode->getTextContent()));
            }

            /**
             * Helper function for Element constructor that converts
             * a Node to an Element.
             */
            XERCES_CPP_NAMESPACE::DOMNode *getNodePtr() const {
                return pNode;
            }

            /**
             * Needed to make it possible to compare
             * a Node with NULL.
             */
            bool operator == (const Node & other) const {
                return this->pNode == other.pNode;
            }
            bool operator != (const Node & other) const {
                return this->pNode != other.pNode;
            }
            bool operator == (const int ptrValue) const {
                return this->pNode ==
                    (XERCES_CPP_NAMESPACE::DOMNode *) ptrValue;
            }
            bool operator != (const int ptrValue) const {
                return this->pNode !=
                    (XERCES_CPP_NAMESPACE::DOMNode *) ptrValue;
            }

        protected:
            /**
             * Pointer to actual node returned from Xerces parser.
             */
            XERCES_CPP_NAMESPACE::DOMNode *pNode;
        };

        /**
         * Element, actually a subclass of Node, but not handled that
         * way syntactically.
         * XXX THIS MAY BE CAUSING PROBLEMS WHEN USER CASTS!!!
         */
        class Element : public Node {

        public:
            /**
             * Default constructor.
             */
            Element() {
                pNode = 0;
            }

            /**
             * Construct an element node from a pointer returned by
             * the parser.
             *
             * @param elemPtr  Element pointer from parser.
             */
            Element(XERCES_CPP_NAMESPACE::DOMElement *elemPtr) {
                pNode = static_cast<XERCES_CPP_NAMESPACE::DOMNode *>(elemPtr);
            }

            /**
             * Constructor used in converting a Node to an Element.
             *
             * WARNING:
             *   Check type first before making static cast.
             *
             * @param elem      Reference to Node to be cast.
             */
            Element(const cdr::dom::Node& node) {
                pNode = node.getNodePtr();
            }

            /**
             * Constructor used in casting a Document element to an Element.
             *
             * @param doc       Reference to Document to be cast.
             */
            Element(const cdr::dom::Document& doc);

            /**
             * Get a single attribute of an element node.
             *
             * @param name  Name of the attribute.
             * @return      Attribute value, or 0 if attribute non-existent.
             */
            cdr::String getAttribute(const cdr::String name) const {
                return (cdr::String(
                    (static_cast<XERCES_CPP_NAMESPACE::DOMElement *>(pNode))
                      ->getAttribute((const XMLCh*)name.c_str())));
            }

            /**
             * Get a list of nodes for elements of specified name.
             *
             * @param name  Name of the elements to look for.
             * @return      An object representing the list of matching nodes.
             */
            NodeList getElementsByTagName(const cdr::String name) const {
                const XERCES_CPP_NAMESPACE::DOMElement *e = castToElement();
                const XMLCh* n = (const XMLCh*)name.c_str();
                return NodeList(e->getElementsByTagName(n));
            }

            /**
             * Get the name of the current Element node.
             */
            cdr::String getNodeName() const {
                return (cdr::String(pNode->getNodeName()));
            }

            /**
             * Get first child as a Node.
             */
            Node getFirstChild() const {
                return Node(pNode->getFirstChild());
            }

            /**
             * Get a string containing the catenation of the text
             * of this node and any sub-element nodes.
             *
             * See Node.getTextContent().
             */
            cdr::String getTextContent() const {
                return (cdr::String(pNode->getTextContent()));
            }

            /**
             * Needed to make it possible to compare
             * an Element with NULL.
             */
            /*  XXX DON'T NEED THIS WITH SUBCLASS?
            bool operator == (const Element & other) const {
                return this->pNode == other.pNode;
            }
            bool operator != (const Element & other) const {
                return this->pNode != other.pNode;
            }
            */

        private:
            // Helper inline method simplifies code above.
            const XERCES_CPP_NAMESPACE::DOMElement* castToElement() const {
                return static_cast<const XERCES_CPP_NAMESPACE::DOMElement*>
                    (pNode);
            }
        };

        /**
         * Document class.
         */
        class Document {
        public:
            /**
             * Construct a Document from a pointer to a document
             * node returned by a parser.
             *
             * @param docPtr    Pointer to Document from parser.
             */
            Document(XERCES_CPP_NAMESPACE::DOMDocument *docPtr) {
                pDoc = docPtr;
            }

            /**
             * Get the top level element of a document.
             */
            Element getDocumentElement() const {
                return Element(pDoc->getDocumentElement());
            }

            /**
             * Get first child as a Node.
             */
            Node getFirstChild() const {
                return Node(pDoc).getFirstChild();
            }

            /**
             * Helper function for Element constructor that converts
             * a Document to an Element.
             */
            XERCES_CPP_NAMESPACE::DOMDocument *getDocPtr() const {
                return pDoc;
            }

            /**
             * Needed to make it possible to compare
             * a Document with NULL, same as for Node
             */
            bool operator == (const Document & other) const {
                return this->pDoc == other.pDoc;
            }
            bool operator != (const Document & other) const {
                return this->pDoc != other.pDoc;
            }

        private:
            XERCES_CPP_NAMESPACE::DOMDocument *pDoc;

        };

        /**
         * Carries error information for a failure of DOM processing.
         */
        typedef XERCES_CPP_NAMESPACE::XMLException      XMLException;
        typedef XERCES_CPP_NAMESPACE::DOMException      DOMException;
        typedef XERCES_CPP_NAMESPACE::SAXException      SAXException;
        typedef XERCES_CPP_NAMESPACE::SAXParseException SAXParseException;

        /**
         * Wrap Parser class, which is not part of standard.
         *
         * All memory for nodes, named node maps, etc. allocated
         * by the parser is held by the wrapped Xerces object.
         *
         * In the recent versions of Xerces and going forward, all memory
         * in a DOM tree is tracked in the Parser object.  When the
         * XercesDOMParser is destroyed, it destroys all memory allocated
         * for the DOM tree during the parse.
         *
         * This is different from old xml4c behavior.
         *
         * Some of our CDR software (e.g., validateDocAgainstSchema())
         * parses an XML document in a subroutine and returns the parse
         * tree to a higher level calling function.  We have left that
         * functionality alone when switching to Xerces and instead have
         * installed two different memory management models for
         * cdr::dom::Parser objects - one of which cleans up memory when
         * the Parser object goes out of scope, and one of which (the
         * default) that holds it until later.
         *
         * Always use the default method (saving memory until later) unless
         * you are absolutely sure that no DOM tree memory is referenced
         * after the Parser object that created the tree goes out of scope.
         * If you are wrong about this, fatal memory access violations will
         * certainly result.
         *
         * See the constructor and destructor for details.
         */
        class Parser : public XERCES_CPP_NAMESPACE::XercesDOMParser {

        public:

            /**
             * Creates a new <code>Parser</code> object, with our own custom
             * error handling object plugged into it.
             *
             *  @param saveMem  Flag indicating whether to save the Xerces
             *                  parse tree pointers in thread static memory or
             *                  not.
             *                  <ul>
             *                  <li>
             *                    true = save the parse tree beyond the
             *                    scope of the parser itself.  This is
             *                    the default.  Memory will be released
             *                    when deleteAllThreadParsers() is called.
             *                  </li>
             *                  <li>
             *                    false = delete all memory allocated to
             *                    the parse tree when the parser goes out
             *                    of scope.
             *                    <strong>Do not set saveMem=false unless
             *                       you know for certain that nothing
             *                       whatever in the parse tree will be
             *                       ever be referenced after the Parser
             *                       goes out of scope</strong>
             *                  </li>
             *                  </ul>
             */
            Parser(bool saveMem = false);

            /**
             * Releases the resources allocated for our error handler.
             */
            ~Parser();

            /**
             * Parses a narrow-character string, creating a DOM tree.
             *
             *  @param  xml     reference to string containing the UTF-8
             *                  serialization of an XML document.
             *  @exception      DOMException if a parsing error is encountered.
             */
            void parse(const std::string& xml) throw(DOMException);

            /**
             * Parses a wide-character string, creating a DOM tree.
             *
             *  @param  xml     reference to string containing the UTF-16
             *                  serialization of an XML document.
             *  @exception      DOMException if a parsing error is encountered.
             */
            void parse(const cdr::String& xml) throw(DOMException) {
                parse(xml.toUtf8());
            }

            /**
             * Parses a document directly from a file.
             *
             *  @param  fileName    c-style (null-terminated) string
             *                      identifying name of XML file.
             *  @exception          DOMException if file not found or a
             *                      parsing error is encountered.
             */
            void parseFile(const char* fileName) throw(DOMException) {
                pXparser->parse(fileName);
            }

            /**
             * Get the DOM Document resulting from a parse.
             */
            Document getDocument() {
                return Document(pXparser->getDocument());
            }

            /**
             * Delete any and all parser objects accumulated during
             * the processing of one transaction for a thread.
             *
             * Must be called when we finish a transaction and know
             * that no DOM trees created during the processing of that
             * transaction are required any more.  Deleting the
             * parser objects used to create those DOM trees also
             * frees all memory in the trees themselves.
             */
            static void deleteAllThreadParsers();

        private:

            /**
             * Pointer to actual Xerces DOM parser object.
             */
            XERCES_CPP_NAMESPACE::XercesDOMParser *pXparser;

            /**
             * Address of an instance of the error handler specialized for
             * the CDR.
             */
            XERCES_CPP_NAMESPACE::ErrorHandler *errorHandler;

            /**
             * Indicate whether to save the underlying Xerces parser
             * structures beyond the scope of the object, or to delete
             * them in our own destructor when the Parser goes out of
             * scope.  See comments above in the class description and
             * in the constructor.
             */
            bool savingMem;

            /**
             * Artificial flag used to perform initialization required by the
             * xerces package.
             */
            static bool initialized;

            /**
             * Perform initialization required by the xerces package.
             *
             *  @exception      DOMException if a parsing error is encountered.
             */
            static bool doInit() throw(DOMException) {
                XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
                return true;
            }

            /**
             * Disabled copy constructor.
             */
            Parser(const Parser&);

            /**
             * Disabled assignment operator.
             */
            Parser& operator=(const Parser&);

        };

        /**
         * Convenience method, for extracting the text content
         * from an element.
         *
         * When first implemented, this was not part of the DOM
         * interface recommendation.  There is now a Level 3 method
         * of the Node interface.
         *
         * This is now implemented in Node, but is retained here for
         * backward compatibility.
         *
         *  @param  node        reference to DOM node from which text
         *                      content is to be extracted.
         */
        extern cdr::String getTextContent(const Node& node);

        /**
         * See getTextContent().
         *
         *  @param  elem        reference to DOM Element from which text
         *                      content is to be extracted.
         */
        extern cdr::String getTextContent(const Element& elem);

        /**
         * Determine whether an element has any text.
         *
         * This only checks the element itself, not subelements.  It is
         * used, for example, in schema validation to verify that a pure
         * container element has no immediate text children.
         *
         * The caller may ask that whitespace not be counted as text.
         *
         *  @param  elem                reference to element to be checked.
         *  @param  excludeWhiteSpace   true  = whitespace only does not count
         *                                  as text.
         *                              false = any text counts.
         *
         *  @return                     true = Element has text.
         */
        extern bool hasText(const Element& elem,
                            const bool excludeWhiteSpace);
    }
}

/**
 * Another convenience method that everyone wishes were in
 * the standard interface.
 *
 *  @param  stream      reference to stream for serialization.
 *  @param  node        reference to DOM node to be serialized.
 *  @return             reference to stream used for serialization.
 */
extern std::wostream& operator<<(std::wostream&, const cdr::dom::Node&);

#endif
