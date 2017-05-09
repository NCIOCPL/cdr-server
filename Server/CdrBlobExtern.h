/*
 * Declarations for external blob management routines.
 *
 * Can't put these in CdrBlob.h because that would create a
 * circular reference with CdrDbConnection.h - which must
 * reference blobs, and these routines must reference
 * connections.
 */

#ifndef CDR_BLOB_EXTERN_
#define CDR_BLOB_EXTERN_

#include "CdrBlob.h"
#include "CdrDbConnection.h"

namespace cdr {

    /**
     * Retrieve a pointer to a blob by its blob id.
     *
     * IMPORTANT: delete the blob when done with it.
     *
     *  @param conn         Database connection.
     *  @param blobId       Find blob associated with this internal id.
     *  @return             Pointer to blob.  Caller must delete it.
     *                       Returns 0 if blob not found.
     */
    cdr::Blob *getBlobPtrByBlobId(cdr::db::Connection& conn, int blobId);

    /**
     * Retrieve a pointer to a blob by its associated document id.
     *
     * IMPORTANT: delete the blob when done with it.
     *
     *  @param conn         Database connection.
     *  @param docId        Find blob associated with this document.
     *  @return             Pointer to blob.  Caller must delete it.
     *                       Returns 0 if blob not found.
     */
    cdr::Blob *getBlobPtrByDocId(cdr::db::Connection& conn, int docId);

    /**
     * Retrieve a pointer to a blob by its associated doc_version.
     *
     * IMPORTANT: delete the blob when done with it.
     *
     *  @param conn         Database connection.
     *  @param docId        Find blob associated with this document.
     *  @param version      Version number.
     *  @return             Pointer to blob.  Caller must delete it.
     *                       Returns 0 if blob not found.
     */
    cdr::Blob *getBlobPtrByDocVersion(cdr::db::Connection&, int docId,
                                      int version);

    /**
     * Put a blob in the database.
     *
     * The function determines for itself if this is a new blob or not.
     *
     *  @param conn         Database connection.
     *  @param blob         Reference to the blob itself.
     *  @param docId        Associate blob with this document.
     *  @return             ID of blob
     */
    int setBlob(cdr::db::Connection& conn, cdr::Blob& blob, int docId);

    /**
     * Add an association between a document version and a blob.
     *
     * Must be called immediately after creating the version.  It
     * will always and only make the association between the
     * passed docId + blobId and the last created version.
     *
     * Must execute it inside a transaction that creates the blob
     * (if needed) and the version in order to guarantee the
     * integrity of the query to find the last doc_version.
     *
     *  @param conn         Database connection.
     *  @param docId        Associate blob with this document.
     *  @param blobId       Blob id to associate with last version of doc
     *  @return             Void.
     */
    void addVersionBlobUsage(cdr::db::Connection& conn, int docId, int blobId);

    /**
     * For a given doc ID, PERMANENTLY delete all links between the
     * CWD and blob and between all versions and other blobs.
     * Then delete all the blobs themselves.
     *
     * Caller should include this in a database transaction so that if
     * related changes to the document or blobs fail, all can be rolled back.
     *
     * As of this writing, this function should ONLY be used for deleting
     * PDQ Board meeting records that have outlived their useful life.
     *
     *  @param conn         Database connection.
     *  @param docId        Delete blobs linked to this document.
     *
     *  @return             Void.
     */
    void delAllBlobVersions(cdr::db::Connection& conn, int docId);
}

#endif // CDR_BLOB_EXTERN_
