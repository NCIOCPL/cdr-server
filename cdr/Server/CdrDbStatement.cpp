/*
 * $Id: CdrDbStatement.cpp,v 1.6 2000-05-03 15:25:41 bkline Exp $
 *
 * Implementation for ODBC HSTMT wrapper (modeled after JDBC).
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2000/04/22 22:15:04  bkline
 * Added more comments.
 *
 * Revision 1.4  2000/04/22 09:28:45  bkline
 * Added close() method.  Added error checking for destructor.  Added
 * some tests for SQL_NO_DATA_FOUND.  Set p->len to 1 for NULL values
 * (needed by ODBC for some reason).
 *
 * Revision 1.3  2000/04/21 13:55:03  bkline
 * Fixed bug in setString buffer initializations.
 *
 * Revision 1.2  2000/04/17 21:24:48  bkline
 * Added nullability for ints and strings.  Fixed length byte for setString().
 *
 * Revision 1.1  2000/04/15 12:21:14  bkline
 * Initial revision
 */

#include <iostream>
#include "CdrDbStatement.h"
#include "CdrDbConnection.h"
#include "CdrDbResultSet.h"

/**
 * Allocates a statement handle for the current database connection.
 */
cdr::db::Statement::Statement(Connection& c) 
    : conn(c), refCount(1), pRefCount(&refCount)
{
    hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    rc = SQLAllocHandle(SQL_HANDLE_STMT, conn.hdbc, &hstmt);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure allocating database statement",
                             getErrorMessage(rc));
}

/**
 * Copy constructor.
 */
cdr::db::Statement::Statement(const Statement& s) 
    : conn(s.conn), hstmt(s.hstmt), pRefCount(s.pRefCount)
{
    //std::cerr << "COPY CONSTRUCTOR FOR Statement; "
    //            << *pRefCount BEFORE INCREMENT=" << *pRefCount << '\n';
    ++*pRefCount;
}

/**
 * Releases the parameter information for the current query and drops the
 * statement handle.
 */
cdr::db::Statement::~Statement()
{
    //std::cerr << "Statement destructor; *pRefCount BEFORE DECREMENT=" 
    //    << *pRefCount << '\n';
    if (--*pRefCount > 0)
        return;
    SQLRETURN rc = SQLFreeStmt(hstmt, SQL_DROP);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure dropping database statement",
                             getErrorMessage(rc));
}

/**
 * Resets the statement handle, allowing it to be reused.
 */
void cdr::db::Statement::close()
{
    //std::cerr << "Statement::close()\n";
    closeStatement();
}

/**
 * Common processing for close() in base and derived classes.
 */
void cdr::db::Statement::closeStatement()
{
    //std::cerr << "Statement::closeStatement()\n";
    SQLRETURN rc = SQLFreeStmt(hstmt, SQL_CLOSE);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure closing database statement",
                             getErrorMessage(rc));
}

/**
 * submits the specified query to the CDR database and returns a new
 * <code>ResultSet</code> object.
 */
cdr::db::ResultSet cdr::db::Statement::executeQuery(const char* query)
{
    SQLRETURN rc;
    rc = SQLExecDirect(hstmt, (SQLCHAR *)query, SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure executing database query",
                             getErrorMessage(rc));
    return cdr::db::ResultSet(*this);
}

/**
 * Wrapper method which provides a more convenient way to invoke the
 * <code>Connection</code> class's method of the same name.
 */
cdr::String cdr::db::Statement::getErrorMessage(SQLRETURN rc)
{
    return cdr::db::Connection::getErrorMessage(rc, conn.henv, conn.hdbc,
                                                hstmt);
}
