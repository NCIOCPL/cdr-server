/*
 * $Id: CdrString.cpp,v 1.1 2000-04-11 14:16:13 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
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
 * Extracts integer value from wide string.
 */
int cdr::String::getInt() const
{
    std::wistringstream is(*this);
    int i;
    is >> i;
    return i;
}
