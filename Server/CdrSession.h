/*
 * $Id$
 *
 * Information about the current login.
 *
 * BZIssue::4767
 */

#ifndef CDR_SESSION_
#define CDR_SESSION_

#include "CdrString.h"
#include "CdrDbConnection.h"
#include "CdrBlob.h"

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
            iBlob = Blob();
        }

        /**
         * Accessor method for the primary key of the row in the
         * <code>session</code> table of the CDR database for this session.
         *
         *  @return         integer representing the key for this session.
         */
        int getId() const { return id; }

        /**
         * Accessor method for the name of this session.
         *
         *  @return         reference to string containing the name of the
         *                  session.
         */
        String getName() const { return name; }

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
         * Set a user's password to a new value.  For systems supporing
         * hashed passwords, stores only a hash value.
         *
         * @param conn      database connection.
         * @param userName  unique short name of the user to modify,
         *                   may or may not be the same as the Session user.
         * @param password  new password.
         * @param newUser   true=new user, false=existing user
         */
        void setUserPw(db::Connection&, const String&, const String&,
                       const bool);

        /**
         * Accessor method for the blob sent by the client as part of
         * the command set submitted.  A command set can only contain
         * at most one CdrDocBlob element (enforced by the command set
         * preprocessor).
         *
         *  @return         CDR Blob object
         */
        Blob getClientBlob() const { return iBlob; }

        /**
         * Used by preprocessor of command set buffer to store any blob
         * it finds in what the client sent.
         *
         *  @param  blob  string representing outcome of processing of
         *                  last command.
         */
        void setClientBlob(const Blob& blob) { iBlob = blob; }

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
         *  @param  ip              string representation of the IP address
         *                          from which the most recent request came.
         */
        void setLastActivity(db::Connection& conn, const String& ip) const;

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
         * identifying string.  The exception to this prinicple is the 'guest'
         * session, kept open permanently for the guest account.
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

        /**
         * Possibly null blob object representing blob submitted by the
         * client (if any).  The name i[ncoming]Blob is used in case
         * we also need another object to represent an o[outgoing]Blob
         * later on.
         */
        Blob iBlob;
    };

    /**
     * Create a new session row in the database.
     *
     * This does not create a session object.  It just creates a new row
     * in the database with a new, random session ID.  It can be called
     * either for a new session or to duplicate one (q.v.).
     *
     * Note: No password checking is done.  Password authentication will
     * be done elsewhere, possibly using Active Directory.
     *
     *  @param userId       Unique ID of user for whom session will be created.
     *  @param userName     Double check of usrId, must match db usr.name
     *  @param conn         Open database connection.
     *  @param comment      Optional reason for session.
     *
     *  @return             New string form session ID.
     *
     *  @throws cdr::Exception if name/password failure.
     */
    String createSessionRecord(
            const int      userId,
            const String&  userName,
            db::Connection conn,
            const String&  comment = false);

    /**
     * Duplicate a session record.
     *
     * Used when a user starts a batch job that requires a logged in session
     * in order to run.  The user might queue the batch job and then logout,
     * causing his existing session to be logged out and unusable when the
     * batch job starts.
     *
     *  @param oldSession   Session string name for a logged in session.
     *  @param conn         Open database connection.
     *  @param comment      Optional comment - why duplicating a session.
     *                      Default stores null in the session table.
     *
     *  @return             New session name
     *
     *  @throws cdr::Exception if oldSession does not exist or is inactive.
     */
    String duplicateSessionRecord(
        const String&   oldSession,
        db::Connection& conn,
        const String&   comment = false);

    /**
     * Check the format of a password.  Does it match the rules we establish
     * for a safe password.
     *
     *  @param password     String to check.
     *
     *  @throws cdr::Exception if password fails any test.
     */
    void testPasswordString(const cdr::String& password);

    /**
     * Determine if a passed password matches the password for the
     * passed user name.
     *
     * This function will likely be modified or abandoned if/when we move to
     * authentication via external NIH Active Directory resources.
     *
     *  @param userName     The unique short name of the user's usr record.
     *  @param password     Matching password.
     *  @param conn         Open database connection.
     *
     *  @return             ID of the user, -1 if not found.
     */
    int chkIdPassword(
        String&         userName,
        String&         password,
        db::Connection& conn);

    /**
     * Get the current count of consecutive failed logins for a user.
     *
     *  @param conn         Open database connection.
     *  @param userName     Unique short name of the user's usr record
     *
     *  @return             Integer count, -1 if user unknown
     */
    int getLoginFailedCount(
        db::Connection&    conn,
        const cdr::String& userName);

    /**
     * Clear or increment the current count of consecutive failed logins
     * for a user.
     *
     *  @param conn         Open database connection.
     *  @param userName     Unique short name of the user's usr record.
     *  @param counter      0 = set the count to 0, i.e., clear it.
     *                      1 = add one to the count, i.e., increment it.
     *
     *  @return             New value of login_failed count, or -1 if user
     *                      is not found.
     *
     *  @throws cdr::Exception if anything other than 0 or 1 counter passed.
     */
    int setLoginFailedCount(
        db::Connection&    conn,
        const cdr::String& userName,
        int                counter);
}

#endif
