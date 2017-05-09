/*
 * CDR wrapper for a string of bytes.
 *
 * BZIssue::4767
 */

#ifndef CDR_BLOB_
#define CDR_BLOB_

#include "CdrString.h"
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
         * Creates a new non-null <code>Blob</code> object from an array
         * of bytes.
         *
         *  @param  s           address of array of bytes.
         *  @param  n           number of bytes in array.
         */
        Blob(const unsigned char*s, size_t n) : BlobBase(s, n), null(false) {}

        /**
         * Creates a new non-null <code>Blob</code> object from a base-64-
         * encoded string.
         *
         *  @param  s           pointer to base-64 encoded string buffer
         *  @param  n           count of characters in buffer
         */
        Blob(const char* s, size_t n);

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

        /**
         * Returns a base-64 encoded string object for the blob.
         *
         *  @return             encoded String object.
         */
        String encode() const;

        /**
         * Returns an unsigned integer representing a hash
         * of the bytes in the Blob.
         *
         *  @return             Hash value.
         */
        unsigned int hash() const { return cdr::hashBytes(data(), length()); }

    private:

        /**
         * Flag remembering whether object is <code>NULL</code>.
         */
        bool    null;

        /**
         * Character used for filling out blob encoding.
         */
        static wchar_t getPadChar() { return L'='; }

        /**
         * Table used for encoding blobs.
         */
        static const wchar_t* getEncodingTable() {
            return L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   L"abcdefghijklmnopqrstuvwxyz"
                   L"0123456789+/";
        }


        /**
         * Access method for initializing and using the decode table.
         *
         *  @return             decoding table.
         */
        static const char* getDecodingTable();
        static size_t getDecodingTableSize() { return 128; }

        /**
         * Only the low six bits can be represented in an encoding character.
         */
        static char invalidBits() { return '\xC0'; }
    };
}

#endif // CDR_BLOB_
