/*
 * $Id$
 *
 * Implementation for ODBC connection wrapper.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.9  2002/03/04 20:52:03  bkline
 * Workaround for bug in Microsoft runtime.
 *
 * Revision 1.8  2002/02/28 01:02:53  bkline
 * Added code to close mutex handle.
 *
 * Revision 1.7  2001/12/14 15:19:10  bkline
 * Added optional hooks for tracking number of active connections during
 * heap debugging.
 *
 * Revision 1.6  2000/06/01 18:48:58  bkline
 * Removed some debugging output.
 *
 * Revision 1.5  2000/05/21 00:48:59  bkline
 * Replaced public constructor with ServerDriver::getConnection().
 *
 * Revision 1.4  2000/05/03 15:25:41  bkline
 * Fixed database statement creation.
 *
 * Revision 1.3  2000/04/22 22:15:04  bkline
 * Added more comments.
 *
 * Revision 1.2  2000/04/22 09:33:17  bkline
 * Added transaction support and getLastIdent() method.
 *
 * Revision 1.1  2000/04/15 12:22:32  bkline
 * Initial revision
 */

#include <iostream>

#include "CdrDbConnection.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"
#include "CdrCtl.h"
#include "CdrLock.h"

// Included for use by getpw
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <string.h>
#include <sys/stat.h>

/**
 * The following are defined for debugging purposes.
 *
 * Originally these were defined via conditional compilation, but they
 * are now part of the regular CdrServer.  However they are only used
 * when ctl table entries direct their use.
 *
 * There is no performance penalty for defining them and no significant
 * memory cost.
 */

// Total connections since CdrServer start
static int totalConnections;

// Total currently active
static int activeConnections;

// Highest number active at any one time
static int highConnections;

// Total active for each caller
static int activeCalls[cdr::db::connThatsAllFolks];

// Highest number active for each caller
static int highCalls[cdr::db::connThatsAllFolks];

// Logfile for connection statistics
static std::string connLogFile =
        cdr::log::getDefaultLogDir() + "/" + "ServerConns.log";

// Set true for verbose logging
static bool verboseLogging = true;

// For thread safety
static const char* const ActiveConnectionsMutex = "ActiveConnectionsMutex";

// Forward declaration
static void logConnections(char *src);


/**
 * ODBC environment handle shared by all CDR connections.
 */
HENV cdr::db::Connection::henv = SQL_NULL_HENV;

/**
 * Allocates a shared environment handle if this hasn't already been taken
 * care of.  Then allocates a connection handle and connects to the CDR
 * database.  Makes sure auto-commit mode is turned on.
 *
 *  @exception cdr::Exception if ODBC error encountered.
 */
