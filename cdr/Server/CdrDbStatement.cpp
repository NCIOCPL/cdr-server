/*
 * $Id: CdrDbStatement.cpp,v 1.5 2000-04-22 22:15:04 bkline Exp $
 *
 * Implementation for ODBC HSTMT wrapper (modeled after JDBC).
 *
 * $Log: not supported by cvs2svn $
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

#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

/**
 * Allocates a statement handle for the current database connection.
 */
cdr::db::Statement::Statement(Connection& c) : conn(c)
{
    hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    rc = SQLAllocHandle(SQL_HANDLE_STMT, conn.hdbc, &hstmt);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure allocating database statement",
                             getErrorMessage(rc));
}

/**
 * Releases the parameter information for the current query and drops the
 * statement handle.
 */
cdr::db::Statement::~Statement()
{
    clearParms();
    SQLRETURN rc = SQLFreeStmt(hstmt, SQL_DROP);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure dropping database statement",
                             getErrorMessage(rc));
}

/**
 * Releases the parameter information for the current query and resets the
 * statement handle.
 */
void cdr::db::Statement::close()
{
    clearParms();
    SQLRETURN rc = SQLFreeStmt(hstmt, SQL_CLOSE);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure closing database statement",
                             getErrorMessage(rc));
}

/**
 * Releases the parameter information for the current query.
 */
void cdr::db::Statement::clearParms()
{
    for (int i = 0; i < paramVector.size(); ++i) {
        Parameter* p = paramVector[i];
        delete [] p->value;
        delete p;
    }
    paramVector.clear();
}

/**
 * Binds the registered positional parameters for the query and submits it to
 * the database.  Returns a new <code>ResultSet</code> object.
 */
cdr::db::ResultSet cdr::db::Statement::executeQuery(const char* query)
{
    SQLRETURN rc;
    for (int i = 0; i < paramVector.size(); ++i) {
        Parameter* p = paramVector[i];
        rc = SQLBindParameter(hstmt, p->position, SQL_PARAM_INPUT, 
                                     p->cType, p->sType, 
                                     p->len, 0, p->value, 0, &p->cb);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Failure binding parameter",
                                 getErrorMessage(rc));
    }
    rc = SQLExecDirect(hstmt, (SQLCHAR *)query, SQL_NTS);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
            rc != SQL_NO_DATA_FOUND)
        throw cdr::Exception(L"Failure executing database query",
                             getErrorMessage(rc));
    return cdr::db::ResultSet(*this);
}

/**
 * Saves a copy of the string value <code>val</code> for the <code>pos</code>
 * parameter of the current query.
 */
void cdr::db::Statement::setString(int pos, const cdr::String& val)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_WCHAR;
    p->sType     = val.size() > 2000 ? SQL_WLONGVARCHAR : SQL_WVARCHAR;
    if (val.isNull()) {
        p->len   = 1; // Required by ODBC even for NULL data!
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->len   = val.size() * sizeof(wchar_t) + sizeof(wchar_t);
        p->value = new wchar_t[val.size() + 1];
        p->cb    = SQL_NTS;
        memcpy(p->value, val.c_str(), p->len);
    }
    paramVector.push_back(p);
}

/**
 * Saves a copy of the string value <code>val</code> for the <code>pos</code>
 * parameter of the current query.  This version handles narrow-character
 * strings.  The third parameter is optional and defaults to false.
 * 
 *  @deprecated
 */
void cdr::db::Statement::setString(int pos, const std::string& val, bool null)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_CHAR;
    p->sType     = SQL_VARCHAR;
    if (null) {
        p->len   = 1; // Required by ODBC even for NULL data!
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->len   = val.size() + 1;
        p->value = new char[val.size() + 1];
        p->cb    = SQL_NTS;
        memcpy(p->value, val.c_str(), p->len);
    }
    paramVector.push_back(p);
}

/**
 * Saves a copy of the integer value <code>val</code> for the <code>pos</code>
 * parameter of the current query.
 */
void cdr::db::Statement::setInt(int pos, const cdr::Int& val)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_SLONG;
    p->sType     = SQL_INTEGER;
    p->len       = 0;
    if (val.isNull()) {
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->value = new int[1];
        p->cb    = 0;
        int i    = val;
        memcpy(p->value, &i, sizeof(int));
    }
    paramVector.push_back(p);
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
