/*
 * $Id: CdrString.h,v 1.3 2000-04-17 21:28:01 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/14 16:02:00  bkline
 * Added support for converting UTF-8 to UTF-16.
 *
 * Revision 1.1  2000/04/11 14:18:24  bkline
 * Initial revision
 *
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
        String(bool n = false) : null(n) {}
        String(const wchar_t* s) : StdWstring(s), null(false) {}
        String(const wchar_t* s, size_t n) : StdWstring(s, n), null(false) {}
        String(const StdWstring& s) : StdWstring(s), null(false) {}
        String(const char* s) : null(false) { utf8ToUtf16(s); }
        String(const std::string& s) : null(false) { utf8ToUtf16(s.c_str()); }
        String(const DOMString& s) 
            : StdWstring(s.rawBuffer(), s.length()), null(false) {}
        bool isNull() const { return null; }
        int getInt() const;
        std::string toUtf8() const;
    private:
        void utf8ToUtf16(const char*);
        bool null;
    };

    // Containers of our strings.
    typedef std::set<String>             StringSet;
    typedef std::vector<String>          StringVector;
}

#endif
