/*
 * $Id: CdrGetDoc.h,v 1.1 2000-05-03 15:41:09 bkline Exp $
 *
 * Internal support functions for CDR document retrieval.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_GET_DOC_
#define CDR_GET_DOC_

#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    extern cdr::String getDocString(const cdr::String&, cdr::db::Connection&);

}

#endif