cdr::db::Connection::Connection(const SQLCHAR* dsn,
                                const SQLCHAR* uid,
                                const SQLCHAR* pwd,
                                const cdr::db::ConnectCaller callerId)
    : autoCommit(false), hdbc(SQL_NULL_HDBC), refCount(1), whoCalled(callerId)
{
    // Object's variable = True = track database connection pool usage
    checkConnections = false;

    // Default timeout for connections
    int loginTimeout = 60;
    try {
        if (getCtlValue(L"dbConn", "CheckConnections") != L"")
            checkConnections = true;
        cdr::String timeoutStr = getCtlValue(L"dbConn", "ConnectionTimeout");
        if (timeoutStr != L"")
            loginTimeout = timeoutStr.getInt();
    }
    catch (const cdr::Exception& e) {
        // CdrServer makes a call to get a connection to use in loading
        //  the ctl table into memory.  In that first ever call to
        //  getConnection(), the table is not loaded and the call to
        //  getCtlValue() will fail with an exception.
        // That's okay.  Leave the value we were searching for in its
        //  initial state - which should be legal (because we'll make it so.)
        // Subsequent calls should work.
        //
        // Reference e to silence warning
        e.what();
    }

    master = this;
    SQLRETURN rc;
    if (henv == SQL_NULL_HENV) {
        rc = SQLSetEnvAttr(0, SQL_ATTR_CONNECTION_POOLING,
                           (SQLPOINTER)SQL_CP_ONE_PER_HENV, 0);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Failure enabling connection pooling",
                                 getErrorMessage(rc, henv, 0, 0));
        rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Failure allocating database environment",
                                 getErrorMessage(rc, henv, 0, 0));
        rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                           (void *)SQL_OV_ODBC3, 0);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Failure setting database environment",
                                 getErrorMessage(rc, henv, 0, 0));
    }
    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure allocating database connection",
                             getErrorMessage(rc, henv, hdbc, 0));

    // Set a login timeout, no error until this many seconds have passed
    rc = SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT,
                           (SQLPOINTER) &loginTimeout, 0);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure setting database connection timeout",
                             getErrorMessage(rc, henv, hdbc, 0));

    rc = SQLConnect(hdbc, const_cast<SQLCHAR*>(dsn), SQL_NTS,
                          const_cast<SQLCHAR*>(uid), SQL_NTS,
                          const_cast<SQLCHAR*>(pwd), SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        // Don't do this here, it deletes access to error messages
        // hdbc = SQL_NULL_HDBC;

        // Add a delay.  If connection failed don't try immediately again
        cdr::log::WriteFile("Connection constructor",
                            "Waiting 20 seconds after a connection failure");
        Sleep(20000);
        throw cdr::Exception(L"Failure connecting to database",
                             getErrorMessage(rc, henv, hdbc, 0));
    }
    setAutoCommit(true);

    // If we're debug/checking connection processing
    if (checkConnections) {
        HANDLE mutex = CreateMutex(0, false, ActiveConnectionsMutex);
        if (mutex != 0) {
            cdr::Lock lock(mutex, 1000);
            if (lock.m) {
                // Increment connection counters
                ++activeConnections;
                ++totalConnections;
                ++activeCalls[callerId];

                // Have we hit a new high?
                bool newHigh = false;
                if (activeConnections > highConnections) {
                    highConnections = activeConnections;
                    newHigh         = true;
                }
                if (activeCalls[callerId] > highCalls[callerId]) {
                    highCalls[callerId] = activeCalls[callerId];
                    newHigh         = true;
                }

                // Log if new high or if we're configured to always log
                if (newHigh || verboseLogging)
                    logConnections("New");
            }
            CloseHandle(mutex);
        }
    }
}

/**
 * Makes a safe copy of the <code>Connection</code> object
 * which knows when the master copy has been closed.
 *
 *  @param  c           reference to <code>Connection</code>
 *                      object.
 */
cdr::db::Connection::Connection(const Connection& c) : hdbc(c.hdbc),
      master(const_cast<Connection*>(&c))
{
    whoCalled = c.whoCalled;
    ++master->refCount;
    //std::cerr << "Connection copy constructor; refCount=" << master->refCount
    //          << "\n";;
}

/**
 * Makes a safe copy of the <code>Connection</code> object
 * which knows when the master copy has been closed.
 *
 *  @param  c           reference to <code>Connection</code>
 *                      object.
 *  @return             reference to modified
 *                      <code>Connection</code> object.
 */
cdr::db::Connection& cdr::db::Connection::operator=(const Connection& c)
{
    hdbc        = c.hdbc;
    whoCalled   = c.whoCalled;
    master      = const_cast<Connection*>(&c);
    ++master->refCount;
    return *this;
}

/**
 * Cleans up the connection if appropriate.
 */
cdr::db::Connection::~Connection()
{
    //std::cerr << "Connection destructor; refCount=" << master->refCount <<
    //    "\n";
    if (--master->refCount < 1 && !isClosed())
        close();
}

/**
 * Cleans up the connection.
 */
