/*
 * Nullable integer type.
 */

#ifndef CDR_INT_
#define CDR_INT_

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Nullable integer object.  Useful for representing nullable database
     * columns.
     */
    class Int {

    public:

        /**
         * Creates a possibly NULL object representing an integer value.
         * The integer value is initialized to zero.
         *
         *  @param  n       flag indicating whether the object is NULL.
         */
        Int(bool n) : null(n), val(0) {}

        /**
         * Creates a non-NULL object representing the specified integer
         * value.
         *
         *  @param  i       integer value used to initialize the object.
         */
        Int(int i) : null(false), val(i) {}

        /**
         * conversion operator.
         *
         *  @return         integer value represented by the object.
         */
        operator int() const { return val; }

        /**
         * Accessor method reporting whether the object is NULL.
         *
         *  @return         <code>true</code> iff the object is NULL.
         */
        bool    isNull() const { return null; }

    private:

        /**
         * Integer value represented by the object.
         */
        int     val;

        /**
         * Flag indicating whether the object is NULL.
         */
        bool    null;
    };
}

#endif
