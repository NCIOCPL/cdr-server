/*
 * $Id: CdrDbConnection.cpp,v 1.3 2000-04-22 22:15:04 bkline Exp $
 *
 * Implementation for ODBC connection wrapper.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/22 09:33:17  bkline
 * Added transaction support and getLastIdent() method.
 *
 * Revision 1.1  2000/04/15 12:22:32  bkline
 * Initial revision
 */

#include "CdrDbConnection.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"

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
cdr::db::Connection::Connection() : autoCommit(false), hdbc(SQL_NULL_HDBC)
{
    SQLRETURN rc;
    if (henv == SQL_NULL_HENV) {
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
    rc = SQLConnect(hdbc, (SQLCHAR *)"cdr", SQL_NTS,
                          (SQLCHAR *)"cdr", SQL_NTS,
                          (SQLCHAR *)"***REMOVED***", SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure connecting to database",
                             getErrorMessage(rc, henv, hdbc, 0));
    setAutoCommit(true);
}

/**
 * Cleans up the connection.
 */
cdr::db::Connection::~Connection()
{
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
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
    if (autoCommit == val)
        return;
    SQLUINTEGER setting = val ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
    SQLRETURN rc = SQLSetConnectAttr(hdbc, 
                                     SQL_ATTR_AUTOCOMMIT, 
                                     reinterpret_cast<SQLPOINTER>(setting), 
                                     0);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure setting auto commit",
                             getErrorMessage(rc, henv, hdbc, 0));
    autoCommit = val;
}

/**
 * Commits the pending database actions for the current transaction.
 */
void cdr::db::Connection::commit()
{
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
    cdr::db::Statement s(*this);
    cdr::db::ResultSet r = s.executeQuery("SELECT @@IDENTITY");
    if (!r.next())
        throw cdr::Exception(L"Failure getting last ident value");
    Int i = r.getInt(1);
    if (i.isNull())
        throw cdr::Exception(L"Failure getting last ident value");
    return i;
}
