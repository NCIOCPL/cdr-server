/*
 * $Id: CdrParserInput.h,v 1.3 2000-04-22 18:57:38 bkline Exp $
 *
 * Maintains state information for input for CDR string parser.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/22 16:33:24  bkline
 * Fleshed out documentation.
 *
 * Revision 1.1  2000/04/21 14:00:52  bkline
 * Initial revision
 */

#ifndef CDR_PARSER_INPUT_
#define CDR_PARSER_INPUT_

#include "CdrString.h"
#include "CdrException.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * The <code>ParserInput</code> class manages the input fed to a
     * lexical analyzer on behalf of a CDR parser for in-memory strings.
     */
    class ParserInput {
    public:

        /**
         * Creates a new input object, reading for parsing.
         */
        ParserInput(const String& s) 
            : str(s), pos(0), buf(str.c_str()), len(str.size()), lastTok(-1) {}

        /**
         * Post-increments the current parsing position.  Returns a new
         * object which remembers the original un-incremented position,
         * which can be used in the expression "*parserInput++."
         * <P>
         * <EMPH>WARNING!  Do not use the return value from this operator
         * beyond the lifetime of the original object, because the 
         * object's destructor will have destroyed the state information
         * which makes the object useful.  I'm fairly confident that
         * by blocking the assignment operator and copy constructor
         * such a misuse has been prevented.
         *
         *  @throw  cdr::Exception if an attempt is made to increment
         *                         past the end of the input.
         */
        ParserInput operator++(int) {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            return ParserInput(buf, pos++, len);
        }

        /**
         * Pre-increments the current parsing position.  Returns a reference
         * to the object which can be used in the expression "*++pInput."
         *
         *  @throw  cdr::Exception if an attempt is made to increment
         *                         past the end of the input.
         */
        ParserInput& operator++() {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            ++pos;
            return *this;
        }

        /**
         * Returns the 16-bit character for the current input position.
         */
        wchar_t operator *() const { return buf[pos]; }

        /**
         * Skips <code>n</code> positions past the current input position.
         *
         *  @throw  cdr::Exception if an attempt is made to move past
         *                         the end of the input.
         */
        ParserInput& operator+=(size_t n) { 
            if (pos + n > len)
                throw cdr::Exception(L"ParserInput: past EOF");
            pos += n; 
            return *this;
        }

        /**
         * Returns a null-terminated buffer containing the value enclosed
         * in quote (single or double) delimiters.  Removes escaping
         * backslashes (used to escape the quote delimiter when used
         * as part of the string's value).  The buffer is owned by the
         * <code>ParserInput</code> object, which is responsible for
         * disposing of the allocated memory.  The current position
         * is moved forward past the terminating delimiter.
         *
         *  @throw  cdr::Exception if matching delimiter is not found
         *                         before the end of the input buffer.
         */
        const wchar_t* extractString(wchar_t delim);

        /**
         * Returns a null-terminated buffer containing <code>count</code>
         * characters beginning at the current input position.  The 
         * buffer is owned by the <code>ParserInput</code> object, which is
         * responsible for disposing of the allocated memory.  The current
         * position is moved forward past the terminating delimiter.
         *
         *  @throw  cdr::Exception if more characters are requested than
         *                         remain in the input buffer.
         */
        const wchar_t* extractString(size_t count);

        /**
         * Returns <code>true</code> if the input matches the specified
         * null-terminated buffer at the current input position.
         */
        bool matches(const wchar_t* s) const {
            if (!s) return false;
            size_t len = wcslen(s);
            return wcsncmp(buf + pos, s, len) == 0;
        }

        /**
         * Returns the current input position.  Useful for error reporting.
         */
        size_t getPos() const { return pos; }

        /**
         * Returns the original length of the input string.  Useful for error
         * reporting.
         */
        size_t getLen() const { return len; }

        /**
         * Reports the token last passed to the parser by the lexical
         * analyzer.  Useful for error reporting.
         */
        int    getLastTok() const { return lastTok; }

        /**
         * Remembers the token last passed to the parser by the lexical
         * analyzer.
         */
        int    setLastTok(int t) { return lastTok = t; }

        /**
         * Reports the number of characters remaining in the input.  Might be
         * useful for error reporting.
         */
        size_t getRemainingLen() const { return len - pos; }

        /**
         * Returns a read-only pointer to the input buffer.
         */
        const wchar_t* getBuf() const { return buf; }


        /**
         * Returns a read-only pointer to the current position in the input
         * buffer.  Allows the lexical analyzer to perform look-ahead before
         * it decides what to do with the input.
         */
        const wchar_t* getRemainingBuf() const { return buf + pos; }

    private:
        String          str;
        size_t          pos;
        size_t          len;
        const wchar_t*  buf;
        int             lastTok;
        StringList      extractedStrings;
        ParserInput() : buf(L""), pos(0), len(0) {}
        ParserInput(const ParserInput&);            // Block this.
        ParserInput operator=(const ParserInput&);  // And this.

        /**
         * Used by the post-increment operator.  Note that not all of the
         * state information is copied from the original object.  See the
         * notes above in the comment for the post-increment operator.
         */
        ParserInput(const wchar_t* b, size_t p, size_t l)
            : buf(b), pos(p), len(l) {}
    };

}

#endif
