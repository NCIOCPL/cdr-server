/*
 * $Id: CdrDbStatement.cpp,v 1.2 2000-04-17 21:24:48 bkline Exp $
 *
 * Implementation for ODBC HSTMT wrapper (modeled after JDBC).
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/15 12:21:14  bkline
 * Initial revision
 *
 */

#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"

cdr::db::Statement::Statement(Connection& c) : conn(c)
{
    hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc;
    rc = SQLAllocHandle(SQL_HANDLE_STMT, conn.getDbc(), &hstmt);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure allocating database statement",
                             getErrorMessage(rc));
}

cdr::db::Statement::~Statement()
{
    for (int i = 0; i < paramVector.size(); ++i) {
        Parameter* p = paramVector[i];
        delete [] p->value;
        delete p;
    }
    SQLFreeStmt(hstmt, SQL_DROP);
}

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
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Failure executing database query",
                             getErrorMessage(rc));
    return cdr::db::ResultSet(*this);
}

void cdr::db::Statement::setString(int pos, const cdr::String& val)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_WCHAR;
    p->sType     = val.size() > 2000 ? SQL_WLONGVARCHAR : SQL_WVARCHAR;
    if (val.isNull()) {
        p->len   = 0;
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->len   = val.size() * sizeof(wchar_t) + sizeof(wchar_t);
        p->value = new wchar_t[val.size() + 1];
        p->cb    = SQL_NTS;
        memset(p->value, 0, p->len + sizeof(wchar_t));
        memcpy(p->value, val.c_str(), p->len);
    }
    paramVector.push_back(p);
}

void cdr::db::Statement::setString(int pos, const std::string& val, bool null)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_CHAR;
    p->sType     = SQL_VARCHAR;
    if (null) {
        p->len   = 0;
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->len   = val.size() + 1;
        p->value = new char[val.size() + 1];
        p->cb    = SQL_NTS;
        memset(p->value, 0, p->len + sizeof(char));
        memcpy(p->value, val.c_str(), p->len);
    }
    paramVector.push_back(p);
}

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

cdr::String cdr::db::Statement::getErrorMessage(SQLRETURN rc)
{
    return cdr::db::Connection::getErrorMessage(rc, getEnv(), getDbc(),
                                                hstmt);
}
