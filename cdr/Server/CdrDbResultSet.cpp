/*
 * $Id: CdrDbResultSet.cpp,v 1.4 2000-04-26 01:24:05 bkline Exp $
 *
 * Implementation for ODBC result fetching wrapper (modeled after JDBC).
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/04/22 22:15:04  bkline
 * Added more comments.
 *
 * Revision 1.2  2000/04/17 21:26:06  bkline
 * Added nullability for ints and strings.
 *
 * Revision 1.1  2000/04/15 12:21:38  bkline
 * Initial revision
 */

#include "CdrDbResultSet.h"

/**
 * Gathers information about the columns of the results of the current query
 * in preparation for retrieving the results.
 */
cdr::db::ResultSet::ResultSet(cdr::db::Statement& s) : st(s)
{
    SQLSMALLINT nCols;
    SQLRETURN   rc = SQLNumResultCols(st.hstmt, &nCols);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Database failure creating result set",
                             st.getErrorMessage(rc));
    for (SQLSMALLINT i = 1; i <= nCols; ++i) {
        Column* c= new Column;
        char name[1024];
        SQLSMALLINT cbName;
        rc = SQLDescribeCol(st.hstmt, (SQLSMALLINT)i, (SQLCHAR *)name, 
                            sizeof name, &cbName, &c->type, &c->size, 
                            &c->digits, &c->nullable);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
            throw cdr::Exception(L"Database failure fetching column info",
                                 st.getErrorMessage(rc));
        c->name = std::string(name, cbName);
        columnVector.push_back(c);
    }
}

/**
 * Discards the column information for the query.
 */
cdr::db::ResultSet::~ResultSet()
{
    for (size_t i = 0; i < columnVector.size(); ++i) {
        Column* c = columnVector[i];
        delete c;
    }
}

/**
 * Returns true if there is another row to be fetched and prepares to extract
 * the values.  A future enhancement may actually extract the values here, in
 * order to support the getXXX() methods in any order, and multiple times for
 * the same value.
 */
bool cdr::db::ResultSet::next()
{
    SQLRETURN rc = SQLFetch(st.hstmt);
    if (rc == SQL_NO_DATA_FOUND)
        return false;
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception(L"Database failure fetching data",
                             st.getErrorMessage(rc));
    return true;
}

/**
 * Extracts the string value for the <code>pos</code> of the result row.
 */
cdr::String cdr::db::ResultSet::getString(int pos)
{
    if (pos > columnVector.size())
        throw cdr::Exception(L"ResultSet: column request out of range");
    SQLRETURN rc;
    Column* c = columnVector[pos - 1];
    bool isBlob = c->size > 10000;
    size_t bufSize = isBlob ? 0 : c->size + 1;
    wchar_t* data = new wchar_t[bufSize];
    memset(data, 0, bufSize * sizeof(wchar_t));
    SDWORD cbData = (bufSize) * sizeof(wchar_t);
    rc = SQLGetData(st.hstmt, (UWORD)pos, SQL_C_WCHAR, (PTR)data,
                    cbData, &cbData);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        delete [] data;
        throw cdr::Exception("Database failure extracting data",
                             st.getErrorMessage(rc));
    }
    if (cbData == SQL_NULL_DATA) {
        delete[] data;
        return cdr::String(true);
    }
    if (isBlob) {
        delete[] data;
        size_t chars = cbData / sizeof(wchar_t);
        data = new wchar_t[chars + 1];
        data[chars] = 0;
        rc = SQLGetData(st.hstmt, (UWORD)pos, SQL_C_WCHAR, (PTR)data,
                cbData + sizeof(wchar_t), &cbData);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            delete [] data;
            throw cdr::Exception("Database failure extracting data",
                                 st.getErrorMessage(rc));
        }
    }

    cdr::String s(data, cbData / sizeof(wchar_t));
    delete [] data;
    return s;
}

/**
 * Extracts the integer value for the <code>pos</code> of the result row.
 */
cdr::Int cdr::db::ResultSet::getInt(int pos)
{
    SQLRETURN rc;
    int data;
    SDWORD cbData;
    rc = SQLGetData(st.hstmt, (UWORD)pos, SQL_C_SLONG, (PTR)&data, 0,
                    &cbData);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        throw cdr::Exception("Database failure extracting data",
                             st.getErrorMessage(rc));
    if (cbData == SQL_NULL_DATA)
        return cdr::Int(true);
    return data;
}
