/*
 * $Id: CdrDom.cpp,v 1.1 2000-04-11 14:16:41 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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
