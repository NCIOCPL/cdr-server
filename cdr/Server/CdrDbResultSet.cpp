/*
 * $Id: CdrDbResultSet.cpp,v 1.1 2000-04-15 12:21:38 bkline Exp $
 *
 * Implementation for ODBC result fetching wrapper (modeled after JDBC).
 *
 * $Log: not supported by cvs2svn $
 */

#include "CdrDbResultSet.h"

cdr::db::ResultSet::ResultSet(cdr::db::Statement& s) : st(s)
{
    SQLSMALLINT nCols;
    SQLRETURN   rc = SQLNumResultCols(st.getStmt(), &nCols);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Database failure creating result set",
                             st.getErrorMessage(rc));
    for (SQLSMALLINT i = 1; i <= nCols; ++i) {
        Column* c= new Column;
        char name[1024];
        SQLSMALLINT cbName;
        rc = SQLDescribeCol(st.getStmt(), (SQLSMALLINT)i, (SQLCHAR *)name, 
                            sizeof name, &cbName, &c->type, &c->size, 
                            &c->digits, &c->nullable);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Database failure fetching column info",
                                 st.getErrorMessage(rc));
        c->name = std::string(name, cbName);
        columnVector.push_back(c);
    }
}

cdr::db::ResultSet::~ResultSet()
{
    for (size_t i = 0; i < columnVector.size(); ++i) {
        Column* c = columnVector[i];
        delete c;
    }
}

bool cdr::db::ResultSet::next()
{
    SQLRETURN rc = SQLFetch(st.getStmt());
    if (rc == SQL_NO_DATA_FOUND)
        return false;
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Database failure fetching data",
                             st.getErrorMessage(rc));
    return true;
}

cdr::String cdr::db::ResultSet::getString(int pos)
{
    if (pos > columnVector.size())
        throw cdr::Exception(L"ResultSet: column request out of range");
    SQLRETURN rc;
    Column* c = columnVector[pos - 1];
    wchar_t* data = new wchar_t[c->size + 1];
    memset(data, 0, (c->size + 1) * sizeof(wchar_t));
    SDWORD cb_data = c->size * sizeof(wchar_t);
    rc = SQLGetData(st.getStmt(), (UWORD)pos, SQL_C_WCHAR, (PTR)data,
                    cb_data, &cb_data);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception("Database failure extracting data",
                             st.getErrorMessage(rc));
    return cdr::String(data);
}

int cdr::db::ResultSet::getInt(int pos)
{
    SQLRETURN rc;
    int data;
    SDWORD cb_data;
    rc = SQLGetData(st.getStmt(), (UWORD)pos, SQL_C_SLONG, (PTR)&data, 0,
                    &cb_data);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception("Database failure extracting data",
                             st.getErrorMessage(rc));
    return data;
}
