/*
 * $Id: CdrException.h,v 1.3 2000-05-04 01:14:32 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.1  2000/04/11 14:18:07  bkline
 * Initial revision
 */

#ifndef CDR_EXCEPTION_
#define CDR_EXCEPTION_

#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Carries information about an error condition which was encountered
     * during CDR processing.  We wanted to derived this from the standard
     * exception class, but that class is not a template class, and is
     * inexplicably and inextricably tied to 8-bit character strings.  Bah!
     */
    class Exception {

    public:

        /**
         * Creates a new <code>Exception</code> object from a null-terminated
         * Unicode string (encoded as UTF-16).
         *
         *  @param  s           address of null-terminated wide-character
         *                      string.
         */
        Exception(const wchar_t *s) : str(s) {}

        /**
         * Creates a new <code>Exception</code> object from a pair of
         * <code>String</code> objects.  Typically used when a general error
         * message for a particular error condition needs to be combined with
         * a specific value.  The two strings are stored with L": " as
         * separation.
         *
         *  @param  s1          reference to general error string.
         *  @param  s2          reference to specific error string.
         *
         */
        Exception(const cdr::String& s1, const cdr::String& s2) 
            : str(s1 + L": " + s2) {}

        /**
         * Accessor method for the string version of the object.
         *
         *  @return             string representation of the error condition
         *                      represented by the <code>Exception</code>
         *                      object.
         */
        cdr::String what() const { return str; }

    private:

        /**
         * String representation of the error condition triggering this
         * exception.
         */
        cdr::String str;
    };
}

#endif
