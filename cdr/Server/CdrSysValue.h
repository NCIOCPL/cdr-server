/*
 * CdrSysValue.h
 *
 * Header file for adding, replacing, deleting, retrieving sys_value
 * table rows.
 *
 * There is no sys_value class, just procedural functions.
 *
 *                                          Alan Meyer, January, 2003
 *
 * $Id: CdrSysValue.h,v 1.1 2003-01-31 00:06:53 ameyer Exp $
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_SYS_VALUE_
#define CDR_SYS_VALUE_

#include "CdrSession.h"
#include "CdrString.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Create a new system value.
     *
     *  @param  session     Active session identifier.
     *  @param  conn        Active database connection.
     *  @param  name        Name of the new value, must be unique in db.
     *  @param  value       String value to associate with the name.
     *  @param  notes       Optional documentation.
     *                       Empty string = NULL in database.
     *  @param  program     Optional name of program setting the value.
     *                       Empty string = NULL in database.
     *  @return void        Throws exception if error.
     */
    void addSysValue (
       cdr::Session& session,
       cdr::db::Connection& conn,
       const cdr::String& name,
       const cdr::String& value,
       const cdr::String& notes,
       const cdr::String& program);

    /**
     * Replace an existing system value.
     *
     *  @param  session     Active session identifier.
     *  @param  conn        Active database connection.
     *  @param  name        Name of the value to replace, must exist.
     *  @param  value       String value to replace existing value.
     *  @param  notes       Optional documentation.
     *                       Empty string => Leave notes alone
     *  @param  program     Optional name of program setting the value.
     *                       Empty string => NULL in database.
     *  @return void        Throws exception if error.
     */
    void repSysValue (
       cdr::Session& session,
       cdr::db::Connection& conn,
       const cdr::String& name,
       const cdr::String& value,
       const cdr::String& notes = L"",
       const cdr::String& program = L"");

    /**
     * Delete an existing system value.
     *
     *  @param  session     Active session identifier.
     *  @param  conn        Active database connection.
     *  @param  name        Name of the entry to delete.  Must exist.
     *  @return void        Throws exception if error.
     */
    void delSysValue (
       cdr::Session& session,
       cdr::db::Connection& conn,
       const cdr::String& name);

    /**
     * Retrieve an existing system value.
     *
     *  @param  session     Active session identifier.
     *  @param  conn        Active database connection.
     *  @param  name        Name of the entry to delete.  Must exist.
     *  @return             Value associated with the passed name.
     *                       If value does not exist, NULL cdr::String returned.
     *                       Whether this is an error or not is up to the
     *                       calling program.
     */
    cdr::String getSysValue (
       cdr::Session& session,
       cdr::db::Connection& conn,
       const cdr::String& name);
}

#endif // ifndef CDR_SYS_VALUE_
