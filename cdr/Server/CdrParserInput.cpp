/*
 * $Id: CdrParserInput.cpp,v 1.1 2000-04-21 13:59:43 bkline Exp $
 *
 * Implementation of wrapper for input to CDR string parsers.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrParserInput.h"

const wchar_t* cdr::ParserInput::extractString(wchar_t delim)
{
    size_t n = 0;
    const wchar_t* start = buf + pos;
    const wchar_t* p = start;
    wchar_t c;
    while ((c = *p++) != 0) {
        if (c == delim) {
            pos += n + 1;
            extractedStrings.push_back(cdr::String(start, n));
            return extractedStrings.back().c_str();
        }
        else if (c == '\\') {
            if (*p) {
                ++p;
                ++n;
            }
        }
        ++n;
    }
    throw cdr::Exception(L"ParserInput::extractString: missing delimiter");
}

const wchar_t* cdr::ParserInput::extractString(size_t count)
{
    if (pos + count > len)
        throw cdr::Exception(L"ParserInput::extractString: past EOF");
    const wchar_t* p = buf + pos;
    pos += count;
    extractedStrings.push_back(cdr::String(p, count));
    return extractedStrings.back().c_str();
}
