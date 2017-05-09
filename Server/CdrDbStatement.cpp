/*
 * Implementation for ODBC HSTMT wrapper (modeled after JDBC).
 */

#include <iostream>
#include "CdrDbStatement.h"
#include "CdrDbConnection.h"
#include "CdrDbResultSet.h"

/**
 * Allocates a statement handle for the current database connection.
 */
cdr::db::Statement::Statement(Connection& c)
    : conn(c), pRefCount(new int(1))
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
    delete pRefCount;
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
 * Submits the specified query to the CDR database and returns a new
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
 * Submits a SQL request to the CDR database and returns the number of rows
 * affected for an UPDATE, INSERT, or DELETE statement.  Other SQL statements
 * (including DDL) can be submitted using this method, but if the query is not
 * an UPDATE, INSERT, or DELETE statement the return value is undefined.
 */
int cdr::db::Statement::executeUpdate(const char* query)
{
    SQLRETURN rc;
    rc = SQLExecDirect(hstmt, (SQLCHAR *)query, SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure executing database query",
                             getErrorMessage(rc));
    SQLINTEGER count;
    SQLRowCount(hstmt, &count);
    return count;
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

/**
 * Moves to the statement object's next result set, if available.
 */
bool cdr::db::Statement::getMoreResults()
{
    SQLRETURN rc;
    rc = SQLMoreResults(hstmt);
    return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

/**
 * Returns the current result as a ResultSet object.
 */
cdr::db::ResultSet cdr::db::Statement::getResultSet()
{
    return cdr::db::ResultSet(*this);
}
