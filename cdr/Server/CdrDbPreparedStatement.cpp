/*
 * $Id: CdrDbPreparedStatement.cpp,v 1.3 2000-10-23 14:09:31 mruben Exp $
 *
 * Implementation of class for prepared CDR SQL queries.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/08/10 15:00:22  bkline
 * Fixed problem with string values exactly 2000 characters long.
 *
 * Revision 1.1  2000/05/03 15:17:10  bkline
 * Initial revision
 *
 */

#include <iostream>
#include "CdrDbPreparedStatement.h"
#include "CdrDbResultSet.h"
#include "CdrException.h"

/**
 * Copy constructor.  Blocks copying of objects already in use.  I believe
 * that for most cases MSVC++ is enforcing the access to the copy constructor,
 * but then optimizing it away, so this doesn't get invoked.  I think that's
 * ok in this case, but it could be a source of bugs.  Check with Mike and see
 * what he thinks.
 */
cdr::db::PreparedStatement::PreparedStatement(const PreparedStatement& ps)
    : Statement(ps)
{
    std::cerr << "COPY CONSTRUCTOR FOR PreparedStatement; *pRefCount="
        << *pRefCount << '\n';
    if (paramVector.size() > 0)
        throw cdr::Exception(L"PreparedStatement is already in use"
                             L" and cannot be copied");
}

cdr::db::PreparedStatement::~PreparedStatement()
{
    // Will be decremented by base class destructor.
    //std::cerr << "PreparedStatement destructor: *pRefCount=" << *pRefCount
    //          << '\n';
    if (*pRefCount == 1)
        clearParameters();
}

/**
 * Releases the parameter information for the current query and resets the
 * statement handle.
 */
void cdr::db::PreparedStatement::close()
{
    //std::cerr << "PreparedStatement::close()\n";
    clearParameters();
    closeStatement();
}

/**
 * Releases the parameter information for the current query.
 */
void cdr::db::PreparedStatement::clearParameters()
{
    //std::cerr << "PreparedStatement::clearParameters()\n";
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
cdr::db::ResultSet cdr::db::PreparedStatement::executeQuery()
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
    return static_cast<cdr::db::Statement*>(this)->executeQuery(query.c_str());
}

/**
 * Saves a copy of the string value <code>val</code> for the <code>pos</code>
 * parameter of the current query.
 */
void cdr::db::PreparedStatement::setString(int pos, const cdr::String& val)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_WCHAR;
    p->sType     = val.size() >= 2000 ? SQL_WLONGVARCHAR : SQL_WVARCHAR;
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

#if 0
/**
 * Saves a copy of the string value <code>val</code> for the <code>pos</code>
 * parameter of the current query.  This version handles narrow-character
 * strings.  The third parameter is optional and defaults to false.
 * 
 *  @deprecated
 */
void cdr::db::PreparedStatement::setString(int pos, const std::string& val, bool null)
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
#endif

/**
 * Saves a copy of the integer value <code>val</code> for the <code>pos</code>
 * parameter of the current query.
 */
void cdr::db::PreparedStatement::setInt(int pos, const cdr::Int& val)
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
 * Saves a copy of the blob value <code>val</code> for the <code>pos</code>
 * parameter of the current query.
 */
void cdr::db::PreparedStatement::setBytes(int pos, const cdr::Blob& val)
{
    Parameter* p = new Parameter;
    p->position  = pos;
    p->cType     = SQL_C_BINARY;
    p->sType     = val.size() >= 2000 ? SQL_LONGVARBINARY : SQL_VARBINARY; 
    if (val.isNull()) {
        p->len   = 1; // Required by ODBC even for NULL data!
        p->value = 0;
        p->cb    = SQL_NULL_DATA;
    }
    else {
        p->len   = val.size();
        p->value = new unsigned char[val.size()];
        p->cb    = p->len;
        memcpy(p->value, val.c_str(), p->len);
    }
    paramVector.push_back(p);
}

