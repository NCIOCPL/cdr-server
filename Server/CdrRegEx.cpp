/*
 * $Id$
 *
 * Implementation of CDR wrapper for regular expression handling.
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

void cdr::RegEx::set(const wchar_t* pattern)
{
    try {
        if (caseFlag)
            assign(pattern, std::regex_constants::icase);
        else
            assign(pattern);
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
    try {
        if (caseFlag)
            assign(pattern, std::regex_constants::icase);
        else
            assign(pattern);
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
    return regex_match(what, *this);
}

bool cdr::RegEx::match(const cdr::String& what)
{
    return regex_match(what, *this);
}

bool cdr::RegEx::match(const char* what)
{
    return match(cdr::String(what));
}

bool cdr::RegEx::match(const std::string& what)
{
    return match(cdr::String(what));
}
