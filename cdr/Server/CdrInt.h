/*
 * $Id: CdrInt.h,v 1.1 2000-04-17 21:27:23 bkline Exp $
 *
 * Nullable integer type.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_INT_
#define CDR_INT_

namespace cdr {
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
