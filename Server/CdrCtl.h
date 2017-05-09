/**
 * CdrCtl.h
 *
 * Header file for manipulating and retrieving data from the ctl table
 * in the cdr database.
 *
 *                                          Alan Meyer  August, 2013
 */

#ifndef CDR_CTL_
#define CDR_CTL_

#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * Set a value into the control table.
     *
     * ASSUMPTIONS:
     *    All parameter validation has been done and has passed.
     *
     *  @param conn         Active database connection.
     *  @param action        "Install", "Create", or "Inactivate".
     *                      If Install subsequent parms are ignored.
     *  @param group        String grouping values together, '|' not allowed.
     *                       Required for Create, Inactivate.
     *  @param key          String key or name of value.
     *                       Required for Create, Inactivate
     *  @param value        String control value, required.
     *                       Required for Create, else ignored.
     *                       Pass a number as a string, e.g., L"123".
     *  @param comment      String - always optional.
     *
     *  @throws cdr::Exception if database error or bad database table state
     */
    void setOneCtlValue(
        cdr::db::Connection& conn,
        const cdr::String& action,
        const cdr::String& group,
        const cdr::String& key,
        const cdr::String& value,
        const cdr::String& comment);

    /**
     * Install all active values in the ctl table into an in-memory map
     * for rapid access.
     *
     *  @param conn         Active database connection.
     *
     *  @return             Void.
     *
     *  @throws             Exception if no data to load or underlying
     *                      DB interface exception.
     */
    void loadCtlTableIntoMemory(cdr::db::Connection &conn);

    /**
     * Read a value from the in memory control table.
     *
     *  @param group        Group name.
     *  @param key          Name of the specific key within the group.
     *
     *  @return             Stored value, returned as a cdr::String.
     *                      L""  = group/key not found.
     *
     * Note: It is illegal in this module to store a group + key with no
     *       value.  setCtlTableValues() enforces that.  If such a row
     *       gets into the ctl table, a search for it will return the
     *       same thing as if the group + key were not in the table at all.
     */
    cdr::String getCtlValue(const cdr::String group, const cdr::String key);
}

#endif // CDR_DOC_
