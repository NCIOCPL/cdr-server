/*
 * $Id: CdrGetDoc.h,v 1.11 2002-07-03 12:55:59 bkline Exp $
 *
 * Internal support functions for CDR document retrieval.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.10  2002/06/28 20:40:44  ameyer
 * Made docVer parameter constant on call to getDocString()
 *
 * Revision 1.9  2002/01/23 18:24:12  mruben
 * added new optional components for CdrDocCtl
 *
 * Revision 1.8  2002/01/08 18:18:19  mruben
 * added support for denormalization and getting control information
 *
 * Revision 1.7  2001/04/05 23:58:50  ameyer
 * Added default value to last arg on overloaded getDocString().
 *
 * Revision 1.6  2001/03/13 22:05:54  mruben
 * modified to support filtering on CdrDoc
 *
 * Revision 1.5  2000/10/27 02:34:12  ameyer
 * Added version retrieval and control.
 *
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
     *  @param  usecdata    if true, document is returned as CDATA.
     *  @param  denormalize if true, link denormalization filter is applied.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&   docId,
                                    cdr::db::Connection& conn,
                                    bool usecdata = true,
                                    bool denormalize = true);

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
     *  @param  usecdata    if true, document is returned as CDATA.
     *  @param  denormalize if true, link denormalization filter is applied.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&    docId,
                                    cdr::db::Connection&  conn,
                                    const struct cdr::CdrVerDoc *verDoc,
                                    bool usecdata = true,
                                    bool denormalize = true);

    /**
     * Overloaded version of getDocString which pulls a previous version
     * of the document from the version control system instead of the
     * document table using version number.
     *
     * Not all control elements will be the same.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  version     version number
     *  @param  usecdata    if true, document is returned as CDATA.
     *  @param  denormalize if true, link denormalization filter is applied.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&    docId,
                                    cdr::db::Connection&  conn,
                                    int version,
                                    bool usecdata = true,
                                    bool denormalize = true);

    /**@#-*/

    namespace DocCtlComponents
    {
    /**@#+*/

      /**
       * The components of CdrCtl
       */
      enum { all                 = 0xffff,
             DocValStatus        = 0x0001,
             DocValDate          = 0x0002,
             DocTitle            = 0x0004,
             DocComment          = 0x0008,
             DocActiveStatus     = 0x0010,
             std                 = 0x001f,
             DocCreate           = 0x0020,  // date & user
             DocMod              = 0x0040,  // date & user
             DocFirstPub         = 0x0080,  // date & user
      };
    }

    /**
     * Pulls the control components of a CDR document from the database
     * and constructs a serializable XML version of it.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  elements    bit mask selecting components to include.  Note
     *                      that this specifies the minimum to be included.
     *                      Additional elements may be included.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocCtlString(const cdr::String&      docId,
                                       cdr::db::Connection&    conn,
                                       int elements
                                           = cdr::DocCtlComponents::std);

    /**
     * Pulls the control components of a CDR document from the database
     * and constructs a serializable XML version of it.
     *
     *  @param  docId       reference to string containing the document's ID.
     *  @param  version     int version number of document
     *  @param  conn        reference to an active connection to the CDR
     *                      database.
     *  @param  elements    bit mask selecting components to include
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocCtlString(const cdr::String&      docId,
                                       int                     version,
                                       cdr::db::Connection&    conn,
                                       int elements
                                           = cdr::DocCtlComponents::std);

}

#endif
