/*
 * $Id: CdrDom.cpp,v 1.10 2005-03-04 02:51:12 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.9  2004/03/23 16:26:47  bkline
 * Upgraded to version 5.4.0 of xml4c (but still using deprecated APIs).
 *
 * Revision 1.8  2002/11/21 21:01:13  bkline
 * Fixed serializing code in insertion operator (replacing special
 * characters with entities).
 *
 * Revision 1.7  2001/10/17 13:51:28  bkline
 * Added output insertion operator.
 *
 * Revision 1.6  2000/10/05 21:25:09  bkline
 * Added parseFile(const char*) method.
 *
 * Revision 1.5  2000/10/04 18:26:40  bkline
 * Modified access to exception error message strings.
 *
 * Revision 1.4  2000/05/09 21:07:48  bkline
 * Added error handler.
 *
 * Revision 1.3  2000/04/26 01:30:38  bkline
 * Added second parse() method, for taking a cdr::String directly.
 *
 * Revision 1.2  2000/04/15 14:06:39  bkline
 * Added getTextContent function.
 *
 * Revision 1.1  2000/04/11 14:16:41  bkline
 * Initial revision
 *
 */

#include <iostream> // XXX Substitute logging when in place.
#include "CdrDom.h"
#include "CdrException.h"
#include <xercesc/framework/MemBufInputSource.hpp> // XXX Do we still need this?
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

            /**
             * Max number of Xerces Parsers that we allocate.
             */
            static const int MAX_PARSERS = 20;

            /**
             * Array of pointers to Xerces parsers.
             *
             * We keep all of them around for the execution of
             * one transaction in order to be sure that we have
             * finished using any DOM trees created by any of them
             * before deleting the parsers - which also deletes all
             * Node memory in the DOM tree created by that parser.
             *
             * An array is used rather than a vector because an std::vector
             * uses a constructor, which is not allowed with thread
             * memory.
             *
             * If our guess on the max is too low, something is
             * seriously wrong with our conception of what happens in
             * one transaction.
             */
            static __declspec(thread) int guard1 = 0;
            static __declspec(thread)
                   XERCES_CPP_NAMESPACE::XercesDOMParser
                   *allThreadParsers[MAX_PARSERS];

            /**
             * Error handlers associated with the above DOM parsers.
             */
            static __declspec(thread) int guard2 = 0;
            static __declspec(thread)
                   XERCES_CPP_NAMESPACE::ErrorHandler
                   *allErrorHandlers[MAX_PARSERS];

            /**
             * Index of next free parser pointer in allThreadParsers.
             */
            static __declspec(thread) int guard3 = 0;
            static __declspec(thread) int nextThreadParser = 0;
            static __declspec(thread) int guard4 = 0;

            // For debug - how many parsers were created but not saved
            static __declspec(thread) int countNonSavedParsers = 0;

namespace cdr {
    namespace dom {
        class ErrorHandler : public XERCES_CPP_NAMESPACE::ErrorHandler {
        public:
            void warning(const SAXParseException& e) {
                // Substitute logging when in place.
                std::wcerr << L"*** DOM WARNING: " << getMessage(e)
                           << std::endl;
            }
            void error(const SAXParseException& e) {
                throw cdr::Exception(L"DOM ERROR", getMessage(e));
            }
            void fatalError(const SAXParseException& e) {
                throw cdr::Exception(L"FATAL DOM ERROR", getMessage(e));
            }
            void resetErrors() {}
        private:
            cdr::String getMessage(const SAXParseException& e) {
                wchar_t extra[80];
                swprintf(extra, L" [at line %d, column %d]",
                         e.getLineNumber(), e.getColumnNumber());
                return cdr::String(e.getMessage()) + extra;
            }
        };
    }
}

cdr::dom::Node cdr::dom::NamedNodeMap::getNamedItem(cdr::String name) {
    return pNodeMap ?
        cdr::dom::Node(pNodeMap->getNamedItem((const XMLCh*)name.c_str())) :
        cdr::dom::Node();
}

cdr::dom::Node cdr::dom::NamedNodeMap::item(int index) {
    return pNodeMap ? cdr::dom::Node(pNodeMap->item(index)) :
                      cdr::dom::Node();
}

// This is needed, but why?  What's going on here
bool cdr::dom::Parser::initialized = cdr::dom::Parser::doInit();

