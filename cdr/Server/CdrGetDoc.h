/*
 * $Id: CdrGetDoc.h,v 1.5 2000-10-27 02:34:12 ameyer Exp $
 *
 * Internal support functions for CDR document retrieval.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/10/17 21:22:10  ameyer
 * Added overloaded getDocString for versioned doc.
 *
 * Revision 1.3  2000/09/25 14:04:09  mruben
 * added getDocCtlString to get information from document table
 *
 * Revision 1.2  2000/05/04 01:06:06  bkline
 * Added ccdoc comments.
 *
 * Revision 1.1  2000/05/03 15:41:09  bkline
 * Initial revision
 *
 */

#ifndef CDR_GET_DOC_
#define CDR_GET_DOC_

#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrVersion.h"

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
    extern cdr::String getDocString(const cdr::String&   docId,
                                    cdr::db::Connection& conn);

    /**
     * Overloaded version of getDocString which pulls a previous version
     * of the document from the version control system instead of the
     * document table.
     *
     * Not all control elements will be the same.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  verDoc      ptr to struct filled out by version control.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&    docId,
                                    cdr::db::Connection&  conn,
                                    struct cdr::CdrVerDoc *verDoc);

    /**@#-*/

    namespace DocCtlComponents
    {
    /**@#+*/

      /**
       * The components of CdrCtl
       */
      enum { all = 0xffff,
             DocValStatus = 0x0001,
             DocValDate =   0x0002,
             DocTitle =     0x0004,
             DocComment =   0x0008
      };
    }

    /**
     * Pulls the control components of a CDR document from the database
     * and constructs a serializable XML version of it.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  select      bit mask selecting components to include
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocCtlString(const cdr::String&      docId,
                                       cdr::db::Connection&    conn,
                                       int elements
                                           = cdr::DocCtlComponents::all);

}

#endif
