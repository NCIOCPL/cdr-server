/*
 * $Id: CdrValidateDoc.h,v 1.2 2000-10-05 15:19:57 ameyer Exp $
 *
 * Support routines for CDR document validation.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/05/03 15:42:00  bkline
 * Initial revision
 *
 */

#ifndef CDR_VALIDATE_DOC_
#define CDR_VALIDATE_DOC_

#include "CdrString.h"
#include "CdrDoc.h"
#include "CdrDom.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Validation status values.
     */
    const char ValidStatus       = 'V';
    const char InvalidStatus     = 'I';
    const char UnvalidatedStatus = 'U';

    /**
     * Validation / update options
     */
    const enum ValidRule {
        ValidateOnly,           // Validate doc but leave database alone
        UpdateIfValid,          // Update database iff no errors
        UpdateUnconditionally   // Update database regardless of errors
    };


    /**
     * Front end function to all validation.
     * Called by validateDoc to perform validation on behalf of a client.
     * Also called by CdrPutDoc when adding or replacing a document in
     *   the database.
     *
     *  @param  docObj          Reference to CdrDoc object to validate
     *  @param  dbUpdate        Instructions pertaining to database update
     *                          ValidateOnly - Do not update status/link info.
     *                          UpdateIfValid - Update info if doc valid
     *                          UpdateUnconditionally - Update status/link
     *  @param  validationTypes Reference to string containing a space-
     *                          delimited list of validation methods to be
     *                          performed on the document (for example,
     *                          "Schema Links"); if the string is empty (the
     *                          default) then all available validation methods
     *                          are to be invoked; currently "Schema" and
     *                          "Links" are the available validation methods;
     *                          additions to this list should be documented
     *                          here.
     *  @param  schemaOnly      skip link validation if true
     *  @param  linksOnly       skip schema validation if true
     *
     *  @return                 Error messages in a single XML string
     *
     *  @exception cdr::Exception  if database error encountered, no
     *                          permission to validate, etc.
     */
    extern cdr::String execValidateDoc (cdr::CdrDoc&, cdr::ValidRule,
                                        const cdr::String& = L"");


    /**
     * Checks document against the schema for its document type, reporting any
     * errors found in the caller's <code>Errors</code> vector.  Caller is
     * responsible for ensuring that the docTypeName is correct for this
     * document, and that the current user is authorized to invoke the
     * document validation.
     *
     *  @param  docElem             Top level element for CdrDoc element
     *  @param  docTypeName         string for name of document's type
     *  @param  dbConnection        reference to CDR database connection object
     *  @param  errors              vector of strings to be populated by this
     *                              function
     *  @param  updateDocStatus     if <code>true</code> update the doc_status
     *                              column of the document table
     *
     *  @exception  cdr::Exception  if database error encountered
     */
    extern void validateDocAgainstSchema(
            cdr::dom::Element&         docElem,
            const String&              docTypeName,
            db::Connection&            dbConnection,
            StringList&                errors);
}

#endif
