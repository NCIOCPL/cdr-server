/*
 * $Id: CdrDom.cpp,v 1.2 2000-04-15 14:06:39 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/11 14:16:41  bkline
 * Initial revision
 *
 */

#include "CdrDom.h"
#include <framework/MemBufInputSource.hpp>

bool cdr::dom::Parser::initialized = cdr::dom::Parser::doInit();

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
