/*
 * $Id: CdrString.h,v 1.14 2000-07-11 22:42:03 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2000/06/09 04:01:22  ameyer
 * Added toString template function.
 *
 * Revision 1.12  2000/05/16 21:19:26  bkline
 * Added packErrors() function.
 *
 * Revision 1.11  2000/05/09 19:21:59  mruben
 * added operator+= and operator+
 *
 * Revision 1.10  2000/05/04 12:39:43  bkline
 * More ccdoc comments.
 *
 * Revision 1.9  2000/05/03 15:42:31  bkline
 * Added greater-than and less-than comparison operators.  Added getFloat()
 * method.
 *
 * Revision 1.8  2000/04/26 01:40:12  bkline
 * Added copy constructor, != operator, and extractDocId() method.
 *
 * Revision 1.7  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.6  2000/04/22 18:01:26  bkline
 * Fleshed out documentation comments.
 *
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
#include <sstream>
#include <set>
#include <vector>
#include <list>

// IBM DOM implementation.
#include <dom/DOMString.hpp>

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

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
         *
         *  @param  n           <code>true</code> if the object represents
         *                      a NULL value; defaults to <code>false</code>.
         */
        String(bool n = false) : null(n) {}

        /**
         * Creates a non-NULL <code>String</code> object from a
         * null-terminated buffer of wide characters.
         *
         *  @param  s           address of null-terminated array of characters
         *                      representing a string encoded as UTF-16.
         */
        String(const wchar_t* s) : StdWstring(s), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from at most the
         * first <code>n</code> wide characters in buffer <code>s</code>.
         *
         *  @param  s           address of array of characters representing
         *                      a string encoded as UTF-16.
         *  @param  n           limit to the number of characters in the
         *                      array to be used in constructing the new
         *                      object.
         */
        String(const wchar_t* s, size_t n) : StdWstring(s, n), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a standard
         * <code>wstring</code>.
         *
         *  @param  s           reference to standard string object to be
         *                      copied.
         */
        String(const StdWstring& s) : StdWstring(s), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a
         * null-terminated buffer of characters encoded using
         * <code>UTF-8</code>.
         *
         *  @param  s           address of null-terminated array of characters
         *                      representing a string encoded as UTF-8.
         */
        String(const char* s) : null(false) { utf8ToUtf16(s); }

        /**
         * Creates a non-NULL <code>String</code> object from a standard
         * <code>string</code> encoded using <code>UTF-8</code>.
         *
         *  @param  s           reference to standard <code>string</code>
         *                      object containing UTF-8 encoded value for
         *                      object.
         */
        String(const std::string& s) : null(false) { utf8ToUtf16(s.c_str()); }

        /**
         * Initializes a new non-NULL <code>String</code> object consisting of
         * <code>n</code> repetitions of wide character <code>c</code>.
         * Useful for creating a <code>String</code> which is known in advance
         * to require at least <code>n</code> characters, but whose contents
         * must be determined after creation of the object.
         *
         *  @param  n           size of new object (in characters).
         *  @param  c           character to be used to initialize the object.
         */
        String(size_t n, wchar_t c) : StdWstring(n, c), null(false) {}

        /**
         * Creates a non-NULL <code>String</code> object from a string used by
         * the DOM interface.
         *
         *  @param  s           reference to DOM string to be copied.
         */
        String(const DOMString& s)
            : StdWstring(s.rawBuffer(), s.length()), null(false) {}

        /**
         * Copy constructor.
         *
         *  @param  s           reference to object to be copied.
         */
        String(const String& s) : StdWstring(s), null(s.null) {}

        /**
         * Concatentate something else to this cdr::String
         *
         *  @param  s           const reference to obejct to be concatentated
         *
         *  @return             reference to this cdr::String
         */
        String& operator+=(const String& s)
            { *this = *this + s; return *this; }
        String& operator+=(const wchar_t* s)
            { *this = *this + s; return *this; }
        String& operator+=(wchar_t s)
            { *this = *this + s; return *this; }

        /**
         * Compares another cdr::String to this one.  Case is significant for
         * the purposes of this comparison.
         *
         *  @param  s           reference to object to be compared with this
         *                      object.
         *  @return             <code>true</code> iff the contents of this
         *                      object differ from those of <code>s</code>.
         */
        bool operator!=(const String& s) const
            { return *this != static_cast<const StdWstring&>(s); }

        /**
         * Compares another cdr::String to this one.  Case is significant for
         * the purposes of this comparison.
         *
         *  @param  s           reference to object to be compared with this
         *                      object.
         *  @return             <code>true</code> iff the contents of this
         *                      object sort lexically before those of
         *                      <code>s</code>.
         */
        bool operator<(const String& s) const
            { return *this < static_cast<const StdWstring&>(s); }

        /**
         * Compares another cdr::String to this one.  Case is significant for
         * the purposes of this comparison.
         *
         *  @param  s           reference to object to be compared with this
         *                      object.
         *  @return             <code>true</code> iff the contents of this
         *                      object sort lexically after those of
         *                      <code>s</code>.
         */
        bool operator>(const String& s) const
            { return *this > static_cast<const StdWstring&>(s); }

        /**
         * Accessor method for determining whether this object is NULL.
         *
         *  @return             <code>true</code> iff the object represents
         *                      a <code>NULL</code> string.
         */
        bool isNull() const { return null; }

        /**
         * Parses the integer value represented by the string value of the
         * object.
         *
         *  @return             integer value represented by the string
         *                      value of the object; <code>0</code> if
         *                      the value of the object does not represent
         *                      a number.
         */
        int getInt() const;

        /**
         * Parses the floating-point value represented by the string value of
         * the object.
         *
         *  @return             floating-point value represented by the string
         *                      value of the object; <code>0.0</code> if
         *                      the value of the object does not represent
         *                      a number.
         */
        double getFloat() const;

        /**
         * Template function to convert any object for which a stringstream
         * conversion exists to a String.
         */
        template <class T> String static toString (const T& in)
        {
            std::wostringstream os;
            os << in;
            return os.str();
        }

        /**
         * Creates a standard <code>string</code> copy of the value of the
         * object, encoded using <code>UTF-8</code>.
         *
         *  @return             new <code>string</code> object containing
         *                      UTF-8 encoding of the object's value.
         */
        std::string toUtf8() const;

        /**
         * Skips past the 'CDR' prefix and extracts the integer id for the
         * document.
         *
         *  @return             integer for the document's primary key.
         *  @exception          <code>cdr::Exception</code> if the first
         *                      characters of the string are not "CDR".
         */
        int extractDocId() const;

    private:
        void utf8ToUtf16(const char*);
        bool null;
    };

    /**
     * Concatenate two Strings
     */
    inline String operator+(String s, const String& t)
    {
      return s += t;
    }

    /**
     * Containers of <code>String</code> objects.  Typedefs given here for
     * convenience, and as workarounds for MSVC++ template bugs.
     */
    typedef std::set<String>             StringSet;
    typedef std::vector<String>          StringVector;
    typedef std::list<String>            StringList;

    /**
     * Convert an integer to a CDR doc id, with 'CDR' prefix and
     * zero padding.
     *
     *  @param  id          integer form of a document id.
     *  @return             string form of the doc id.
     */
    extern String stringDocId (const int);

    /**
     * Packs the error messages contained in the caller's list into a
     * single string suitable for embedding within the command response.
     *
     *  @param  errors      list of strings containing error information
     *                      gathered by the command processor.
     *  @return             marked-up &lt;errors&gt; element containing
     *                      one &lt;err&gt; element for each member of
     *                      the <code>errors</code> list.
     */
    extern String packErrors(const StringList& errors);
}

#endif