void cdr::db::Connection::close()
{
    if (isClosed())
        throw cdr::Exception(
                L"Connection::close(): Connection already closed.");

    if (checkConnections) {
        HANDLE mutex = CreateMutex(0, false, ActiveConnectionsMutex);
        if (mutex != 0) {
            cdr::Lock lock(mutex, 1000);
            if (lock.m) {
                if (activeConnections < 1)
                    throw cdr::Exception(
                        L"Connection::close(): No connections open.");

                // Decrement and log
                --activeConnections;
                --activeCalls[whoCalled];
                if (verboseLogging)
                    logConnections("Del");
            }
            CloseHandle(mutex);
        }
    }

    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    master->hdbc = SQL_NULL_HDBC;
}

/**
 * Loops through all the ODBC and server error messages and concatenates them
 * into a single string suitable for logging and sending back as Error
 * elements.
 */
cdr::String cdr::db::Connection::getErrorMessage(SQLRETURN sqlReturn,
                                                 HENV env,
                                                 HDBC dbc,
                                                 HSTMT stmt)
{
    cdr::String     errString = L"SQL ERROR CODE: ";
    static UCHAR    state[20];
    static UCHAR    errorMessage[SQL_MAX_MESSAGE_LENGTH];
    static char     workBuf[SQL_MAX_MESSAGE_LENGTH + 1024];
    SDWORD          nativeError;
    SWORD           messageLength;
    SQLRETURN       rc;

    switch (sqlReturn) {
        case SQL_SUCCESS:
            errString += L"NO ERROR";
            break;
        case SQL_SUCCESS_WITH_INFO:
            errString += L"SQL_SUCCESS_WITH_INFO";
            break;
        case SQL_NO_DATA_FOUND:
            errString += L"SQL_NO_DATA_FOUND";
            break;
        case SQL_ERROR:
            errString += L"SQL_ERROR";
            break;
        case SQL_INVALID_HANDLE:
            errString += L"SQL_INVALID_HANDLE";
            break;
        case SQL_STILL_EXECUTING:
            errString += L"SQL_STILL_EXECUTING";
            break;
        case SQL_NEED_DATA:
            errString += L"SQL_NEED_DATA";
            break;
        default:
            errString += L"UNKNOWN RESULT CODE";
            break;
    }
    while ((rc = SQLError(env, dbc, stmt, state, &nativeError,
        errorMessage, sizeof errorMessage - 1, &messageLength))
        == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
    {

        sprintf(workBuf, "; SQL STATE: %s"
                         "; SQL NATIVE ERROR: %ld"
                         "; SQL ERROR MESSAGE: %s",
                         state,
                         nativeError,
                         errorMessage);
        errString += cdr::String(workBuf);
    }
    return errString;
}

/**
 * Controls whether multi-query transactions will be used.
 */
void cdr::db::Connection::setAutoCommit(bool val)
{
    if (isClosed())
        throw cdr::Exception(L"Connection::setAutoCommit(): Connection closed");
    if (getAutoCommit() == val)
        return;
    SQLUINTEGER setting = val ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
    SQLRETURN rc = SQLSetConnectAttr(hdbc,
                                     SQL_ATTR_AUTOCOMMIT,
                                     reinterpret_cast<SQLPOINTER>(setting),
                                     0);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure setting auto commit",
                             getErrorMessage(rc, henv, hdbc, 0));
    master->autoCommit = val;
}

/**
 * Commits the pending database actions for the current transaction.
 */
void cdr::db::Connection::commit()
{
    if (isClosed())
        throw cdr::Exception(L"Connection::commit(): Connection closed");
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Commit failure",
                             getErrorMessage(rc, henv, hdbc, 0));
}

/**
 * Discards the pending database actions for the current transaction.
 */
void cdr::db::Connection::rollback()
{
    if (isClosed())
        throw cdr::Exception(L"Connection::rollback(): Connection closed");
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_ROLLBACK);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Rollback failure",
                             getErrorMessage(rc, henv, hdbc, 0));
}

