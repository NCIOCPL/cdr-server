/*
 * $Id: CdrVersion.h,v 1.1 2000-10-26 15:31:41 mruben Exp $
 *
 * Internal support functions for CDR verison control
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifndef CDR_VERSION_
#define CDR_VERSION_

#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Checks in a document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  usr         int user ID
     *  @param  comment     cdr::String* comment on check in.  If NULL, the
     *                      comment will not be updated
     *  @param  abandon     bool true if checkout is to be abandoned (i.e.,
     *                      new version not stored).  Note that this does
     *                      not replace edits that have been made.
     *  @param  force       bool true if checkin is to be forced even if
     *                      if not checked out by usr.
     *
     *  @return             int version number of new version.  -1 if
     *                      new version not stored because no change
     *
     */
    extern int checkIn(int                     docId,
                       cdr::db::Connection&    conn,
                       int                     usr,
                       const cdr::String*      comment = NULL,
                       bool                    abandon = false,
                       bool                    force = false);

    /**
     * Checks out a document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  usr         int user ID
     *  @param  comment     cdr::String comment
     *  @param  force       bool true if checkout is to be forced even if
     *                      checked checked out by another user.
     *
     *  @return             int version number of version checked out.  0
     *                      if no version has been checked in.  Note that
     *                      this is not an error.
     *
     */
    int checkOut(int                    docId,
                 cdr::db::Connection&   conn,
                 int                    usr,
                 const cdr::String&     comment = L"",
                 bool                   force = false);

    struct CdrVerDoc
    {
        CdrVerDoc() : document(0), num(0), usr(0), doc_type(0) {}

        int                 document;
        int                 num;
        cdr::String         dt;
        cdr::String         updated_dt;
        int                 usr;
        int                 doc_type;
        cdr::String         title;
        cdr::String         xml;
        cdr::Blob           data;
        cdr::String         comment;
    };

    /**
     * Gets version of document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  num         int version number to retrieve.
     *  @param  docver      pointer to CdrVerDoc.  If not NULL, information
     *                      stored there.
     *
     *  @return             true if version found.  false if not.
     *
     */
    bool getVersion(int                    docId,
                    cdr::db::Connection&   conn,
                    int                    num,
                    CdrVerDoc*             verdoc = 0);

    /**
     * Gets version of document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  dt          cdr::String date.  Get's version as of that date
     *  @param  docver      pointer to CdrVerDoc.  If not NULL, information
     *                      stored there.
     *
     *  @return             true if version found.  false if not.
     *
     */
    bool getVersion(int                    docId,
                    cdr::db::Connection&   conn,
                    const cdr::String&     dt,
                    CdrVerDoc*             verdoc = 0);

    /**
     * Gets labeled version of document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  label       cdr::String label name
     *  @param  docver      pointer to CdrVerDoc.  If not NULL, information
     *                      stored there.
     *
     *  @return             true if version found.  false if not.
     *
     */
    bool getLabelVersion(int                    docId,
                         cdr::db::Connection&   conn,
                         const cdr::String&     label_name,
                         CdrVerDoc*             verdoc = 0);

    /**
     * Gets version number of document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  date        pointer to cdr::String.  If not null, date of
     *                      most recent checked in version is stored.
     *
     *  @return             current version number.  -1 if document does
     *                      not exist
     *
     */
  int getVersionNumber(int                    docId,
                       cdr::db::Connection&   conn,
                       cdr::String* date = 0);

    /**
     * Checks if document has changed since last version in control
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *
     *  @return             true if version has changed, false if unchanged
     *
     */
  bool isChanged(int docId,
                 cdr::db::Connection& conn);

    /**
     * Checks if document is checked out
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  usr         pointer to int.  If not null, user who has
     *                      checked out document stored.
     *  @param  dt_out      pointer to cdr::String.  If not null, date
     *                      checked out stored.
     *
     *  @return             true if version has changed, false if unchanged
     *
     */
  bool isCheckedOut(int                     docId,
                    cdr::db::Connection&    conn,
                    int*                    usr = 0,
                    cdr::String*            dt_out = 0);
}

#endif
