/*
 * $Id: CdrString.cpp,v 1.15 2001-05-16 20:39:01 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.14  2001/04/05 22:34:02  ameyer
 * Added ynCheck() utility.
 *
 * Revision 1.13  2001/03/02 13:59:26  bkline
 * Replaced body of cdr::entConvert() with more efficient implementation.
 *
 * Revision 1.12  2001/03/01 23:28:38  ameyer
 * Fixed bug, not updating s_first_time.
 *
 * Revision 1.11  2000/10/25 19:06:01  mruben
 * added functions to put dates in correct format
 *
 * Revision 1.10  2000/10/18 03:19:15  ameyer
 * Added optional parameter to tagWrap allowing attributes to be included.
 *
 * Revision 1.9  2000/08/15 20:16:18  ameyer
 * Added entConvert function for converting "<>&" and others if need.
 *
 * Revision 1.8  2000/07/21 21:07:19  ameyer
 * Added tagWrap().
 *
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
#include <iostream>
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

    // Populate string.
    for (i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)*s;
        if (ch < 0x80) {
            (*this)[i] = (wchar_t)ch;
            ++s;
        }
        else if ((ch & 0xE0) == 0xC0) {
            (*this)[i] = ((ch & 0x1F) << 6)
                         | (((unsigned char)s[1]) & 0x3F);
            s += 2;
        }
        else {
            (*this)[i] = ((ch & 0x0F) << 12)
                         | ((((unsigned char)s[1]) & 0x3F) << 6)
                         | (((unsigned char)s[2]) & 0x3F);
            s += 3;
        }
    }
}


/**
 * Convert reserved XML characters to character entities in a string.
 *
 *  @param  inStr           reference to original string, possibly containing
 *                          reserved XML characters, such as & or <.
 *  @return                 copy of string with reserved characters replaced
 *                          as appropriate; note that in the common case in
 *                          which no replacements are needed the runtime
 *                          library will optimize away the copy of the
 *                          original string using reference counting.
 */

cdr::String cdr::entConvert (
    const cdr::String& inStr
) {
    // Ampersand MUST be first element in this table!
    static struct { wchar_t ch; wchar_t* ent; } eTable[] = {
        { L'&', L"&amp;" },
        { L'<', L"&lt;"  },
        { L'>', L"&gt;"  }
    };

    // If we make no changes no memory allocations or copying will happen.
    cdr::String outStr = inStr;
    if (outStr.isNull())
        return outStr;

    // Replace each reserved character with its entity equivalent.
    for (int i = 0; i < sizeof eTable / sizeof *eTable; ++i) {
        wchar_t                ch  = eTable[i].ch;
        wchar_t*               ent = eTable[i].ent;
        cdr::String::size_type len = wcslen(ent);
        cdr::String::size_type pos = outStr.find(ch);
        while (pos != outStr.npos) {
            outStr.replace(pos, 1, ent);
            pos = outStr.find(ch, pos + len);
        }
    }

    return outStr;
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
cdr::String cdr::tagWrap (const cdr::String& data, const cdr::String& tag,
                          const cdr::String& attrs)
{
    cdr::String elem = L"<" + tag;
    if (attrs.size() > 0)
        elem += L" " + attrs;
    elem += L">" + data + L"</" + tag + L">";
    return elem;
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

/**
 * Puts date in database format (space separating date and time)
 */
cdr::String cdr::toDbDate(cdr::String date)
{
  if (date.length() > 10 && date[10] == L'T')
    date[10] = L' ';

  return date;
}

/**
 * Puts date in XML schema format ('T' separating date and time)
 */
cdr::String cdr::toXmlDate(cdr::String date)
{
  if (date.length() > 10 && date[10] == L' ')
    date[10] = L'T';

  return date;
}

/**
 * Determine if a string contains "Y" or "N" or "y" or "n".
 */

bool cdr::ynCheck (cdr::String ynString, bool defaultVal, cdr::String forceMsg)
{
    // Check
    if (ynString == L"Y" || ynString == L"y")
        return true;
    if (ynString == L"N" || ynString == L"n")
        return false;

    // Expected values not found, throw exception or return default
    if (forceMsg != L"")
        throw cdr::Exception (L"Must specify 'Y' or 'N' in " + forceMsg);

    return defaultVal;
}
