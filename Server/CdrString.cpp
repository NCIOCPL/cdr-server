/*
 * $Id: CdrString.cpp,v 1.26 2006-11-12 13:49:34 bkline Exp $
 *
 * BZIssue::4767
 */

#include <sstream>
#include <iostream>
#include <cwchar>
#include "CdrString.h"
#include "CdrBlob.h"
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
 * Return upper case version of string.
 */
cdr::String cdr::String::toUpperCase() const {

    // Create output buffer
    wchar_t *outChars = new wchar_t[size() + 1];

    // Copy chars
    const wchar_t *srcp  = c_str();
    wchar_t       *destp = outChars;
    while (*srcp) {
        if (iswlower(*srcp))
            *destp = towupper(*srcp);
        else
            *destp = *srcp;
        ++srcp;
        ++destp;
    }

    // C string terminator
    *destp = 0;

    // Convert to String for return
    cdr::String outStr = cdr::String(outChars);

    delete outChars;
    return outStr;
}

/**
 * Return lower case version of string.
 */
cdr::String cdr::String::toLowerCase() const {

    // Same as toUpperCase
    wchar_t       *outChars = new wchar_t[size() + 1];
    const wchar_t *srcp     = c_str();
    wchar_t       *destp    = outChars;
    while (*srcp) {
        if (iswupper(*srcp))
            *destp = towlower(*srcp);
        else
            *destp = *srcp;
        ++srcp;
        ++destp;
    }

    *destp = 0;
    cdr::String outStr = cdr::String(outChars);
    delete outChars;
    return outStr;
}

/**
 * Convert reserved XML characters to character entities in a string.
 *
 *  @param  inStr           reference to original string, possibly containing
 *                          reserved XML characters, such as & or <.
 *  @param  doQuotes        if true, also replace " with &quot; entity
 *                          and ' with &apos; entity.
 *  @return                 copy of string with reserved characters replaced
 *                          as appropriate; note that in the common case in
 *                          which no replacements are needed the runtime
 *                          library will optimize away the copy of the
 *                          original string using reference counting.
 */
cdr::String cdr::entConvert(const String& inStr, bool doQuotes)
{
    // Ampersand MUST be first element in this table!
    static struct { wchar_t ch; wchar_t* ent; } eTable[] = {
        { L'&',  L"&amp;"  },
        { L'<',  L"&lt;"   },
        { L'>',  L"&gt;"   },
        { L'"',  L"&quot;" },
        { L'\'', L"&apos;" }
    };
    int numConversions = doQuotes ? 5 : 3;

    // If we make no changes no memory allocations or copying will happen.
    String outStr = inStr;
    if (outStr.isNull())
        return outStr;

    // Replace each reserved character with its entity equivalent.
    for (int i = 0; i < numConversions; ++i) {
        wchar_t                ch  = eTable[i].ch;
        wchar_t*               ent = eTable[i].ent;
        String::size_type len = wcslen(ent);
        String::size_type pos = outStr.find(ch);
        while (pos != outStr.npos) {
            outStr.replace(pos, 1, ent);
            pos = outStr.find(ch, pos + len);
        }
    }

    return outStr;
}


/**
 * Normalize all whitespace characters in a string to single space.
 */
cdr::String cdr::normalizeWhiteSpace (
    const cdr::String& inStr
) {
    const wchar_t *srcp;            // Pointer into source string
    wchar_t       *destp,           // Pointer into dest buffer
                  *bufp;            // Buffer for output
    bool          inWhite=false;    // True=in run of whitespace

    // Output buffer
    destp = bufp = new wchar_t[inStr.size() + 1];

    // Copy from source to dest
    srcp = inStr.c_str();
    while (*srcp) {
        // Non-whites are copied
        if (!iswspace (*srcp)) {
            *destp++ = *srcp;
            inWhite = false;
        }
        // First whitespace is copied as space, rest are ignored
        else if (!inWhite) {
            *destp++ = (wchar_t) ' ';
            inWhite = true;
        }
        ++srcp;
    }
    *destp = (wchar_t) '\0';

    // Returnable string
    cdr::String outStr = (cdr::String) bufp;
    delete[] bufp;

    return outStr;
}


/**
 * Trim leading and trailing whitespace.
 */
