/*
 * $Id: CdrException.h,v 1.2 2000-04-22 18:57:38 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/11 14:18:07  bkline
 * Initial revision
 *
 */

#ifndef CDR_EXCEPTION_
#define CDR_EXCEPTION_

#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Base class for exceptions thrown by the CDR.
     */
    class Exception {
    public:
        Exception(const wchar_t *s) : str(s) {}
        Exception(const cdr::String& s1, const cdr::String& s2) 
            : str(s1 + L": " + s2) {}
        cdr::String getString() const { return str; }
    private:
        cdr::String str;
    };
}

#endif
