/*
 * $Id: CdrInt.h,v 1.2 2000-04-22 18:57:38 bkline Exp $
 *
 * Nullable integer type.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/17 21:27:23  bkline
 * Initial revision
 *
 */

#ifndef CDR_INT_
#define CDR_INT_

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    class Int {
    public:
        Int(bool n) : null(n), val(0) {}
        Int(int i) : null(false), val(i) {}
        operator int() const { return val; }
        bool    isNull() const { return null; }
    private:
        int     val;
        bool    null;
    };
}

#endif
