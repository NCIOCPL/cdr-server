/*
 * $Id: CdrDbResultSet.h,v 1.2 2000-04-17 21:27:40 bkline Exp $
 *
 * Wrapper for ODBC result fetching.  Modeled after JDBC interface.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/15 12:16:59  bkline
 * Initial revision
 *
 */

#ifndef CDR_DB_RESULT_SET_
#define CDR_DB_RESULT_SET_

#include <vector>
#include <string>
#include "CdrDbStatement.h"

namespace cdr {
    namespace db {
        class ResultSet {
        public:
            ResultSet(Statement& s);
            ~ResultSet();
            bool        next();
            cdr::String getString(int);
            cdr::Int    getInt(int);
        private:
            Statement&  st;
            struct Column {
                std::string name;
                SQLSMALLINT type;
                SQLUINTEGER size;
                SQLSMALLINT digits;
                SQLSMALLINT nullable;
            };
            typedef std::vector<Column*> ColumnVector;;
            ColumnVector    columnVector;
        };
    }
}

#endif
