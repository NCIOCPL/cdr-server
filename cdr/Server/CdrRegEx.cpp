/*
 * $Id: CdrRegEx.cpp,v 1.1 2000-05-03 15:19:14 bkline Exp $
 *
 * Implementation of CDR wrapper for regular expression handling.
 *
 * $Log: not supported by cvs2svn $
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
    unsigned flags = jm::regbase::normal | jm::regbase::use_except;
    if (caseFlag)
        flags |= jm::regbase::icase;
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
    unsigned flags = jm::regbase::normal | jm::regbase::use_except;
    if (caseFlag)
        flags |= jm::regbase::icase;
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
    jm::wcmatch m;
    return jm::query_match(what, m, *this);
}

bool cdr::RegEx::match(const cdr::String& what)
{
    jm::wcmatch m;
    return jm::query_match(what, m, *this);
}

bool cdr::RegEx::match(const char* what)
{
    return match(cdr::String(what));
}

bool cdr::RegEx::match(const std::string& what)
{
    return match(cdr::String(what));
}
