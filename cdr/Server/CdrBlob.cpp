/*
 * $Id: CdrBlob.cpp,v 1.1 2004-10-22 03:20:42 ameyer Exp $
 *
 * Manage the occurrence of blobs and references to blobs in the
 * CDR database.
 *
 * $Log: not supported by cvs2svn $
 */

#include <string>
#include "CdrBlob.h"
#include "CdrDbConnection.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrDbPreparedStatement.h"
#include "CdrException.h"

/**
 * Put an entry in the doc_blob_usage table
 */
static void addDocBlobUsage(
    cdr::db::Connection& conn,
    int docId,
    int blobId
) {
    std::string qry =
        "INSERT INTO doc_blob_usage (doc_id, blob_id) VALUES (?,?)";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, docId);
    stmt.setInt(2, blobId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Replace an existing entry in the doc_blob_usage table
 */
static void repDocBlobUsage(
    cdr::db::Connection& conn,
    int docId,
    int blobId
) {
    std::string qry = "UPDATE doc_blob_usage SET blob_id=? WHERE doc_id=?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, blobId);
    stmt.setInt(2, docId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Delete an existing entry in the doc_blob_usage table
 */
static void delDocBlobUsage(
    cdr::db::Connection& conn,
    int docId
) {
    std::string qry = "DELETE FROM doc_blob_usage WHERE doc_id=?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, docId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Put an entry in the version_blob_usage table.
 * Replace and delete are not required because old versions
 *   never change.
 */
static void addVersionBlobUsage(
    cdr::db::Connection& conn,
    int docId,
    int version,
    int blobId
) {
    std::string qry =
        "INSERT INTO version_blob_usage (doc_id, version, blob_id) "
        " VALUES (?,?,?)";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, docId);
    stmt.setInt(2, version);
    stmt.setInt(3, blobId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Find the blob id associated with a doc or a doc_version.
 *
 *  @param conn         Database connection.
 *  @param docId        Find blob associated with this document.
 *  @param version      If non-zero, look for version, not doc association.
 *  @return             Blob id.
 *                      0 if no blob associated with id(s).
 */
int lookupBlobId(
    cdr::db::Connection& conn,
    int docId,
    int version
) {
    int blobId = 0;
    std::string qry;

    if (version)
        // Look for doc_version blob
        qry = "SELECT blob_id FROM version_blob_usage "
              " WHERE doc_id=? AND version=?";
    else
        // Look for doc blob
        qry = "SELECT blob_id FROM doc_blob_usage WHERE doc_id=?";

    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, docId);
    if (version)
        stmt.setInt(2, version);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
        blobId = rs.getInt(1);
    stmt.close();
    return blobId;
}

/**
 * Tell the caller whether a particular blob has ever been versioned.
 * If so, we can't replace or delete it, only add a new blob.
 *
 *  @param conn         Database connection.
 *  @param blobId       Check on this blob.
 *  @return             True  = Blob has been versioned at least once.
 *                      False = No versions of this blob.
 */
bool isBlobVersioned(
    cdr::db::Connection& conn,
    int blobId
) {
    bool isVersioned = false;

    std::string qry =
        "SELECT TOP 1 doc_id FROM version_blob_usage WHERE blob_id=?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, blobId);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
        isVersioned = true;
    stmt.close();
    return isVersioned;
}

/**
 * Add a blob to the database for a particular document and optional version.
 *
 * Assumptions:
 *   Only call this if we already know that a new blob is required.
 *
 *  @param conn         Database connection.
 *  @param blob         Reference to the blob itself.
 *  @param docId        Find blob associated with this document.
 *  @param version      If non-zero, look for version, not doc association.
 *  @return             Blob id for new blob.
 */
static int addBlob(
    cdr::db::Connection& conn,
    cdr::Blob blob,
    int docId,
    int version
) {
    // First insert the new blob, getting the new blob id
    std::string qry = "INSERT INTO doc_blob (data) VALUES (?)";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setBytes(1, blob);
    stmt.executeUpdate();
    int blobId = conn.getLastIdent();
    stmt.close();

    // If there is already an association between this doc and an old blob
    //   replace the old association
    if (lookupBlobId(conn, docId, 0))
        repDocBlobUsage(conn, docId, blobId);

    // If we're creating a version, make the new version association
    if (version)
        addVersionBlobUsage(conn, docId, version, blobId);

    return blobId;
}

/**
 * Replace a blob.
 *
 * Assumptions
 *    Blob has never been versioned.  If it has, use addBlob not repBlob.
 *
 *  @param conn         Database connection.
 *  @param blob         Reference to the blob itself.
 *  @param docId        Replace blob associated with this document.
 *  @param version      If non-zero, also create a version usage for this blob.
 *  @param blobId       ID of the blob to replace.
 *  @return             Void.
 */
static void replaceBlob(
    cdr::db::Connection& conn,
    cdr::Blob blob,
    int docId,
    int version,
    int blobId
) {
    // Validate
    if (isBlobVersioned(conn, blobId)) {
        wchar_t msg[80];
        swprintf(msg, L"Attempt to replace versioned blob for doc=%d",
                 docId);
        throw cdr::Exception(msg);
    }

    // First replace the blob itself
    std::string qry = "UPDATE doc_blob set data=? WHERE blob_id=?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setBytes(1, blob);
    stmt.setInt(2, blobId);
    stmt.executeUpdate();
    stmt.close();

    // If we're creating a version, make the new version association
    // Document association already exists
    if (version)
        addVersionBlobUsage(conn, docId, version, blobId);
}

/**
 * Delete a blob, identified by document id, from the database.
 *
 * Assumptions:
 *   Blob has never been versioned.
 *   User wishes to eliminate the blob from the current working document.
 *
 *  @param conn         Database connection.
 *  @param docId        ID of the doc associated with blob.
 *  @return             Void.
 */
static void delBlob(
    cdr::db::Connection& conn,
    int docId
) {
    wchar_t msg[80];    // For error messages

    // Find the blob id for the document
    int blobId = lookupBlobId(conn, docId, 0);

    // Validate
    if (!blobId) {
        swprintf(msg, L"Attempt to delete non-existent blob for doc=%d",
                 docId);
        throw cdr::Exception(msg);
    }

    // Validate no versioning exists
    if (isBlobVersioned(conn, blobId)) {
        swprintf(msg, L"Attempt to delete versioned blob for doc=%d",
                 docId);
        throw cdr::Exception(msg);
    }

    // Break the association between document and blob, there can only be one
    delDocBlobUsage(conn, docId);

    // Now delete the blob
    std::string qry = "DELETE FROM doc_blob WHERE blob_id=?";
    cdr::db::PreparedStatement stmt = conn.prepareStatement(qry);
    stmt.setInt(1, blobId);
    stmt.executeUpdate();
    stmt.close();
}
