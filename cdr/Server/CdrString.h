/*
 * $Id: CdrString.h,v 1.6 2000-04-22 18:01:26 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2000/04/21 14:01:17  bkline
 * Added String(size_t, wchar_t) constructor.
 *
 * Revision 1.4  2000/04/19 18:29:52  bkline
 * Added StringList typedef.
 *
 * Revision 1.3  2000/04/17 21:28:01  bkline
 * Made cdr::String nullable.
 *
 * Revision 1.2  2000/04/14 16:02:00  bkline
 * Added support for converting UTF-8 to UTF-16.
 *
 * Revision 1.1  2000/04/11 14:18:24  bkline
 * Initial revision
 */

#ifndef CDR_STRING_
#define CDR_STRING_

// System headers.
#include <string>
#include <set>
#include <vector>
#include <list>

// IBM DOM implementation.
#include <dom/DOMString.hpp>

namespace cdr {

    /**
     * This typedef is required to work around MSVC++ bugs in the handling of
     * parameterized types.
     */
    typedef std::wstring StdWstring;

    /**
     * Wrapper class for wide-character string.  Used to store Unicode
     * as UTF-16.
     */
    class String : public StdWstring {
    public:

        /**
         * Creates an empty string.  The object represents a <code>NULL</code>
         * string if the optional argument is set to <code>true</code>.
         */
        String(bool n = false) : null(n) {}

        /**
         * Creates a non-NULL <code>String</code> object from a
         * null-terminated buffer of wide characters.
         */
        String(const wchar_t* s) : StdWstring(s), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from at most the
         * first <code>n</code> wide characters in buffer <code>s</code>.
         */
        String(const wchar_t* s, size_t n) : StdWstring(s, n), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a standard
         * <code>wstring</code>.
         */
        String(const StdWstring& s) : StdWstring(s), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a
         * null-terminated buffer of characters encoded using
         * <code>UTF-8</code>.
         */
        String(const char* s) : null(false) { utf8ToUtf16(s); }

        /**
         * Creates a non-NULL <code>String</code> object from a standard
         * <code>string</code> encoded using <code>UTF-8</code>.
         */
        String(const std::string& s) : null(false) { utf8ToUtf16(s.c_str()); }

        /**
         * Initializes a new non-NULL <code>String</code> object consisting of
         * <code>n</code> repetitions of wide character <code>c</code>.
         * Useful for creating a <code>String</code> which is known in advance
         * to require at least <code>n</code> characters, but whose contents
         * must be determined after creation of the object.
         */
        String(size_t n, wchar_t c) : StdWstring(n, c), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a string used by
         * the DOM interface.
         */
        String(const DOMString& s) 
            : StdWstring(s.rawBuffer(), s.length()), null(false) {}

        /**
         * Returns <code>true</code> if the object represents a
         * <code>NULL</code> string.
         */
        bool isNull() const { return null; }

        /**
         * Parses the integer value represented by the string value of the
         * object.
         */
        int getInt() const;

        /**
         * Creates a standard <code>string</code> copy of the value of the
         * object, encoded using <code>UTF-8</code>.
         */
        std::string toUtf8() const;
    private:
        void utf8ToUtf16(const char*);
        bool null;
    };

    /**
     * Containers of <code>String</code> objects.  Typedefs given here for
     * convenience.
     */
    typedef std::set<String>             StringSet;
    typedef std::vector<String>          StringVector;
    typedef std::list<String>            StringList;
}

#endif
