/*
 * $Id: CdrDbConnection.cpp,v 1.1 2000-04-15 12:22:32 bkline Exp $
 *
 * Implementation for ODBC connection wrapper.
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrDbConnection.h"
#include "CdrException.h"

HENV cdr::db::Connection::henv = SQL_NULL_HENV;

cdr::db::Connection::Connection()
{
    SQLRETURN rc;
    hdbc = SQL_NULL_HDBC;
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
}

cdr::db::Connection::~Connection()
{
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
}

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