cdr::String cdr::trimWhiteSpace (
    const cdr::String& inStr,
    const bool leading,
    const bool trailing
) {
    const wchar_t *srcp,            // Pointer to start of source string
                  *osrcp,           // Original, prior to trim
                  *endp,            // Pointer to end of source
                  *oendp;           // Original, prior to trim
    wchar_t       *destp,           // Pointer into dest buffer
                  *bufp;            // Buffer for output


    // Point to start and end (last non-null char) of source string
    srcp = osrcp = inStr.c_str();
    endp = oendp = srcp + wcslen(srcp) - 1;

    // If we trim leading ws, point past initial whitespace
    if (leading) {
        while (*srcp) {
            if (!iswspace(*srcp))
                break;
            ++srcp;
        }
    }

    // If we trim the end, move pointer back if needed
    if (trailing) {
        while (endp >= srcp) {
            if (!iswspace(*endp))
                break;
            --endp;
        }
    }

    // If nothing changed, return the original string
    if (srcp == osrcp && endp == oendp)
        return inStr;

    // Output buffer, big enough for string + last char + null terminator
    // Add to destp before subtracting srcp, else could be negative size
    // Buffer will always be at least one char wide, even if passed
    //   inStr was empty
    size_t len = (endp + 2) - srcp;
    destp = bufp = new wchar_t[len];

    // Copy - could be more efficient with different cdr::String
    //   constructor, but no big deal
    // This works even if string is all whitespace
    while (srcp <= endp)
        *destp++ = *srcp++;
    *destp = (wchar_t) '\0';

    // Return copy of string
    cdr::String outStr(bufp, len-1);
    delete[] bufp;
    return outStr;
}


/**
 * Extracts integer value from wide string.
 */