/**
 * Obtains the most recent IDENTITY value generated by SQL Server for this
 * connection.
 */
int cdr::db::Connection::getLastIdent()
{
    if (isClosed())
        throw cdr::Exception(L"Connection::getLastIdent(): Connection closed");
    cdr::db::Statement s(*this);
    cdr::db::ResultSet r = s.executeQuery("SELECT @@IDENTITY");
    if (!r.next())
        throw cdr::Exception(L"Failure getting last ident value");
    Int i = r.getInt(1);
    if (i.isNull())
        throw cdr::Exception(L"Failure getting last ident value");
    return i;
}

/**
 * Returns a string representing the CDR DBMS's idea of the
 * current time.
 */
cdr::String cdr::db::Connection::getDateTimeString()
{
    if (isClosed())
        throw cdr::Exception(
                L"Connection::getDateTimeString(): Connection closed");
    cdr::db::Statement query(*this);
    cdr::db::ResultSet rs = query.executeQuery("SELECT GETDATE()");
    if (!rs.next())
        throw cdr::Exception(L"Failure getting date from DBMS");
    // Unless we explicitly put this on the stack the destructor is never called
    // (bug in Microsoft's runtime).
    cdr::String result = rs.getString(1);
    return result;
}

cdr::db::Statement cdr::db::Connection::createStatement()
{
    return cdr::db::Statement(*this);
}

cdr::db::PreparedStatement
cdr::db::Connection::prepareStatement(const std::string& s)
{
    if (isClosed())
        throw cdr::Exception(
                L"Connection::prepareStatement(): Connection closed");
    return PreparedStatement(*this, s);
}

/**
 * Opens a new connection to the CDR database.  Connection pooling
 * is used to reduce the number of actual connections which have
 * to be built up and torn down.
 *
 *  @param  url     string of the form "odbc:dsn" where dsn is
 *                  ODBC DSN for the connection.
 *  @param  uid     login ID for the database.
 *  @param  pwd     password for the database account.
 *  @param callerId identifies caller for resource tracking
 *  @return         new <code>Connection</code> object.
 */
cdr::db::Connection
cdr::db::DriverManager::getConnection(const cdr::String& url_,
                                      const cdr::String& uid_,
                                      const cdr::String& pwd_,
                                      const cdr::db::ConnectCaller callerId)
{
    if (url_.length() < 6 || url_.substr(0, 5) != L"odbc:")
        throw cdr::Exception(
                L"DriverManager::getConnection(): Unsupported connection type",
                url_);

    // Remove "odbc:" prefix and convert all parms to std strings
    std::string dsn = url_.toUtf8().substr(5);
    std::string uid = uid_.toUtf8();
    std::string pwd = pwd_.toUtf8();

    if (pwd.length() == 0)
        throw cdr::Exception(L"getConnection has empty pwd");
    if (uid.length() == 0)
        throw cdr::Exception(L"getConnection has empty uid");

/**
 * For DEBUG only.
 *
 * For debugging connection failures, we can record all connection attempts
 * to a log file.  The file will contain a unique sequential ID for the
 * connection attempt, a datetime stamp, and the dsn, uid, and pw.
 *
 * static long        s_DebugSequentialId  = 0L;
 * static std::string s_DebugConnectionLog = "d:/cdr/Log/ServerConn.log";
 *
 * char buf[1000];
 * sprintf (buf, "id=%ld  dsn=[%s]  uid=[%s]  pwd=[%s]", ++s_DebugSequentialId,
 *                dsn.c_str(), uid.c_str(), pwd.c_str());
 * cdr::log::WriteFile(L"conn", buf, s_DebugConnectionLog);
 */

    return Connection(reinterpret_cast<const SQLCHAR*>(dsn.c_str()),
                      reinterpret_cast<const SQLCHAR*>(uid.c_str()),
                      reinterpret_cast<const SQLCHAR*>(pwd.c_str()),
                      callerId);
}


