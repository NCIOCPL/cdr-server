/*
 * $Id: CdrFilter.h,v 1.1 2001-09-20 21:44:55 ameyer Exp $
 *
 * Internal support functions for CDR filter
 *
 * $Log: not supported by cvs2svn $
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
     * Filters a document
     *
     *  @param  document    cdr::String document to be filtered
     *  @param  filter      cdr::String filter to be applied
     *  @param  connection  reference to an active connection to the CDR
     *                      database.
     *  @param  messages    cdr::String*.  If not null, nonerror messages 
     *                      from the filter are placed (as XML) in this string
     *  @param  parms       vector of name-value pairs of parameters
     *
     *  @return             cdr::String filtered document
     *
     */
    extern cdr::String filterDocument(const cdr::String&     document,
                                      const cdr::String&     filter,
                                      cdr::db::Connection&   connection,
                                      cdr::String*           messages = 0,
                                      cdr::FilterParmVector* parms = 0);

}

#endif
