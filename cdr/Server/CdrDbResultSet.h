/*
 * $Id: CdrDbResultSet.h,v 1.5 2000-05-03 15:39:34 bkline Exp $
 *
 * Wrapper for ODBC result fetching.  Modeled after JDBC interface.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.3  2000/04/22 15:36:02  bkline
 * Filled out documentation comments.
 *
 * Revision 1.2  2000/04/17 21:27:40  bkline
 * Added nulability for ints and strings.
 *
 * Revision 1.1  2000/04/15 12:16:59  bkline
 * Initial revision
 */

#ifndef CDR_DB_RESULT_SET_
#define CDR_DB_RESULT_SET_

#include <vector>
#include <string>
#include "CdrDbStatement.h"
#include "CdrBlob.h"
#include "CdrString.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        /**
         * Implements JDBC-like interface for class of the same name for
         * extracting result rows from queries of the CDR database.
         */
        class ResultSet {
        public:
            ResultSet(Statement& s);
            ~ResultSet();

            /**
             * Copy constructor which blocks copying of the column
             * vector.
             */
            ResultSet(const ResultSet& rs) : st(rs.st) {}

            /**
             * Moves the <code>ResultSet</code> to the next row.  The first 
             * call makes the first row the current row.  The method returns 
             * <code>true</code> as long as there is a next row to move to.
             * If there are no further rows to process, it returns
             * <code>false</code>.
             */
            bool        next();

            /**
             * These methods return the specified column value for the current
             * row as the data type that matches the method name.  The data
             * types are capable of representing <code>NULL</code> values.
             */
            cdr::String getString(int);
            cdr::Int    getInt(int);
            cdr::Blob   getBytes(int);

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
            ResultSet& operator=(const ResultSet&);         // Block this.
        };
    }
}

#endif
