/*
 * $Id: CdrDom.cpp,v 1.7 2001-10-17 13:51:28 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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
#include <framework/MemBufInputSource.hpp>
#include <sax/SAXParseException.hpp>

namespace cdr {
    namespace dom {
        class ErrorHandler : public ::ErrorHandler {
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

bool cdr::dom::Parser::initialized = cdr::dom::Parser::doInit();

cdr::dom::Parser::Parser() : errorHandler(0)
{
    errorHandler = new cdr::dom::ErrorHandler();
    setErrorHandler(errorHandler);
}

cdr::dom::Parser::~Parser()
{
    delete errorHandler;
    errorHandler = 0;
}

/**
 * Parse an XML document from an in-memory string object.
 * Throws cdr::dom::DOMException.
 */
void cdr::dom::Parser::parse(const std::string& xml)
    throw(cdr::dom::DOMException)
{
    // Used to track down a bug in xml4c RMK 2000-09-07
    //std::cerr << "XML=[" << xml << "]" << std::endl;
    MemBufInputSource s((const XMLByte* const)xml.c_str(), xml.size(), "MEM");
    ((::DOMParser*)this)->parse(s);
}

void cdr::dom::Parser::parse(const cdr::String& xml)
    throw(cdr::dom::DOMException)
{
    parse(xml.toUtf8());
}

void cdr::dom::Parser::parseFile(const char* fileName)
    throw(cdr::dom::DOMException)
{
    ((::DOMParser*)this)->parse(fileName);
}

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

std::wostream& operator<<(std::wostream& os, const cdr::dom::Node& node)
{
    cdr::String name  = node.getNodeName();
    cdr::String value = node.getNodeValue();
    switch (node.getNodeType()) {
        case cdr::dom::Node::TEXT_NODE:
            os << value;
            break;
        case cdr::dom::Node::PROCESSING_INSTRUCTION_NODE:
            os << L"<?" << name << L" " << value << L"?>";
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
                cdr::String attrValue = attribute.getNodeValue();
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
