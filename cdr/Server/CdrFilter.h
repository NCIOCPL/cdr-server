/*
 * $Id: CdrFilter.h,v 1.3 2002-01-08 18:19:12 mruben Exp $
 *
 * Internal support functions for CDR filter
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/09/21 03:46:40  ameyer
 * Added filterDocumentByScriptId and fitlerDocumentByScriptTitle
 *
 * Revision 1.1  2001/09/20 21:44:55  ameyer
 * Initial revision
 *
 *
 *
 */

#ifndef CDR_FILTER_
#define CDR_FILTER_

#include <utility>
#include <vector>

#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

  typedef std::vector<std::pair<cdr::String, cdr::String> > FilterParmVector;

    /**
     * Filters a document.
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  filter      cdr::String filter to be applied
     *  @param  connection  reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       vector of name-value pairs of parameters
     *  @param  doc_id      CDR identifier of document
     *
     *  @return             cdr::String filtered document
     *
     */
    extern cdr::String filterDocument(const cdr::String&     document,
                                      const cdr::String&     filter,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String doc_id = "");

    /**
     * Filters a document accepting a document id for the filter instead
     * of the actual filter script.
     *
     * Loads the script via the ID and calls the regular version.
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  filterId    int filter identifier - key to document table
     *  @param  connection  reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       vector of name-value pairs of parameters
     *  @param  doc_id      CDR identifier of document
     *
     *  @return             cdr::String filtered document
     *
     */
    extern cdr::String filterDocumentByScriptId (
                                      const cdr::String&     document,
                                      int                    filterId,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String doc_id = "");

    /**
     * Filters a document accepting a document title for the filter instead
     * of the actual filter script.
     *
     * Loads the script via the title and calls the regular version.
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  filterTitle filter title in document table
     *  @param  connection  reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       vector of name-value pairs of parameters
     *  @param  doc_id      CDR identifier of document
     *
     *  @return             cdr::String filtered document
     *
     */
    extern cdr::String filterDocumentByScriptTitle (
                                      const cdr::String&     document,
                                      const cdr::String&     filterTitle,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String doc_id = "");

}

#endif