/**
 * Examine a line from the password file to see if it starts with
 * the specified key (ignoring case differences).  Also ignore
 * leading and trailing whitespace.
 *
 *  @param line         Search this line for a pw for our situation.
 *  @param key          Prefix prepended to pw in the password file.
 *                      Example: "CBIIT:PROD:CDR:CDRSQLACCOUNT:"
 *
 *  @return             Password string or "" if not found.
 */
static std::string findpw(const std::string& line, const std::string& key) {

    // Get pointers to the beginning and one past the end of the line buffer.
    const char* p = line.c_str();
    const char* q = p + line.size();

    // Walk past leading whitespace.
    while (p < q) {
        if (!isspace(*p))
            break;
        ++p;
    }

    // Back up end pointer to beginning of any trailing whitespace.
    while (p < q) {
        if (!isspace(*(q - 1)))
            break;
        --q;
    }

    // See if the rest of the buffer starts with the key.
    const char* k = key.c_str();
    while (*k && p < q) {
        if (std::toupper(*k) != std::toupper(*p))
            return "";
        ++k;
        ++p;
    }

    // Return the rest of the string (sans trailing whitespzce).
    std::string pw(p, q - p);
    return pw;
}

/**
 * Determine the tier (DEV, QA, PROD, etc.) of the current server by
 * reading a configuration file.
 *
 * NOTE:
 *   Leading or trailing spaces will cause the function to fail.
 *   This will show up in an exception msg like:
 *      "getCdrDbPw cannot find password for CBIIT:DEV :CDR:CDRSQLACCOUNT"
 *
 *  @return name of the tier.
 *
 *  @throw cdr::Exception if TIERFILE is not opened.
 */
static std::string getTier() {

    // Set the name of the tier file for the Windows drive we're using
    char tierFile[] = "x:/etc/cdrtier.rc";
    cdr::db::replaceDriveLetter(tierFile);

    std::ifstream s(tierFile);
    if (s.good()) {
        std::string line;
        std::getline(s, line);
        return line;
    }

    // Couldn't open file
    std::string emsg = "getTier cannot open tierfile: \"";
    emsg += strerror(errno);
    throw cdr::Exception(cdr::String(emsg));
}

/**
 * Return the Windows drive letter on which the CDR is running.
 *
 * Exception if not found.
 */
char cdr::db::findCdrDrive() {

    // Cache the result here
    static char s_CdrDrive = 0;

    // Only need to test once.  If cache is initialized, we're good to go
    if (s_CdrDrive)
        return s_CdrDrive;

    // Assume the server is always running in {drive}:/cdr/Bin
    // Note: This can fail in exotic cases, e.g.
    //       CDR installed in c:\cdr\cdr
    //       subst f: c:\cdr    {We want to run the cdr on f:}
    //       Copy of CdrServer.exe put in c:\cdr\bin as well as f:\cdr\bin
    //       We find c before f.  Bummer!
    //       But, hey, that won't happen, will it?
    char fname[] = "x:/cdr/Bin/CdrServer.exe";
    struct _stat statBuf;

    char letters[] = {'d', 'c', 'e', 'f'};

    // Try to find fname with x replaced by each of the letters
    for (int i=0; i<sizeof(letters)/sizeof(char); i++) {
        *fname = letters[i];
        if (_stat(fname, &statBuf) == 0) {
            // Found.  Cache it and return
            s_CdrDrive = letters[i];
            return s_CdrDrive;
        }
    }

    // If we got here, none of the drive letters we tried have the CDR
    throw cdr::Exception(L"No CDR default drive found for CdrServer");
}

/**
 * Replace the first character of a char buf with the cdr drive letter.
 *
 * See CdrDbConnection.h
 */
