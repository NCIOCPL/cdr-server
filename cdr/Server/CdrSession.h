/*
 * $Id: CdrSession.h,v 1.9 2000-10-30 17:42:57 mruben Exp $
 *
 * Information about the current login.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.8  2000/05/09 21:10:06  bkline
 * Replaced struct with class.
 *
 * Revision 1.7  2000/05/04 12:39:43  bkline
 * More ccdoc comments.
 *
 * Revision 1.6  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.5  2000/04/22 18:01:26  bkline
 * Fleshed out documentation comments.
 *
 * Revision 1.4  2000/04/21 14:02:06  bkline
 * Added canDo() method.
 *
 * Revision 1.3  2000/04/16 21:37:25  bkline
 * Changed member names, added new member variables and methods.
 *
 * Revision 1.2  2000/04/15 12:15:37  bkline
 * Added uName and uid members.
 *
 * Revision 1.1  2000/04/14 16:01:08  bkline
 * Initial revision
 */

#ifndef CDR_SESSION_
#define CDR_SESSION_

#include "CdrString.h"
#include "CdrDbConnection.h"

/**@#-*/

namespace cdr {

/**@#+*/

    /** @pkg cdr */

    /**
     * A <code>Session</code> object carries information about the currently
     * logged-on user, as well as the outcome of the last command.
     */
    class Session {

    public:

        /**
         * Creates an unopen <code>Session</code> object.
         */
        Session() { clear(); }

        /**
         * Clears the state for a session being closed.
         */
        void clear() {
            uid   = 0;
            id    = 0;
            name  = L"";
            uName = L"";
        }

        /**
         * Accessor method for the primary key of the row in the
         * <code>session</code> table of the CDR database for this session.
         *
         *  @return         integer representing the key for this session.
         */
        int getId() const { return id; }

        /**
         * Accessor method for the primary key of the row in the
         * <code>usr</code> table of the CDR database for the user account
         * used to create this session.
         *
         *  @return         integer representing the user ID for this session.
         */
        int getUserId() const { return uid; }

        /**
         * Accessor method for the <code>name</code> column of the
         * <code>usr</code> table for the row of the user account used to
         * create this session.
         *
         *  @return         logon string for the currently logged-on user.
         */
        String getUserName() const { return uName; }

        /**
         * Accessor method for the value used for the <code>Status</code> 
         * attribute of the <code>CdrResponse</code> element.  Initialized 
         * to "success" by the command dispatcher prior to invocation of 
         * the command handler for each command.  Can be modified (for 
         * example, to "warning") by the command handler if appropriate, using
         * the <code>setStatus()</code> method.  If an exception is caught 
         * by the command dispatcher, the attribute will be set to "error".
         *
         *  @return         string representing outcome of processing of
         *                  last command.
         */
        String getStatus() const { return lastStatus; }

        /**
         * Used by a command handler to change the status reflecting the
         * outcome of command processing from the default of L"success".
         *
         *  @param  status  string representing outcome of processing of 
         *                  last command.
         */
        void setStatus(const String& status) { lastStatus = status; }

        /**
         * Reports whether the session is open and valid.
         *
         *  @return         <code>true</code> iff the session is open.
         */
        bool isOpen() const { return !name.empty(); }

        /**
         * Used by the CDR server's command dispatcher to find the database
         * information for an existing session, based on the string generated
         * by the CdrLogon command to identify the session.
         *
         *  @throw  cdr::Exception if the session is not found or has expired.
         */
        void lookupSession(const String&, db::Connection&);

        /**
         * Used by the command dispatcher to note the time when each command
         * was received and handled for the session.  This information can be
         * used by the server to time out sessions which have been inactive
         * for a long time.
         *
         *  @param  conn            reference to active connection object
         *                          for CDR database, whose clock we use as
         *                          the reference for the current date and
         *                          time.
         */
        void setLastActivity(db::Connection& conn) const;

        /**
         * Returns <code>true</code> if the user account for the current
         * session is authorized to perform the specified action on the
         * specified document type.  For actions which are independent of
         * specific document types (such as ADD USER), the caller must pass an
         * empty (not a NULL) String.
         *
         *  @param  connection      reference to active connection object
         *                          for the CDR database, which we query to
         *                          determine whether the user can perform the
         *                          specified action.
         *  @param  action          name of the action to be checked.
         *  @param  docType         name of the document type for which the
         *                          action should be checked (or an empty
         *                          string for an action which is not
         *                          associated with any particular document
         *                          type.
         *  @return                 <code>true</code> iff the user is 
         *                          authorized to perform the specified
         *                          action.
         */
        bool canDo(db::Connection&    connection, 
                   const cdr::String& action, 
                   const cdr::String& docType) const;

        /**
         * Returns <code>true</code> if the user account for the current
         * session is authorized to perform the specified action on the
         * specified document.
         *
         *  @param  connection      reference to active connection object
         *                          for the CDR database, which we query to
         *                          determine whether the user can perform the
         *                          specified action.
         *  @param  action          name of the action to be checked.
         *  @param  docId           integer document ID
         *  @return                 <code>true</code> iff the user is 
         *                          authorized to perform the specified
         *                          action.
         */
        bool canDo(db::Connection&    connection, 
                   const cdr::String& action, 
                   int docId) const;

    private:

        /**
         * Primary key for the row in the <code>session</code> table of the
         * CDR database for this session.
         */
        int id;

        /**
         * Name used to identify the session in communication between the
         * client and server.  From the <code>name</code> column of the
         * <code>session</code> table.  The string is generated by the
         * CdrLogon command, and is sufficiently complex to make it
         * sufficiently difficult to spoof an existing session by guessing its
         * identifying string.
         */
        String name;

        /**
         * Primary key for the row in the <code>usr</code> table of the CDR
         * database for the user account used to create this session.
         */
        int uid;

        /**
         * Account name for the currently logged-on user.  From the
         * <code>name</code> column of the <code>usr</code> table.
         */
        String uName;

        /**
         * Value used for the <code>Status</code> attribute of the
         * <code>CdrResponse</code> element.  Initialized to "success" by the
         * command dispatcher prior to invocation of the command handler for
         * each command.  Can be modified (for example, to "warning") by the
         * command handler if appropriate.  If an exception is caught by the 
         * command dispatcher, the attribute will be set to "error".
         */
        String lastStatus;
    };
}

#endif
