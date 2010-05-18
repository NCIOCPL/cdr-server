/*
 * $Id$
 *
 * Manage the occurrence of blobs and references to blobs in the
 * CDR database.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2004/10/22 03:20:42  ameyer
 * New document blob management subroutines.
 * First version not yet tested but installed in CVS in case someone
 * wants to see how it is intended to work.
 *
 */

#include <iostream> // Debug
#include <string>
#include "CdrBlob.h"
#include "CdrBlobExtern.h"
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
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "INSERT INTO doc_blob_usage (doc_id, blob_id) VALUES (?,?)");
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
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "UPDATE doc_blob_usage SET blob_id=? WHERE doc_id=?");
    stmt.setInt(1, blobId);
    stmt.setInt(2, docId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Delete an existing entry in the doc_blob_usage table.
 * No checking done.
 *
 *  @param conn             Database connection.
 *  @param docId            ID of the doc associated with this blob.
 */
static void delDocBlobUsage(
    cdr::db::Connection& conn,
    int docId
) {
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "DELETE FROM doc_blob_usage WHERE doc_id=?");
    stmt.setInt(1, docId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Put an entry in the version_blob_usage table for the last version
 * of the document.
 *
 */
void cdr::addVersionBlobUsage(
    cdr::db::Connection& conn,
    int docId,
    int blobId
) {
    // Get the number of the last version
    cdr::db::PreparedStatement verStmt = conn.prepareStatement(
        "SELECT MAX(num) FROM doc_version WHERE id=?");
    verStmt.setInt(1, docId);
    cdr::db::ResultSet rs = verStmt.executeQuery();
    if (!rs.next()) {
        wchar_t msg[160];
        swprintf(msg, L"Attempt to add blob=%d to doc version for doc=%d"
                      L" but that doc has no version.", blobId, docId);
        throw cdr::Exception(msg);
    }
    int ver = rs.getInt(1);
    verStmt.close();

    // Insert a row into the version_blob_usage table
    cdr::db::PreparedStatement insStmt = conn.prepareStatement(
        "INSERT INTO version_blob_usage (doc_id, blob_id, doc_version) "
        " VALUES (?, ?, ?)");
    insStmt.setInt(1, docId);
    insStmt.setInt(2, blobId);
    insStmt.setInt(3, ver);
    insStmt.executeUpdate();
    insStmt.close();
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
static int lookupBlobId(
    cdr::db::Connection& conn,
    int docId,
    int version
) {
    int blobId = 0;
    std::string qry;

    if (version)
        // Look for doc_version blob
        qry = "SELECT blob_id FROM version_blob_usage "
              " WHERE doc_id=? AND doc_version=?";
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
static bool isBlobVersioned(
    cdr::db::Connection& conn,
    int blobId
) {
    bool isVersioned = false;

    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "SELECT TOP 1 doc_id FROM version_blob_usage WHERE blob_id=?");
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
 *  @return             Blob id for new blob.
 */
static int addBlob(
    cdr::db::Connection& conn,
    cdr::Blob& blob,
    int docId
) {
    // First insert the new blob, getting the new blob id
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "INSERT INTO doc_blob (data) VALUES (?)");
    stmt.setBytes(1, blob);
    stmt.executeUpdate();
    int blobId = conn.getLastIdent();
    stmt.close();

    // If there is already an association between this doc and an old blob
    //   replace the old association
    if (lookupBlobId(conn, docId, 0))
        repDocBlobUsage(conn, docId, blobId);

    // Else establish the asociation
    else
        addDocBlobUsage(conn, docId, blobId);

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
 *  @param blobId       ID of the blob to replace.
 *  @return             Void.
 */
static void replaceBlob(
    cdr::db::Connection& conn,
    cdr::Blob& blob,
    int docId,
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
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "UPDATE doc_blob set data=? WHERE id=?");
    stmt.setBytes(1, blob);
    stmt.setInt(2, blobId);
    stmt.executeUpdate();
    stmt.close();
}

/**
 * Delete a blob, identified by document id, from the database.
 *
 * Assumptions:
 *   Blob has never been versioned.
 *   User wishes to eliminate the blob from the current working document.
 *   Caller must also call delDocBlobUsage().
 *
 *  @param conn         Database connection.
 *  @param blobId       ID of the blob to delete.
 *  @param docId        Passed solely for use in error messages.
 *  @return             Void.
 */
static void delBlob(
    cdr::db::Connection& conn,
    int blobId,
    int docId
) {
    wchar_t msg[80];    // For error messages

    // Validate
    if (!blobId) {
        swprintf(msg,
                 L"Attempt to delete non-existent blob docId=%d, blobId=%d",
                 docId, blobId);
        throw cdr::Exception(msg);
    }

    // Validate no versioning exists
    if (isBlobVersioned(conn, blobId)) {
        swprintf(msg,
                 L"Attempt to delete versioned blob docId=%d, blobId=%d",
                 docId, blobId);
        throw cdr::Exception(msg);
    }

    // Now delete the blob
    cdr::db::PreparedStatement stmt = conn.prepareStatement(
        "DELETE FROM doc_blob WHERE id=?");
    stmt.setInt(1, blobId);
    stmt.executeUpdate();
    stmt.close();
}

/* Get a blob from the database.
 * See CdrBlobExtern.h for documentation.
 */
cdr::Blob *cdr::getBlobPtrByBlobId(
    cdr::db::Connection& conn,
    int blobId
) {
    cdr::Blob *pBlob = 0;

    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "SELECT data FROM doc_blob WHERE id = ?");
    stmt.setInt(1, blobId);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
        pBlob = new cdr::Blob(rs.getBytes(1));
    stmt.close();

    return pBlob;
}

/* Get a blob from the database by doc id.
 * See CdrBlobExtern.h for documentation.
 */
cdr::Blob *cdr::getBlobPtrByDocId(
    cdr::db::Connection& conn,
    int docId
) {
    cdr::Blob *pBlob = 0;

    cdr::db::PreparedStatement stmt = conn.prepareStatement(
            "SELECT data FROM doc_blob JOIN doc_blob_usage "
            "    ON doc_blob.id = doc_blob_usage.blob_id "
            " WHERE doc_blob_usage.doc_id = ?");
    stmt.setInt(1, docId);
    cdr::db::ResultSet rs = stmt.executeQuery();
    if (rs.next())
        pBlob = new cdr::Blob(rs.getBytes(1));
    stmt.close();

    return pBlob;
}

/* Get a blob from the database by doc id + version.
 * See CdrBlobExtern.h for documentation.
 */
cdr::Blob *cdr::getBlobPtrByDocVersion(
    cdr::db::Connection& conn,
    int docId,
    int version
) {
    // Simple brute force - calling this should be pretty rare
    int blobId = lookupBlobId(conn, docId, version);
    if (blobId)
        return cdr::getBlobPtrByBlobId(conn, blobId);
    return 0;
}

/* Add or replace a blob in the database.
 * See CdrBlobExtern.h for interface.
 */
int cdr::setBlob(
    cdr::db::Connection& conn,
    cdr::Blob& blob,
    int docId
) {
    // Find id by inspecting the database, gets 0 if not found
    int blobId = lookupBlobId(conn, docId, 0);

    // Setting a zero length blob means delete it from the current working doc
    if (blob.length() == 0) {
        // Break the association between blob and doc
        delDocBlobUsage(conn, docId);

        if (!isBlobVersioned(conn, blobId))
            // Nothing else references this blob, delete the blob itself
            delBlob(conn, blobId, docId);

        // Nothing more to do.  No more blobId.
        return 0;
    }

    // If this doc already has a blob, is it different from passed blob?
    bool blobIsNew = true;
    if (blobId) {
        cdr::Blob *pBlob = cdr::getBlobPtrByBlobId(conn, blobId);
        if (pBlob) {
            blobIsNew = pBlob->compare(blob) ? true : false;
            delete pBlob;
        }
    }

    if (blobIsNew) {
        if (blobId && !isBlobVersioned(conn, blobId))
            // Blob is different, but it was never versioned
            // We replace the blob that was there.  Old one disappears.
            replaceBlob(conn, blob, docId, blobId);
        else
            // Even if this doc had a blob, it now gets a new one
            // But we can't overwrite the old one because it's versioned
            blobId = addBlob(conn, blob, docId);
    }

    return blobId;
}
