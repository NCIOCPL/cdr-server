/*
 * $Id: CdrString.h,v 1.1 2000-04-11 14:18:24 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_STRING_
#define CDR_STRING_

// System headers.
#include <string>
#include <set>
#include <vector>

// IBM DOM implementation.
#include <dom/DOMString.hpp>

namespace cdr {

    // The typedef is required to work around MSVC++ bugs.
    typedef std::wstring StdWstring;

    /**
     * Wrapper class for wide-character string.  Used to store Unicode
     * as UTF-16.
     */
    class String : public StdWstring {
    public:
        String() {}
        String(const wchar_t* s) : StdWstring(s) {}
        String(const DOMString& s) : StdWstring(s.rawBuffer(), s.length()) {}
        String(const StdWstring& s) : StdWstring(s) {}
        int getInt() const;
        std::string toUtf8() const;
    };

    // Containers of our strings.
    typedef std::set<String>             StringSet;
    typedef std::vector<String>          StringVector;
}

#endif
