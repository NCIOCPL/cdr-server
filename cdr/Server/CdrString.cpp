/*
 * $Id: CdrString.cpp,v 1.8 2000-07-21 21:07:19 ameyer Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.7  2000/07/11 22:41:24  ameyer
 * Added stringDocId() helper function.
 *
 * Revision 1.6  2000/05/16 21:19:09  bkline
 * Added packErrors() method.
 *
 * Revision 1.5  2000/05/04 12:44:27  bkline
 * Changed method for extracting a document ID to throw an exception if
 * the string does match the expected pattern for a docId.
 *
 * Revision 1.4  2000/05/03 15:20:00  bkline
 * Added getFloat() method.
 *
 * Revision 1.3  2000/04/26 01:28:01  bkline
 * Added extractDocId() method.
 *
 * Revision 1.2  2000/04/11 17:49:54  bkline
 * Added constructor for converting from UTF-8.
 *
 * Revision 1.1  2000/04/11 14:16:13  bkline
 * Initial revision
 *
 */

#include <sstream>
#include "CdrString.h"
#include "CdrRegEx.h"
#include "CdrException.h"

/**
 * Creates UTF-8 version of string.  Ignores UCS code points beyond 0xFFFF.
 */
std::string cdr::String::toUtf8() const
{
    // Calculate storage requirement.
    size_t i, len = 0;
    const wchar_t* wchars = c_str();
    for (i = 0; i < size(); ++i) {
        if ((unsigned short)wchars[i] < 0x80)
            ++len;
        else if ((unsigned short)wchars[i] < 0x800)
            len += 2;
        else
            len += 3;
    }

    // Create string.
    std::string utf8(len, ' ');
    size_t j;

    // Populate string.
    for (i = j = 0; i < size(); ++i) {
        unsigned short ch = (unsigned short)*wchars++;
        if (ch < 0x80)
            utf8[j++] = (char)(unsigned char)ch;
        else if (ch < 0x800) {
            utf8[j++] = (char)(unsigned char)(0xC0 | ((ch & 0x07C0) >> 6));
            utf8[j++] = (char)(unsigned char)(0x80 |  (ch & 0x003F));
        }
        else {
            utf8[j++] = (char)(unsigned char)(0xE0 | ((ch & 0xF000) >> 12));
            utf8[j++] = (char)(unsigned char)(0x80 | ((ch & 0x0FC0) >> 6));
            utf8[j++] = (char)(unsigned char)(0x80 |  (ch & 0x003F));
        }
    }
    return utf8;
}

/**
 * Converts string from UTF-8 to UTF-16 (common support for two
 * constructors).  Ignores values beyond U+FFFF.
 */
void cdr::String::utf8ToUtf16(const char* s)
{
    // Calculate storage requirement.
    size_t i, len = 0;
    for (i = 0; s[i]; ++i) {
        if (((unsigned char)s[i] & 0x80) == 0)
            ++len;
        else if (((unsigned char)s[i] & 0x40) == 0x40)
            ++len;
    }

    // Make room.
    resize(len);
    size_t j;

    // Populate string.
    for (i = j = 0; i < size(); ++i) {
        unsigned char ch = (unsigned char)*s;
        if (ch < 0x80) {
            (*this)[j++] = (wchar_t)ch;
            ++s;
        }
        else if ((ch & 0xE0) == 0xC0) {
            (*this)[j++] = ((ch & 0x1F) << 6)
                         | (((unsigned char)s[1]) & 0x3F);
            s += 2;
        }
        else {
            (*this)[j++] = ((ch & 0x0F) << 12)
                         | ((((unsigned char)s[1]) & 0x3F) << 6)
                         | (((unsigned char)s[2]) & 0x3F);
            s += 3;
        }
    }
}

/**
 * Extracts integer value from wide string.
 */
int cdr::String::getInt() const
{
    std::wistringstream is(*this);
    int i;
    is >> i;
    return i;
}

/**
 * Extracts floating-point value from wide string.
 */
double cdr::String::getFloat() const
{
    std::wistringstream is(*this);
    double d;
    is >> d;
    return d;
}

/**
 * Skips past the 'CDR' prefix and extracts the integer id for the
 * document.
 */
int cdr::String::extractDocId() const
{
    cdr::RegEx pattern(L"CDR\\d+");
    if (!pattern.match(*this))
        throw cdr::Exception(L"Invalid document ID string", *this);
    cdr::String numString = substr(3);
    return numString.getInt();
}

/**
 * Convert an integer to a CDR doc id, with 'CDR' prefix and
 * zero padding.
 */
cdr::String cdr::stringDocId(const int id)
{
    wchar_t buf[20];

    swprintf (buf, L"CDR%010d", id);
    cdr::String s(buf);
    return s;
}

/**
 * Wrap an xml tag around a string, returning the wrapped string.
 */
cdr::String cdr::tagWrap (const cdr::String& data, const cdr::String& tag)
{
    return (L"<" + tag + L">" + data + L"</" + tag + L">");
}


/**
 * Packs the error messages contained in the caller's list into a single
 * string suitable for embedding within the command response.
 */
cdr::String cdr::packErrors(const cdr::StringList& errors)
{
    cdr::String s = L"   <Errors>\n";
    cdr::StringList::const_iterator i = errors.begin();
    while (i != errors.end())
        s += L"    <Err>" + *i++ + L"</Err>\n";
    s += L"   </Errors>\n";
    return s;
}