void cdr::db::replaceDriveLetter(char *fname) {

    char drvLetter = findCdrDrive();
    fname[0] = drvLetter;

    return;
}
/**
 * Look in the CDR password file for the SQL Server password used
 * to log into the CDR database on this tier.  If the password
 * file isn't there, assume we're on a server hosted by OCE, and
 * return the password used on all tiers for that environment.
 * Remember the password we found in the file, so we can skip
 * the step of looking in the file if we're satisfied that the
 * file hasn't changed since the last time we looked.  If we
 * don't find the password, return the string used for the CBIIT
 * DEV server.
 *
 * Note: This only handles the ":CDR:CDRSQLACCOUNT:" login account.
 */
const cdr::String cdr::db::getCdrDbPw() {

    // Remember the file's modification time.
    static struct _stat last_check;

    // Identify database password file for the drive we're running on
    char dbpwFile[] = "x:/etc/cdrdbpw";
    cdr::db::replaceDriveLetter(dbpwFile);

    // DEBUG
    // Uncomment the following lines to build a server that
    //  always fails to connect.
    // cdr::log::WriteFile("getCdrDbPw", "Failing on purpose for testing");
    // return cdr::String("fubarTesting");

    // Cache the password so we can optimize future calls.
    static std::string pw;

    // Find the time the password file was last modified.
    struct _stat this_check;
    int result = _stat(dbpwFile, &this_check);

    // If the file isn't there at all, or we can't read it, do not proceed
    if (result) {
        // Zero out the static struct in case of truly bizarre _stat behavior,
        //  i.e., _stat() modified the struct but returned failure.
        memset(&last_check, 0, sizeof(last_check));

        // Record the error
        std::string emsg = "getCdrDbPw() cannot stat resource: ";
        emsg += strerror(errno);
        throw cdr::Exception(cdr::String(emsg));
    }

    // If the password file hasn't changed, use the cached value.
    // The first time through here the compiler will have initialized the
    //  static struct last_check to all zeros.  The equality comparison
    //  will produce false because this_check.st_mtime cannot be zero.
    if (this_check.st_mtime == last_check.st_mtime) {
        if (pw.empty()) {
            // Guard against case where the line for the password is missing
            //  in the password resource file.
            throw cdr::Exception(L"Password not found in password file");
        }
        return pw;
    }

    last_check = this_check;

    // Should appear once at startup and again each time PW file is edited
    cdr::log::WriteFile(L"getCdrDbPw", L"Reading password resource file");

    // Construct the key for the password we're looking for.
    std::string key = "CBIIT:" + getTier() + ":CDR:CDRSQLACCOUNT:";

    // Walk through the lines in the file looking for the key.
    std::string line;
    std::ifstream s(dbpwFile);
    while (s.good()) {
        std::getline(s, line);
        pw = findpw(line, key);
        if (!pw.empty())
            return cdr::String(pw);
    }
    // If the pw is not found, nothing useful can be done.
    wchar_t tmpbuf[1000];
    swprintf(tmpbuf, L"getCdrDbPw cannot find password for [%s]", key);
    throw cdr::Exception(cdr::String(tmpbuf));
}

/**
 * Format and log information about the current state of the connection
 * counters used for debugging connection activity.
 *
 * This code is only called when this.checkConnections == true.
 *
 * All data comes from static variables conditionally defined when needed.
 *
 *  @param src  Acquiring or releasing a connection
 */
static void logConnections(char *src) {

    std::wostringstream os;

    // Total connections, total active, highest ever
    os << "  " << src << " Total: " << totalConnections
       << "  Active: " << activeConnections
       << "/" << highConnections;

    // active and high for each callerId
    os << "  By caller:";
    for (int i=0; i<cdr::db::connThatsAllFolks; i++)
        os << " " << activeCalls[i] << "/" << highCalls[i];

    // Dump it to the logfile
    cdr::log::WriteFile("", os.str(), connLogFile);
}
