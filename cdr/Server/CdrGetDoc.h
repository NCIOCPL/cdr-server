/*
 * $Id: CdrGetDoc.h,v 1.2 2000-05-04 01:06:06 bkline Exp $
 *
 * Internal support functions for CDR document retrieval.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/05/03 15:41:09  bkline
 * Initial revision
 *
 */

#ifndef CDR_GET_DOC_
#define CDR_GET_DOC_

#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Pulls the components of a CDR document from the database and constructs
     * a serializable XML version of it.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&      docId, 
                                    cdr::db::Connection&    conn);

}

#endif
