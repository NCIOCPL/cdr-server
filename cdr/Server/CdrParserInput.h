/*
 * $Id: CdrParserInput.h,v 1.1 2000-04-21 14:00:52 bkline Exp $
 *
 * Maintains state information for input for CDR string parser.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_PARSER_INPUT_
#define CDR_PARSER_INPUT_

#include "CdrString.h"
#include "CdrException.h"

namespace cdr {

    class ParserInput {
    public:
        ParserInput(const String& s) 
            : str(s), pos(0), buf(str.c_str()), len(str.size()), lastTok(-1) {}
        // Careful!  Don't use the return from this beyond lifetime of
        // original object!
        ParserInput operator++(int) {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            return ParserInput(buf, pos++, len);
        }
        ParserInput& operator++() {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            ++pos;
            return *this;
        }
        wchar_t operator *() const { return buf[pos]; }
        ParserInput& operator+=(size_t n) { 
            if (pos + n > len)
                throw cdr::Exception(L"ParserInput: past EOF");
            pos += n; 
            return *this;
        }
        const wchar_t* extractString(wchar_t delim);
        const wchar_t* extractString(size_t count);
        bool matches(const wchar_t* s) const {
            if (!s) return false;
            size_t len = wcslen(s);
            return wcsncmp(buf + pos, s, len) == 0;
        }
        size_t getPos() const { return pos; }
        size_t getLen() const { return len; }
        int    getLastTok() const { return lastTok; }
        int    setLastTok(int t) { return lastTok = t; }
        size_t getRemainingLen() const { return len - pos; }
        const wchar_t* getBuf() const { return buf; }
        const wchar_t* getRemainingBuf() const { return buf + pos; }
    private:
        ParserInput() : buf(L""), pos(0), len(0) {}
        ParserInput(const ParserInput&); // Block this.
        ParserInput(const wchar_t* b, size_t p, size_t l)
            : buf(b), pos(p), len(l) {}
        String str;
        size_t pos, len;
        const wchar_t* buf;
        int lastTok;
        StringList extractedStrings;
    };

}

#endif
