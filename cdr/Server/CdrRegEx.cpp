/*
 * $Id: CdrRegEx.cpp,v 1.4 2002-05-01 01:07:13 bkline Exp $
 *
 * Implementation of CDR wrapper for regular expression handling.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2002/03/07 18:15:53  bkline
 * Update for new version of regex package.
 *
 * Revision 1.2  2002/03/03 14:43:34  bkline
 * Eliminated incompatibility with MSC++ 7.0.
 *
 * Revision 1.1  2000/05/03 15:19:14  bkline
 * Initial revision
 *
 */

#ifndef UNICODE
#define UNICODE
#endif

#include "CdrRegEx.h"
#include "CdrException.h"

cdr::RegEx::RegEx(const wchar_t* pattern, bool ignore) : caseFlag(ignore)
{
    set(pattern);
}

cdr::RegEx::RegEx(const cdr::String& pattern, bool ignore) : caseFlag(ignore)
{
    set(pattern);
}

cdr::RegEx::RegEx(const char* pattern, bool ignore) : caseFlag(ignore)
{
    set(pattern);
}

cdr::RegEx::RegEx(const std::string& pattern, bool ignore) : caseFlag(ignore)
{
    set(pattern);
}

cdr::String cdr::RegEx::getPattern() const
{
    return expression();
}

void cdr::RegEx::set(const wchar_t* pattern)
{
    unsigned flags = boost::regbase::normal | boost::regbase::use_except;
    if (caseFlag)
        flags |= boost::regbase::icase;
    try {
        set_expression(pattern, flags);
    }
    catch (const std::exception& e) {
        cdr::String err = cdr::String(L"Regular expression error in '")
                        + pattern
                        + L"'";
        throw cdr::Exception(err, cdr::String(e.what()).c_str());
    }
}

void cdr::RegEx::set(const cdr::String& pattern)
{
    unsigned flags = boost::regbase::normal | boost::regbase::use_except;
    if (caseFlag)
        flags |= boost::regbase::icase;
    try {
        set_expression(pattern, flags);
    }
    catch (const std::exception& e) {
        cdr::String err = cdr::String(L"Regular expression error in '")
                        + pattern
                        + L"'";
        throw cdr::Exception(err, cdr::String(e.what()).c_str());
    }
}

void cdr::RegEx::set(const char* pattern)
{
    set(cdr::String(pattern));
}

void cdr::RegEx::set(const std::string& pattern)
{
    set(cdr::String(pattern));
}

bool cdr::RegEx::match(const wchar_t* what)
{
    boost::wcmatch m;
    return boost::regex_match(what, m, *this);
}

bool cdr::RegEx::match(const cdr::String& what)
{
    boost::wcmatch m;
    //boost::regex_match<std::basic_string<wchar_t>::const_iterator,
    //              boost::wregex::alloc_type> m;
    return boost::regex_match(what.c_str(), m, *this);
}

bool cdr::RegEx::match(const char* what)
{
    return match(cdr::String(what));
}

bool cdr::RegEx::match(const std::string& what)
{
    return match(cdr::String(what));
}
