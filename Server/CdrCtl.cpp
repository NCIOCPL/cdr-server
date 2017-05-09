/**
 * CdrCtl.cpp
 *
 * Implementation of ctl table management.
 *
 *                                          Alan Meyer  August, 2013
 *
 * OUCH!  This is a significant duplication of the CdrSysValue.cpp function
 *        implemented (by me!) in 2003 and never used.
 */

#include "CdrCommand.h"
#include "CdrCtl.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include <string>

// Type of the in-memory ctl table map
typedef std::map<cdr::String, cdr::String> CtlMap;

// Spinlock control variable, shared by all threads
static volatile bool s_ctlUpdateInProgress = false;

// Map of group/key = value, shared by all threads
static CtlMap *s_ctlMap;

// Map loader waits this many milliseconds after setting his lock
const int loadMapDelay = 500;

// Threads wait this many cycles before giving up and declaring error
const int loadMapWaitTries = 10;

// Group|key separator
const cdr::String gkSep = L"|";

// Forward declaration
static void spin(cdr::String callerName);

// See CdrCommand.h
cdr::String cdr::setCtlValues(
    cdr::Session& session,
    const cdr::dom::Node& node,
    cdr::db::Connection& conn
) {
    cdr::String action,         // What we're going to do
                group,          // Control table group
                key,            // Key within group
                value,          // Value for group / key
                comment,        // Optional comment for table row
                errMsg;         // Put error messages here
    cdr::dom::Node ctlChild,    // High level node within command
                   child;       // Lower level nodw within command

    // User authorized?
    if (!session.canDo (conn, L"SET_SYS_VALUE", L""))
         throw cdr::Exception (L"setCtlValue: User '" + session.getUserName()
                               + L"' not authorized to set ctl values");

    // Response element name
    static cdr::String setCtlRespStr = L"CdrSetCtl";

    // Parse the command
    ctlChild = node.getFirstChild();
    while (ctlChild != 0) {
        if (ctlChild.getNodeType() == cdr::dom::Node::ELEMENT_NODE &&
            ctlChild.getNodeName() == L"Ctl") {

            // Eliminate any values left over from last Ctl
            action  = L"";
            group   = L"";
            key     = L"";
            value   = L"";
            comment = L"";
            errMsg  = L"";

            // We have an element defining a row in the table.  Get parts
            child = ctlChild.getFirstChild();
            while (child != 0) {
                if (child.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                    cdr::String name    = child.getNodeName();
                    cdr::String content = cdr::dom::getTextContent(child);

                    // Action + Row content: Group, Key, Value, Comment
                    if (name == L"Action")
                        action = content;
                    else if (name == L"Group")
                        group = content;
                    else if (name == L"Key")
                        key = content;
                    else if (name == L"Value")
                        value = content;
                    else if (name == L"Comment")
                        comment = content;
                }
                child = child.getNextSibling ();
            }

            // Check parameters
            if (action == L"Create" || action == L"Inactivate") {
                // Group is required, no separator allowed
                if (group.empty())
                    errMsg += cdr::tagWrap(
                            L"Missing Group element in command", L"Err");
                else if (group.find(gkSep) != group.npos)
                    errMsg +=  cdr::tagWrap(
                            L"Group contains reserved char '" + gkSep + L"'",
                            L"Err");

                // Key is required, no separator allowed
                if (key.empty())
                    errMsg += cdr::tagWrap(
                            L"Missing Key element in command", L"Err");
                else if (key.find(gkSep) != key.npos)
                    errMsg += cdr::tagWrap(
                            L"Key contains reserved char " + gkSep + L"'",
                            L"Err");

                // Create (but not Inactivate) requires a value
                if (action == L"Create" && value.empty())
                    // Note: Do not allow group/key = "", asking for trouble
                    errMsg += cdr::tagWrap(
                            L"Missing Value element in command", L"Err");
            }
            // Only valid actions are Create, Inactivate, Install
            else if (action != L"Install")
                errMsg += cdr::tagWrap("Invalid action in command", L"Err");

            // Stop here if errors
            // Note: Some Ctl elements in transaction may already have been set
            if (!errMsg.empty())
                return cdr::tagWrap(errMsg, setCtlRespStr);

            // We got something, set the value
            cdr::setOneCtlValue(conn, action, group, key, value, comment);
        }

        ctlChild = ctlChild.getNextSibling ();
    }

    // Success
    return L"<" + setCtlRespStr + L"Resp/>";
}

