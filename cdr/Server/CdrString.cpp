/*
 * $Id: CdrString.cpp,v 1.3 2000-04-26 01:28:01 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/11 17:49:54  bkline
 * Added constructor for converting from UTF-8.
 *
 * Revision 1.1  2000/04/11 14:16:13  bkline
 * Initial revision
 *
 */

#include <sstream>
#include "CdrString.h"

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
 * Skips past the 'CDR' prefix and extracts the integer id for the
 * document.
 */
int cdr::String::extractDocId() const
{
    int i = 0;
    while (i < size() && !isdigit((*this)[i]))
        ++i;
    cdr::String numString = substr(i);
    return numString.getInt();
}