cdr::dom::Parser::Parser(bool saveMem) : errorHandler(0)
{
    // If we would exceed the max allowable open parsers
    if (nextThreadParser == MAX_PARSERS) {
        domLog("Exceeded max thread parsers - fatal error (!)");
        throw cdr::Exception(L"Exceeded max thread parsers");
    }

    // Create and initialize a new parser
    errorHandler = new cdr::dom::ErrorHandler();
    pXparser = new XERCES_CPP_NAMESPACE::XercesDOMParser();
    pXparser->setErrorHandler(errorHandler);

    // Tells whether to save these pointers in thread static memory
    //  that survives our destructor or not
    savingMem = saveMem;
    if (savingMem) {
        // Save pointer to real parser until we know any generated DOM
        //   tree is no longer needed
        allThreadParsers[nextThreadParser] = pXparser;
        allErrorHandlers[nextThreadParser] = errorHandler;
        ++nextThreadParser;
    }
}

cdr::dom::Parser::~Parser()
{
    // If not saving data beyond the scope of the parse, delete it here
    if (!savingMem) {
        delete errorHandler;
        errorHandler = 0;
        delete pXparser;
        pXparser = 0;
    }
}

void cdr::dom::Parser::deleteAllThreadParsers()
{
// DEBUG
char msg[80];
sprintf(msg, "Deleting %d thread parsers, unsaved=%d", nextThreadParser,
countNonSavedParsers);
domLog(msg);
int errorHandlerErrs = 0;
int threadParserErrs = 0;
if (guard1 !=0) domLog ("Start guard1 violation (!)");
if (guard2 !=0) domLog ("Start guard2 violation (!)");
if (guard3 !=0) domLog ("Start guard3 violation (!)");
if (guard4 !=0) domLog ("Start guard4 violation (!)");

    // Delete zero or more parser objects
    while (nextThreadParser > 0) {
        --nextThreadParser;

        // Sanity checks first
        if (!allErrorHandlers[nextThreadParser]) {
domLog("Null error handler in deleteAllThreadParsers (!)");
            ++errorHandlerErrs;
}
        else {
            delete allErrorHandlers[nextThreadParser];
            allErrorHandlers[nextThreadParser] = 0;
        }

        if (!allThreadParsers[nextThreadParser]) {
domLog("Null threadParser in deleteAllThreadParsers (!)");
            ++threadParserErrs;
}
        else {
            delete allThreadParsers[nextThreadParser];
            allThreadParsers[nextThreadParser] = 0;
        }
    }
if (guard1 !=0) domLog ("End guard1 violation (!)");
if (guard2 !=0) domLog ("End guard2 violation (!)");
if (guard3 !=0) domLog ("End guard3 violation (!)");
if (guard4 !=0) domLog ("End guard4 violation (!)");
domLog("Finished deleting all thread parsers");
if (errorHandlerErrs + threadParserErrs != 0) {
sprintf(msg, "errorHandlerErrs=%d, threadParserErrs=%d", errorHandlerErrs, threadParserErrs);
domLog(msg);
}
}

/**
 * Parse an XML document from an in-memory string object.
 * Throws cdr::dom::DOMException.
 */
void cdr::dom::Parser::parse(const std::string& xml)
    throw(cdr::dom::DOMException)
{
    // Used to track down a bug in xml4c RMK 2000-09-07
    XERCES_CPP_NAMESPACE::MemBufInputSource s(
            (const XMLByte* const)xml.c_str(), xml.size(), "MEM");
    pXparser->parse(s);
}

/**
 * Obsolete version of getTextContent, left in for reference.
 *
 * This old version only gets text content from immediate children
 * of an element.  The new version gets content from the entire
 * tree beneath an element.
 *
 * The Xerces 2.60 version is a Node method.  Our version was an
 * external function.  We have therefore wrapped the new Xerces
 * Node method in an external function of the same name to maintain
 * compatibility with this old version - for which there were 140+
 * uses.
 */
/*
cdr::String cdr::dom::getTextContent(const cdr::dom::Node& node)
{
    cdr::String str;
    cdr::dom::Node child = node.getFirstChild();
    while (child != 0) {
        if (child.getNodeType() == cdr::dom::Node::TEXT_NODE)
            str += cdr::String(child.getNodeValue());
        child = child.getNextSibling();
    }
    return str;
}
*/

cdr::dom::Element::Element(const cdr::dom::Document& doc) {
    pNode = (XERCES_CPP_NAMESPACE::DOMElement *)(doc.getDocPtr());
}

cdr::dom::Node::Node(cdr::dom::Element& elem) {
    pNode = static_cast<XERCES_CPP_NAMESPACE::DOMNode *>
        (elem.getNodePtr());
}