int cdr::String::getInt() const
{
    std::wistringstream is(*this);
    int i = 0;
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
    cdr::RegEx pattern(L"CDR\\d+(#.*)?");
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

/**
 * Construct a Blob object from it's encoded string version.
 */
cdr::Blob::Blob(const char* base64, size_t nBytes) : null(true)
{
    // Keep this outside the try block so we can delete it in the catch.
    unsigned char* buf = 0;

    try {

        // This will give us a buffer (not too much) larger than necessary.
        unsigned char* p = buf = new unsigned char[nBytes];

        // Walk through the encoded string.
        const char* table = getDecodingTable();
        const char* ip = base64;
        const char* end = base64 + nBytes;
        char c = '\0';
        int state = 0;
        while (ip < end) {
            c = *ip++;
            if (isspace(c))
                continue;
            else if (c == getPadChar())
                break;
            else if (c < 0 || c >= (int)getDecodingTableSize()) {
                wchar_t err[80];
                swprintf(err, L"Invalid base-64 character: %u", c);
                throw cdr::Exception(err);
            }
            char bits = table[c];
            if (bits == invalidBits()) {
                wchar_t err[80];
                swprintf(err, L"Invalid base-64 character: %u", c);
                throw cdr::Exception(err);
            }
            switch (state) {
                case 0:
                    *p    =  bits << 2;
                    ++state;
                    break;
                case 1:
                    *p++ |=  bits >> 4;
                    *p    = (bits & 0x0F) << 4;
                    ++state;
                    break;
                case 2:
                    *p++ |=  bits >> 2;
                    *p    = (bits & 0x03) << 6;
                    ++state;
                    break;
                case 3:
                    *p++ |=  bits;
                    state = 0;
                    break;
            }
        }

        // Verify that we have no invalid trailing garbage.
        if (c == getPadChar()) {

            // States 0 and 1 are invalid if we got a padding character.
            if (state < 2)
                throw cdr::Exception(L"Invalid number of significant "
                                     L"characters in base-64 encoding");

            // For state 2 there should be two padding characters; get the
            // other one.  We're doing this carefully enough to handle the
            // case in which the two padding characters are separated by
            // whitespace.
            if (state == 2) {
                c = 0;
                while (ip < end) {
                    c = *ip++;
                    if (c == getPadChar())
                        break;
                    if (isspace(c))
                        continue;
                    throw cdr::Exception(L"Invalid character following "
                            L"padding character in base-64 encoding");
                }
                if (c != getPadChar())
                    throw cdr::Exception(L"Invalid number of significant "
                                         L"characters in base-64 encoding");
            }

            // Everything else should be whitespace.
            while (ip < end) {
                c = *ip++;
                if (!isspace(c))
                    throw cdr::Exception(L"Invalid character following "
                            L"padding character in base-64 encoding");
            }

            // Check for invalid trailing bits.
            if (ip < end)
                throw cdr::Exception(L"Some hacker is trying to slip in "
                        L"some extra bits at the end of a base-64 encoded "
                        L"value");
        }

        // No padding: we should have come out on an even 4-character boundary.
        else if (state != 0)
            throw cdr::Exception(L"Invalid number of significant "
                                 L"characters in base-64 encoding");

        // All done.
        assign(buf, p - buf);
        delete [] buf;
    }
    catch (...) {
        delete [] buf;
        throw;
    }
    null = false;
}

/**
 * Encode the Blob object's content as a base-64 string.
 */
cdr::String cdr::Blob::encode() const
{
    wchar_t *buf = 0;
    try {
        size_t nBytes = size();
        size_t nChars = (nBytes /  3 + 1) * 4   // encoding bloat
                      + (nBytes / 57 + 1) * 2;  // line breaks
        buf = new wchar_t[nChars];
        size_t i = 0;
        wchar_t* p = buf;
        size_t charsInLine = 0;
        const unsigned char* bits = data();
        const wchar_t* codes = getEncodingTable();
        while (i < nBytes) {
            switch (nBytes - i) {
            case 1:
                p[0] = codes[(bits[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(bits[i + 0] << 4)                    & 0x3F];
                p[2] = getPadChar();
                p[3] = getPadChar();
                break;
            case 2:
                p[0] = codes[(bits[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(bits[i + 0] << 4 | bits[i + 1] >> 4) & 0x3F];
                p[2] = codes[(bits[i + 1] << 2)                    & 0x3F];
                p[3] = getPadChar();
                break;
            default:
                p[0] = codes[(bits[i + 0] >> 2)                    & 0x3F];
                p[1] = codes[(bits[i + 0] << 4 | bits[i + 1] >> 4) & 0x3F];
                p[2] = codes[(bits[i + 1] << 2 | bits[i + 2] >> 6) & 0x3F];
                p[3] = codes[(bits[i + 2])                         & 0x3F];
                break;
            }
            i += 3;
            p += 4;
            charsInLine += 4;
            if (charsInLine >= 76) {
                *p++ = L'\r';
                *p++ = L'\n';
                charsInLine = 0;
            }
        }
        if (charsInLine) {
            *p++ = L'\r';
            *p++ = L'\n';
        }

        // Release buffer and return as string
        cdr::String retval = cdr::String(buf, p - buf);
        delete [] buf;
        return retval;
    }
    catch (...) {
        delete [] buf;
        throw;
    }
}

/**
 * Access method for base-64 decoding table.
 */
const char* cdr::Blob::getDecodingTable()
{
    static std::string tableString;
    static const char* decodingTable = 0;
    if (!decodingTable) {
        const wchar_t* codes = getEncodingTable();
        tableString.resize(getDecodingTableSize(), invalidBits());
        for (char i = 0; codes[i]; ++i)
            tableString[codes[i]] = i;
        decodingTable = tableString.data();
    }
    return decodingTable;
}

/**
 * Compute a 32 bit hash value for a byte stream.
 * Adapted from: http://www.cs.yorku.ca/~oz/hash.html.
 *
 *  "this algorithm (k=33) was first reported by dan bernstein
 *   many years ago in comp.lang.c. another version of this algorithm
 *   (now favored by bernstein) uses xor:
 *      hash(i) = hash(i - 1) * 33 ^ str[i];
 *   the magic of number 33 (why it works better than many other
 *   constants, prime or not) has never been adequately explained."
 */
unsigned long cdr::hashBytes(const unsigned char *bytes, size_t len)
{
    unsigned long hash = 5381;
    int c;

    // Can't hash what isn't there
    if (len < 1)
        return 0L;

    // Here's Bernstein's original algorithm, adapted for passed len
    //   instead of null terminated string
    while (len--) {
        c = *bytes++;
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