// See CdrCtl.h
void cdr::setOneCtlValue(
    cdr::db::Connection& conn,
    const cdr::String& action,
    const cdr::String& group,
    const cdr::String& key,
    const cdr::String& value,
    const cdr::String& comment
) {
    const size_t EBSIZE = 400;
    wchar_t errBuf[EBSIZE];    // Assemble error message here

    // If creating or inactivating, we need to inactivate existing group+key
    if (action == L"Create" || action == L"Inactivate") {
        // Transaction control
        bool autoCommitted = conn.getAutoCommit();
        conn.setAutoCommit(false);

        std::string updQry =
           "UPDATE ctl"
           "   SET inactivated = GETDATE()"
           " WHERE grp = ?"
           "   AND name = ?"
           "   AND inactivated IS NULL";
        cdr::db::PreparedStatement updStmt = conn.prepareStatement(updQry);
        updStmt.setString (1, group);
        updStmt.setString (2, key);

        // Execute.  Should always be something when action == Inactivate
        int updCount = updStmt.executeUpdate();
        if (updCount != 1 && action == L"Inactivate") {
            swprintf(errBuf, EBSIZE,
                 L"Inactivating a ctl table row should always affect "
                 L"exactly one row.  This update affected %d rows", updCount);
            throw cdr::Exception(errBuf);
        }

        // Insert a new row if required
        if (action == L"Create") {
            std::string insQry =
                "INSERT ctl "
                "       (grp, name, val, comment, created) "
                "VALUES (?, ?, ?, ?, GETDATE())";
            cdr::db::PreparedStatement insStmt = conn.prepareStatement(insQry);
            insStmt.setString (1, group);
            insStmt.setString (2, key);
            insStmt.setString (3, value);
            if (comment.empty()) {
                // Use this to insert a null in the DB in place of L""
                cdr::String nullStr(true);
                insStmt.setString (4, nullStr);
            }
            else
                insStmt.setString (4, comment);

            int insCount = insStmt.executeUpdate();
            if (insCount != 1) {
                // Cancel transaction
                conn.rollback();
                conn.setAutoCommit(autoCommitted);

                swprintf(errBuf, EBSIZE,
                    L"Error inserting ctl row.  %d rows were inserted",
                    insCount);
                throw cdr::Exception(errBuf);
            }
        }

        // Complete transaction
        conn.commit();
        conn.setAutoCommit(autoCommitted);
    }

    else if (action == L"Install")
        return cdr::loadCtlTableIntoMemory(conn);
}

// See CdrCtl.h
void cdr::loadCtlTableIntoMemory(
    cdr::db::Connection& conn
) {
    // Create a new map here
    CtlMap *newMap;

    // Hold the existing map here prior to release
    CtlMap *oldMap;

    // If another load is in progress, hold off
    if (s_ctlUpdateInProgress)
        spin(L"loadCtlTableIntoMemory");

    // Load all active rows from the database
    std::string selQry =
        "SELECT grp, name, val "
        "  FROM ctl "
        " WHERE inactivated IS NULL "
        " ORDER BY grp, name";

    cdr::db::PreparedStatement selStmt = conn.prepareStatement(selQry);
    cdr::db::ResultSet rs = selStmt.executeQuery();

    // Allocate a new map and fill it
    newMap = new CtlMap;

    // Load any rows found, else empty map is okay
    if (rs.next()) {
        do {
            // From the database
            cdr::String group = rs.getString(1);
            cdr::String key   = rs.getString(2);
            cdr::String value = rs.getString(3);

            // Combine group and key to make a map key
            cdr::String mapKey = group + gkSep + key;

            newMap->insert(make_pair(mapKey, value));
        } while (rs.next());
    }

    // Set the spinlock to halt all the readers
    s_ctlUpdateInProgress = true;

    // Wait a second for any threads to complete ops in progress
    // In theory, this could screw up but it should be a cold day in hell
    //  if it does
    Sleep(loadMapDelay);

    // Swap the maps
    oldMap   = s_ctlMap;
    s_ctlMap = newMap;

    // Done with the lock
    s_ctlUpdateInProgress = false;

    // Free the old map, we've replaced it
    if (oldMap != NULL)
        delete oldMap;
}

// See CdrCtl.h
cdr::String cdr::getCtlValue(
    const cdr::String group,
    const cdr::String key
) {
    // Proper initialization done?
    if (s_ctlMap == NULL)
        throw cdr::Exception("getCtlValue: s_ctlMap uninitialized");

    // Combine group and key to form a map key
    cdr::String mapKey = group + gkSep + key;

    // If a load is in progress wait for it to finish
    // If another load is in progress, hold off
    if (s_ctlUpdateInProgress)
        spin(L"getCtlValue");

    // Search the map
    CtlMap::iterator it;
    it = s_ctlMap->find(mapKey);

    // Return value or not found
    if (it == s_ctlMap->end())
        return L"";
    return it->second;
}

/**
 * Wait for the update in progress variable to be reset to false.
 *
 * The wait is timeout limited.  Locks should only last milliseconds at most.
 * If the last a lot longer than that, something has likely gone wrong in
 * the locking process.  Log the problem, unlock the variable, and continue.
 *
 *  @param callerName   Names the calling routine for the error log.
 */
static void spin(cdr::String callerName) {

    // If a(nother) load is in progress, hold off until it's done
    int tries = 0;
    while (s_ctlUpdateInProgress) {
        if (++tries >= loadMapWaitTries) {
            // Turn off the lock, something went wrong with the locker
            s_ctlUpdateInProgress = true;

            // Log what happened
            cdr::log::pThreadLog->Write(callerName,
                    L"Timeout waiting for s_ctlUpdateInProgress unlock.  "
                    L"Unlocking and continuing.");
            break;
        }
        Sleep(loadMapDelay);
    }
}
