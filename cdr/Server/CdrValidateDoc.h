/*
 * $Id: CdrValidateDoc.h,v 1.1 2000-05-03 15:42:00 bkline Exp $
 *
 * Support routines for CDR document validation.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_VALIDATE_DOC_
#define CDR_VALIDATE_DOC_

#include "CdrString.h"
#include "CdrDom.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Checks document against the schema for its document type, reporting any
     * errors found in the caller's <code>Errors</code> vector.  Caller is
     * responsible for ensuring that the docTypeName is correct for this
     * document, and that the current user is authorized to invoke the
     * document validation.
     *
     *  @param  docNode             DOM node for CdrDoc element
     *  @param  docTypeName         string for name of document's type
     *  @param  dbConnection        reference to CDR database connection object
     *  @param  errors              vector of strings to be populated by this
     *                              function
     *  @param  updateDocStatus     if <code>true</code> update the doc_status
     *                              column of the document table
     *
     *  @exception  cdr::Exception  if database or parsing error encountered
     */
    extern void validateDocAgainstSchema(
            const dom::Node&           docNode,
            const String&              docTypeName,
            db::Connection&            dbConnection,
            StringList&                errors,
            bool                       updateDocStatus);
}

#endif
