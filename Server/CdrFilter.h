/*
 * Internal support functions for CDR filter
 */

#ifndef CDR_FILTER_
#define CDR_FILTER_

#include <utility>
#include <vector>

#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrVersion.h"

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
     * LIMITATION:
     *   The filter is in the document table.  There is no ability to
     *   get a version of a filter, or limit the version to a particular
     *   maximum date.
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
     *  @throws             Database or filter exceptions from lower down.
     */
    extern cdr::String filterDocumentByScriptId (
                                      const cdr::String&     document,
                                      int                    filterId,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String            doc_id = "");

    /**
     * Filters a document accepting a document title for the filter instead
     * of the actual filter script.
     *
     * Loads the script via the title and calls the regular version.
     *
     * LIMITATION:
     *   The filter is in the document table.  There is no ability to
     *   get a version of a filter, or limit the version to a particular
     *   maximum date.
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  filterTitle filter title in document table
     *  @param  connection  reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       Pointer to vector of name-value pairs of parameters
     *  @param  doc_id      CDR identifier of document
     *
     *  @return             cdr::String filtered document
     *
     *  @throws             Database or filter exceptions from lower down.
     */
    extern cdr::String filterDocumentByScriptTitle (
                                      const cdr::String&     document,
                                      const cdr::String&     filterTitle,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String            doc_id = "");

    /**
     * Filters a document accepting the name of a filter set.
     *
     * Loads the scripts via the set name and calls lower level version.
     *
     * LIMITATION:
     *   The filter is in the document table.  There is no ability to
     *   get a version of a filter, or limit the version to a particular
     *   maximum date.
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  setName     Name of filter set in document table
     *  @param  version     Version number (string) or symbolic name, or empty
     *  @param  connection  Reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       Pointer to vector of name-value pairs of parameters
     *  @param  doc_id      CDR identifier of document
     *
     *  @return             cdr::String filtered document
     *
     *  @throws             Database or filter exceptions from lower down.
     */
    extern cdr::String filterDocumentByScriptSetName (
                                      const cdr::String&     document,
                                      const cdr::String&     setName,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      const cdr::String&     version = "",
                                      cdr::FilterParmVector* parms = 0,
                                      cdr::String            doc_id = "");


    /**
     * Build a map of filter strings to IDs to use in timing filters
     * by filter id.
     *
     * The map is only built if the environment variable CDR_FILTER_PROFILING
     * is defined.  The actual code to build the map is in CdrFilter.cpp
     * rather than CdrTiming.cpp because it is highly specific to filtering.
     *
     * @throws      Database exceptions from lower down.
     */
    extern void buildFilterString2IdMap();
}

#endif
