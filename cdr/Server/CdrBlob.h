/*
 * $Id: CdrBlob.h,v 1.1 2000-05-03 15:37:25 bkline Exp $
 *
 * CDR wrapper for a string of bytes.
 *
 * $Log: not supported by cvs2svn $
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
        Blob(bool null_ = false) : null(null_) {}
        Blob(const unsigned char*s, size_t n) : BlobBase(s, n), null(false) {}
        Blob(const Blob& b) : BlobBase(b), null(b.null) {}
        bool    isNull() const { return null; }
    private:
        bool    null;
    };

}

#endif
