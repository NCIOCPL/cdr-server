/*
 * $Id: CdrParserInput.h,v 1.4 2000-05-04 11:52:35 bkline Exp $
 *
 * Maintains state information for input for CDR string parser.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
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
         *
         *  @param  s       reference to string to be parsed.
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
         *  @param  dummy           unused argument flagging this
         *                          version of the operator as the
         *                          post-increment flavor.
         *  @return                 copy of the object before incrementing.
         *
         *  @exception cdr::Exception if an attempt is made to increment
         *                          past the end of the input.
         */
        ParserInput operator++(int dummy) {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            return ParserInput(buf, pos++, len);
        }

        /**
         * Pre-increments the current parsing position.  Returns a reference
         * to the object which can be used in the expression "*++pInput."
         *
         *  @return                 reference to the modified object.
         *  @exception cdr::Exception if an attempt is made to increment
         *                          past the end of the input.
         */
        ParserInput& operator++() {
            if (pos >= len) throw cdr::Exception(L"ParserInput: past EOF");
            ++pos;
            return *this;
        }

        /**
         * Accessor method for individual characters in the buffer.
         *
         * @return                  16-bit character for the current 
         *                          input position.
         */
        wchar_t operator *() const { return buf[pos]; }

        /**
         * Skips <code>n</code> positions past the current input position.
         *
         *  @param  n               number of positions to skip.
         *  @return                 reference to the modified object.
         *  @exception cdr::Exception if an attempt is made to move past
         *                          the end of the input.
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
         *  @param  delim           delimiter marking end of string.
         *  @return                 address of newly allocated buffer 
         *                          containing null-terminated copy of
         *                          extracted string.
         *  @exception  cdr::Exception if matching delimiter is not found
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
         *  @param  count           number of characters to extract.
         *  @return                 address of newly allocated buffer 
         *                          containing null-terminated copy of
         *                          extracted string.
         *  @throw  cdr::Exception if more characters are requested than
         *                         remain in the input buffer.
         */
        const wchar_t* extractString(size_t count);

        /**
         * Returns <code>true</code> if the input matches the specified
         * null-terminated buffer at the current input position.
         *
         *  @param  s               address of null-terminated wide-character
         *                          string to be matched.
         *  @return                 <code>true</code> if specified string
         *                          is matched by the current buffer contents.
         */
        bool matches(const wchar_t* s) const {
            if (!s) return false;
            size_t len = wcslen(s);
            return wcsncmp(buf + pos, s, len) == 0;
        }

        /**
         * Returns the current input position.  Useful for error reporting.
         *
         *  @return                 current input position.
         */
        size_t getPos() const { return pos; }

        /**
         * Returns the original length of the input string.  Useful for error
         * reporting.
         *
         *  @return                 original length of the input string.
         */
        size_t getLen() const { return len; }

        /**
         * Reports the token last passed to the parser by the lexical
         * analyzer.  Useful for error reporting.
         *
         *  @return                 last token passed to the parser.
         */
        int    getLastTok() const { return lastTok; }

        /**
         * Remembers the token last passed to the parser by the lexical
         * analyzer.
         *
         *  @param  t               token passed to the parser.
         *  @return                 token passed by the caller (echoed back).
         */
        int    setLastTok(int t) { return lastTok = t; }

        /**
         * Reports the number of characters remaining in the input.  Might be
         * useful for error reporting.
         *
         *  @return                 number of characters remaining in the 
         *                          input buffer.
         */
        size_t getRemainingLen() const { return len - pos; }

        /**
         * Returns a read-only pointer to the input buffer.
         *
         *  @return                 address of the beginning of the original
         *                          input buffer.
         */
        const wchar_t* getBuf() const { return buf; }

        /**
         * Returns a read-only pointer to the current position in the input
         * buffer.  Allows the lexical analyzer to perform look-ahead before
         * it decides what to do with the input.
         *
         *  @return                 address of the current position in the
         *                          input buffer.
         */
        const wchar_t* getRemainingBuf() const { return buf + pos; }

    private:

        /**
         * Self-destruction object to hold the buffer contents.
         */
        String          str;

        /**
         * Current processing position in the buffer.
         */
        size_t          pos;

        /**
         * Total length of the input buffer, in characters.
         */
        size_t          len;

        /**
         * Address of the character array for the input buffer.
         */
        const wchar_t*  buf;

        /**
         * Record of the last token passed to the parser.
         */
        int             lastTok;

        /**
         * Collection of strings extracted from the buffer, maintained so that
         * the addresses returned to callers will be valida as long as (but no
         * longer than) the <code>ParserInput</code> object is valid.
         */
        StringList      extractedStrings;

        /**
         * Default constructor.  Creates empty buffer.
         */
        ParserInput() : buf(L""), pos(0), len(0) {}

        /**
         * Unimplemented (blocked) copy constructor.
         *
         *  @param  pi          reference to object to be copied.
         */
        ParserInput(const ParserInput& pi);

        /**
         * Unimplemented (blocked) assignment operator.
         *
         *  @param  pi          reference to object to be copied.
         *  @return             reference to modified object.
         */
        ParserInput& operator=(const ParserInput& pi);

        /**
         * Used by the post-increment operator.  Note that not all of the
         * state information is copied from the original object.  See the
         * notes above in the comment for the post-increment operator.
         *
         *  @param  b           address of character array for buffer.
         *  @param  p           current position in buffer.
         *  @param  l           length of buffer.
         */
        ParserInput(const wchar_t* b, size_t p, size_t l)
            : buf(b), pos(p), len(l) {}
    };

}

#endif
