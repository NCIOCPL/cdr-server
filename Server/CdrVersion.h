/*
 * $Id: CdrVersion.h,v 1.11 2006-09-19 22:29:00 ameyer Exp $
 *
 * Internal support functions for CDR verison control
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2006/09/01 02:09:51  ameyer
 * Added isCWDLastPub.
 *
 * Revision 1.9  2006/05/17 03:35:14  ameyer
 * Support for date limited version retrieval.
 *
 * Revision 1.8  2004/11/05 05:59:01  ameyer
 * Added some blob related information.
 *
 * Revision 1.7  2002/06/07 13:52:41  bkline
 * Added support for last publishable linked document retrieval.
 *
 * Revision 1.6  2001/06/05 20:48:25  mruben
 * changed to maintain publishable flag on version
 *
 * Revision 1.5  2001/05/23 01:27:50  ameyer
 * Added actStatus parameter to checkIn prototype.
 *
 * Revision 1.4  2001/05/22 21:29:25  mruben
 * added status information to CdrVerDoc
 *
 * Revision 1.3  2000/12/07 16:02:08  ameyer
 * Made allowVersion() a public function, needed by delDoc().
 *
 * Revision 1.2  2000/10/31 15:48:15  mruben
 * changed interface to include session
 *
 * Revision 1.1  2000/10/26 15:31:41  mruben
 * Initial revision
 *
 *
 */

#ifndef CDR_VERSION_
#define CDR_VERSION_

#include "CdrString.h"
#include "CdrSession.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Default date limit for retrieving version of documents,
     * set far into the future.  The default retrieval is therefore,
     * in effect, the current date.
     */
    const cdr::String DFT_VERSION_DATE = L"9000-01-01";

    /**
     * Checks in a document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  usr         int user ID
     *  @param  publishable Y or N
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
    extern int checkIn(cdr::Session&           session,
                       int                     docId,
                       cdr::db::Connection&    conn,
                       cdr::String             actStatus,
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
    int checkOut(cdr::Session           &session,
                 int                    docId,
                 cdr::db::Connection&   conn,
                 const cdr::String&     comment = L"",
                 bool                   force = false);

    struct CdrVerDoc
    {
        CdrVerDoc() : document(0),num(0),usr(0),blob_id(true),doc_type(0) {}

        int                 document;
        int                 num;
        cdr::String         dt;
        cdr::String         updated_dt;
        int                 usr;
        cdr::String         val_status;
        cdr::String         val_date;
        cdr::String         approved;
        int                 doc_type;
        cdr::String         title;
        cdr::String         xml;
        cdr::Int            blob_id;
        cdr::Blob           data;
        cdr::String         comment;
        cdr::String         publishable;
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
     * Gets "last" version number of document, i.e., the highest numbered
     * version, whether publishable or not.
     *
     * If caller passes a date-time limit, the number of the last version
     * updated on or before the passed date-time is retrieved.
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  date        pointer to cdr::String.  If not null, date of
     *                      most recent checked in version is stored.
     *  @param  maxDate     reference to cdr::String.  It's a date-time
     *                      in ISO "YYYY-MM-DD ..." format.
     *                      It may be right truncated.
     *                      The retrieved version must have been updated
     *                      before this date.
     *
     *  @return             current version number.  -1 if document does
     *                      not exist
     *
     */
  int getVersionNumber(int                  docId,
                       cdr::db::Connection& conn,
                       cdr::String*         date = 0,
                       const cdr::String&   maxDate = DFT_VERSION_DATE);

    /**
     * Gets version number of latest publishable version of document.
     *
     * May be date limited, as for getVersionNumber().
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  date        pointer to cdr::String.  If not null, date of
     *                      most recent checked in version is copied here.
     *                      Default = do not fetch date.
     *  @param  maxDate     reference to cdr::String.  It's a date-time
     *                      in ISO "YYYY-MM-DD ..." format.
     *                      It may be right truncated.
     *                      The retrieved version must have been updated
     *                      before this date.
     *                      Default = no date limit.
     *
     *  @return             current version number.  -1 if document does
     *                      not exist
     *
     */
  int getLatestPublishableVersion(int                  docId,
                                  cdr::db::Connection& conn,
                                  cdr::String*         date = 0,
                                  const cdr::String&   maxDate =
                                                       DFT_VERSION_DATE);

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

    /**
     * Determine whether the current working and the last publishable
     * versions of a document are the same.  They are the same if and
     * only if a publishable version exists and its updated_dt value
     * is the same as the dt value in the audit_trail for the last
     * add or modify transaction.
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *
     *  @return             true if CWD == last pub version, else false
     */
  bool isCWDLastPub(int                     docId,
                    cdr::db::Connection&    conn);


    /**
     * Checks if versioning is supported for this document
     *
     *  @param  docId       int document ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *
     *  @return             true if versioning is allowed.
     *
     */
  bool allowVersion(int                     docId,
                    cdr::db::Connection&    conn);

    /**
     * Instantiate the data blob in a CdrVerDoc if and only if the
     * blob_id is not null.
     *
     *
     *  @param conn         Database connection.
     *  @param verdoc       Pointer to structure containing blob_id.
     *  @return             Void, but verdoc will contain data if need
     *  @throw              cdr::Exception if blob_id but no blob.
     */
    void getVerBlob(cdr::db::Connection& conn, CdrVerDoc* verdoc);
}

#endif