cdr::String cdr::dom::Node::getNodeValue() const
{
    // return (cdr::String(pNode->getNodeValue()));
    if (pNode->getNodeValue() == NULL) {
        return cdr::String(false);
    }
    cdr::String val = cdr::String(pNode->getNodeValue());

    return (cdr::String(pNode->getNodeValue()));
}

cdr::String cdr::dom::getTextContent(const cdr::dom::Node& node) {
    return node.getTextContent();
}

cdr::String cdr::dom::getTextContent(const cdr::dom::Element& elem) {
    return elem.getTextContent();
}

bool cdr::dom::hasText(const Element& elem, const bool excludeWhiteSpace)
{
    cdr::dom::Node node = elem.getFirstChild();
    while (node != NULL) {
        if (node.getNodeType() == cdr::dom::Node::TEXT_NODE) {
            if (excludeWhiteSpace) {
                cdr::String val = node.getNodeValue();
                if (val.find_first_not_of(L" \t\r\n") != val.npos)
                    // Text node is not just white space
                    return true;
            }
            else
                // Text node exists and we don't care if it's whitespace
                return true;
        }

        // Get the next child of the element
        node = node.getNextSibling();
    }

    // Didn't find any text nodes, or any with non-whitespace
    return false;
}


std::wostream& operator<<(std::wostream& os, const cdr::dom::Node& node)
{
    cdr::String name  = node.getNodeName();
    cdr::String value = node.getNodeValue();
    switch (node.getNodeType()) {
        case cdr::dom::Node::TEXT_NODE:
            os << cdr::entConvert(value);
            break;
        case cdr::dom::Node::PROCESSING_INSTRUCTION_NODE:
            os << L"<?" << name << L" " << cdr::entConvert(value) << L"?>";
            break;
        case cdr::dom::Node::DOCUMENT_NODE:
        {
            os << L"<?xml version='1.0' ?>";
            cdr::dom::Node child = node.getFirstChild();
            while (child != 0) {
                os << child;
                child = child.getNextSibling();
            }
            break;
        }
        case cdr::dom::Node::ELEMENT_NODE:
        {
            os << L"<" << name;
            cdr::dom::NamedNodeMap attributes = node.getAttributes();
            int nAttributes = attributes.getLength();
            for (int i = 0; i < nAttributes; ++i) {
                cdr::dom::Node attribute = attributes.item(i);
                cdr::String attrValue =
                    cdr::entConvert(attribute.getNodeValue());
                cdr::String attrName  = attribute.getNodeName();
                size_t quot = attrValue.find(L"\"");
                while (quot != attrValue.npos) {
                    attrValue.replace(quot, 1, L"&quot;");
                    quot = attrValue.find(L"\"", quot);
                }
                os << L" "
                   << attrName
                   << L" = \""
                   << attrValue
                   << L"\"";
            }

            cdr::dom::Node child = node.getFirstChild();
            if (child != 0) {
                os << L">";
                while (child != 0) {
                    os << child;
                    child = child.getNextSibling();
                }
                os << L"</" << name << L">";
            }
            else
                os << L"/>";
            break;
        }
        case cdr::dom::Node::ENTITY_REFERENCE_NODE:
        {
            cdr::dom::Node child = node.getFirstChild();
            while (child != 0) {
                os << child;
                child = child.getNextSibling();
            }
            break;
        }
        case cdr::dom::Node::CDATA_SECTION_NODE:
            os << L"<![CDATA[" << value << L"]]>";
            break;
        case cdr::dom::Node::COMMENT_NODE:
            os << L"<!--" << value << L"-->";
            break;
        default:
        {
            wchar_t err[80];
            swprintf(err, L"Unrecognized node type: %d", node.getNodeType());
            throw cdr::Exception(err);
        }
    }
    return os;
}

#include <stdio.h>
#include <stdlib.h>

// Log messages, no action if fails
static FILE *fp = 0;
void domLog(const char *msg1) {
    if (!fp)
        fp = fopen("d:\\cdr\\Log\\DomLog", "a");
    if (fp) {
        fprintf (fp, "=(%d) %s\n", GetCurrentThreadId(), msg1);
        fflush(fp);
    }
}
void domLog(const char *msg1, const cdr::String& msg2) {
    if (!fp)
        fp = fopen("d:\\cdr\\Log\\DomLog", "a");
    if (fp) {
        fprintf (fp, "=(%d) %s \"%s\"\n", GetCurrentThreadId(),
                 msg1, msg2.toUtf8().c_str());
        fflush(fp);
    }
}
