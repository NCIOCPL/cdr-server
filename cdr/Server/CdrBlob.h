/*
 * $Id: CdrBlob.h,v 1.2 2000-05-03 17:27:54 bkline Exp $
 *
 * CDR wrapper for a string of bytes.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/05/03 15:37:25  bkline
 * Initial revision
 */

#ifndef CDR_BLOB_
#define CDR_BLOB_

#include <string>

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * This typedef is required to work around MSVC++ bugs in the handling of
     * parameterized types.
     */
    typedef std::basic_string<unsigned char> BlobBase;

    /**
     * Wrapper class for strings of bytes.
     */
    class Blob : public BlobBase {

    public:

        /**
         * Creates an empty (and possibly NULL) object for a string of bytes.
         *
         *  @param  null_       <code>true</code> if new object should be
         *                      <code>NULL</code>; default is
         *                      <code>false</code>
         */
        Blob(bool null_ = false) : null(null_) {}

        /**
         * Creates a new non-nul <code>Blob</code> object from an array 
         * of bytes.
         *
         *  @param  s           address of array of bytes.
         *  @param  n           number of bytes in array.
         */
        Blob(const unsigned char*s, size_t n) : BlobBase(s, n), null(false) {}

        /**
         * Copy constructor.
         *
         *  @param  b           reference to object to be copied.
         */
        Blob(const Blob& b) : BlobBase(b), null(b.null) {}

        /**
         * Returns setting of object's <code>null</code> flag.
         *
         *  @returns            <code>true</code> if object is NULL.
         */
        bool    isNull() const { return null; }

    private:

        /**
         * Flag remembering whether object is <code>NULL</code>.
         */
        bool    null;
    };

}

#endif
