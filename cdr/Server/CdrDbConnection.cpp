/*
 * $Id: CdrDbConnection.cpp,v 1.8 2002-02-28 01:02:53 bkline Exp $
 *
 * Implementation for ODBC connection wrapper.
 *
 * $Log: not supported by cvs2svn $
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
#include "CdrLock.h"

// XXX For debugging memory leaks.
#ifdef HEAPDEBUG
static int activeConnections;
static const char* const ActiveConnectionsMutex = "ActiveConnectionsMutex";
#endif

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
                                const SQLCHAR* pwd) 
    : autoCommit(false), hdbc(SQL_NULL_HDBC), refCount(1)
{
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
    rc = SQLConnect(hdbc, const_cast<SQLCHAR*>(dsn), SQL_NTS,
                          const_cast<SQLCHAR*>(uid), SQL_NTS, 
                          const_cast<SQLCHAR*>(pwd), SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        hdbc = SQL_NULL_HDBC;
        throw cdr::Exception(L"Failure connecting to database",
                             getErrorMessage(rc, henv, hdbc, 0));
    }
    setAutoCommit(true);
#ifdef HEAPDEBUG
    HANDLE mutex = CreateMutex(0, false, ActiveConnectionsMutex);
    if (mutex != 0) {
        cdr::Lock lock(mutex, 1000);
        if (lock.m) {
            ++activeConnections;
        }
		CloseHandle(mutex);
    }
#endif
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
#ifdef HEAPDEBUG
    HANDLE mutex = CreateMutex(0, false, ActiveConnectionsMutex);
    if (mutex != 0) {
        cdr::Lock lock(mutex, 1000);
        if (lock.m) {
            if (activeConnections < 1)
                throw cdr::Exception(
                    L"Connection::close(): No connections open.");
            --activeConnections;
        }
		CloseHandle(mutex);
    }
#endif
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
    return rs.getString(1);
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
 *  @return         new <code>Connection</code> object.
 */
cdr::db::Connection 
cdr::db::DriverManager::getConnection(const cdr::String& url_, 
                                      const cdr::String& uid_,
                                      const cdr::String& pwd_)
{
    if (url_.length() < 6 || url_.substr(0, 5) != L"odbc:")
        throw cdr::Exception(
                L"DriverManager::getConnection(): Unsupported connection type",
                url_);

    std::string dsn = url_.toUtf8().substr(5);
    std::string uid = uid_.toUtf8();
    std::string pwd = pwd_.toUtf8();
    return Connection(reinterpret_cast<const SQLCHAR*>(dsn.c_str()),
                      reinterpret_cast<const SQLCHAR*>(uid.c_str()),
                      reinterpret_cast<const SQLCHAR*>(pwd.c_str()));
}
