/*
 * $Id: CdrDom.cpp,v 1.4 2000-05-09 21:07:48 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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
            void warning(const SAXParseException& e) {
                // Substitute logging when in place.
                cdr::String err = cdr::String(e.getMessage());
                std::wcerr << L"*** DOM WARNING: " << err << std::endl;
            }
            void error(const SAXParseException& e) {
                throw cdr::Exception(L"DOM ERROR", cdr::String(e.getMessage()));
            }
            void fatalError(const SAXParseException& e) {
                throw cdr::Exception(L"FATAL DOM ERROR",
                                     cdr::String(e.getMessage()));
            }
            void resetErrors() {}
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
    MemBufInputSource s((const XMLByte* const)xml.c_str(), xml.size(), "MEM");
    ((::DOMParser *)this)->parse(s);
}

void cdr::dom::Parser::parse(const cdr::String& xml)
    throw(cdr::dom::DOMException)
{
    parse(xml.toUtf8());
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
