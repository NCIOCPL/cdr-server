/*
 * $Id: CdrRegEx.h,v 1.1 2000-05-03 15:41:36 bkline Exp $
 *
 * CDR wrapper for regular expression processing.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_REGEX_
#define CDR_REGEX_

#ifndef UNICODE
#define UNICODE
#define CDR_UNICODE_DEFINED_
#endif

#include <regex>
#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    class RegEx : private JM::wregex {
    public:
        RegEx(bool ignoreCase = false) : caseFlag(ignoreCase) {}
        RegEx(const wchar_t* pattern, bool ignoreCase = false);
        RegEx(const String& pattern, bool ignoreCase = false);
        RegEx(const char* pattern, bool ignoreCase = false);
        RegEx(const std::string& pattern, bool ignoreCase = false);
        String getPattern() const;
        bool    match(const wchar_t*);
        bool    match(const String&);
        bool    match(const char*);
        bool    match(const std::string&);
        void    set(const wchar_t* pattern);
        void    set(const String& patterh);
        void    set(const char* pattern);
        void    set(const std::string& patterh);
    private:
        bool    caseFlag;
        RegEx(const RegEx&);            // Block this
        RegEx operator=(const RegEx&);  // This, too.
    };

}

#ifdef CDR_UNICODE_DEFINED_
#undef UNICODE
#undef CDR_UNICODE_DEFINED_
#endif

#endif
