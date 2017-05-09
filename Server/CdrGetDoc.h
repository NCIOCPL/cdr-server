/*
 * Internal support functions for CDR document retrieval.
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
     *  @param  getXml      if true, fetch xml, else not.
     *  @param  getBlob     if true, fetch blob in base64, else not.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&   docId,
                                    cdr::db::Connection& conn,
                                    bool usecdata = true,
                                    bool denormalize = true,
                                    bool getXml = true,
                                    bool getBlob = true);

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
     *  @param  getXml      if true, fetch xml, else not.
     *  @param  getBlob     if true, fetch blob in base64, else not.
     *  @param  denormalize if true, link denormalization filter is applied.
     *  @param  maxDate     version must have been updated earlier than
     *                      this date.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&    docId,
                                    cdr::db::Connection&  conn,
                                    const struct cdr::CdrVerDoc *verDoc,
                                    bool usecdata = true,
                                    bool denormalize = true,
                                    bool getXml = true,
                                    bool getBlob = true);

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
     *  @param  getXml      if true, fetch xml, else not.
     *  @param  getBlob     if true, fetch blob in base64, else not.
     *  @return             wide-character String object containing XML
     *                      for the document.
     */
    extern cdr::String getDocString(const cdr::String&    docId,
                                    cdr::db::Connection&  conn,
                                    int version,
                                    bool usecdata = true,
                                    bool denormalize = true,
                                    bool getXml = true,
                                    bool getBlob = true,
                                    const cdr::String& maxDate=
                                               cdr::DFT_VERSION_DATE);

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

    /**
     * Just gets the document XML by itself, wrapped in a CDATA wrapper,
     * inside a CdrDocXml element.
     *
     * This is used as a subroutine by the getDocString() functions, but
     * can also be called by itself.
     *
     * @param xml           The XML string to wrap.
     *
     * @return              xml, wrapped in CdrDocXml/CDATA section.
     */
    extern cdr::String makeDocXml(const cdr::String& xml);

    /**
     * Finds the date the specified document was first published, if
     * known.
     *
     *  @param  docId       Integer document ID.
     *  @param  conn        Reference to an active connection to the CDR
     *                       database.
     *  @return             String containing date document was first
     *                       published, if known; otherwise NULL object.
     */
    extern cdr::String getDateFirstPublished(int, db::Connection&);

    /**
     * Gets the document type name for a document, by ID.
     *
     *  @param  docId       Reference to string containing the document's ID.
     *  @param  conn        Reference to an active connection to the CDR
     *                       database.
     *  @return             String containing docs doctype name.
     */
    extern cdr::String getDocTypeName(const cdr::String&, cdr::db::Connection&);
}

#endif
