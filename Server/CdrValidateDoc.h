/*
 * Support routines for CDR document validation.
 */

#ifndef CDR_VALIDATE_DOC_
#define CDR_VALIDATE_DOC_

#include "CdrString.h"
#include "CdrDoc.h"
#include "CdrDom.h"
#include "CdrDbConnection.h"
#include "CdrValidationCtl.h"

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
        UpdateUnconditionally,  // Update database regardless of errors
        UpdateLinkTablesOnly    // Don't validate links, do update link tables
    };


    /**
     * Front end function to all validation.
     * Called by validateDoc to perform validation on behalf of a client.
     * Also called by CdrPutDoc when adding or replacing a document in
     *   the database.
     *
     *  @param  docObj          Reference to CdrDoc object to validate.
     *                          Actual error messages are stored in this
     *                          object.
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
     *                          Additional types may be added later.
     *                          The parameter string is searched for the above
     *                          substrings, so any separators or other
     *                          characters in the parameter are ignored.
     *
     *  @return                 Validation status of document:
     *                            L"V"alid
     *                            L"I"nvalid
     *                            L"M"alformed
     *
     *  @exception cdr::Exception  if database error encountered, no
     *                          permission to validate, etc.
     */
    extern cdr::String execValidateDoc (cdr::CdrDoc*, cdr::ValidRule,
                                        const cdr::String& = L"");


    /**
     * Checks document against the schema for its document type, reporting any
     * errors found in the caller's <code>Errors</code> vector.  Caller is
     * responsible for ensuring that the docTypeName is correct for this
     * document, and that the current user is authorized to invoke the
     * document validation.
     *
     *  @param  docElem             top level element for CdrDoc
     *  @param  docTypeName         string for name of document's type
     *  @param  dbConnection        reference to CDR database connection object
     *  @param  errors              vector of strings to be populated by this
     *                              function
     *
     *  @exception  cdr::Exception  if database error encountered
     */
    extern void validateDocAgainstSchema(
            cdr::dom::Element&         docElem,
            const String&              docTypeName,
            db::Connection&            dbConnection,
            ValidationControl&         errCtl);
}

#endif
