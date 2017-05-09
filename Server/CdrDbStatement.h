/*
 * Wrapper for ODBC HSTMT.  Modeled after JDBC interface.
 */

#ifndef CDR_DB_STATEMENT_
#define CDR_DB_STATEMENT_

#include <vector>
#include <windows.h>
#include <sqlext.h>
//#include "CdrDbConnection.h"
#include "CdrString.h"
#include "CdrInt.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        class ResultSet;
        class Connection;

        /**
         * Implements JDBC-like interface of same name for queries
         * submitted to the CDR database.
         */
        class Statement {

            /**
             * These classes are granted access to the private members
             * of Statement in order to eliminate the need for public
             * accessor methods for ODBC-specific handles.
             */
            friend ResultSet;
            friend Connection;

         public:

            /**
             * Releases the statement handle after all copies of the object
             * have been destroyed.  It is important that the Statement
             * object not be destroyed if any outstanding ResultSet objects
             * will still be depending on it.
             */
            virtual ~Statement();

            /**
             * Submits a query to the CDR database, and returns a
             * ResultSet object for retrieving rows, if any.
             *
             *  @param  q       address of null-terminated string containing
             *                  the SQL query to be executed.
             *  @return         object from which query results can be
             *                  retrieved.
             */
            ResultSet       executeQuery(const char* q);

            /**
             * Submits a SQL request to the CDR database and returns the
             * number of rows affected for an UPDATE, INSERT, or DELETE
             * statement.  Other SQL statements (including DDL) can be
             * submitted using this method, but if the query is not an
             * UPDATE, INSERT, or DELETE statement the return value is
             * undefined.
             *
             *  @param  query   address of null-terminated string containing
             *                  the SQL query to be executed.
             *  @return         number of rows affected, if applicable.
             */
            int executeUpdate(const char* query);

            /**
             * Returns the current result as a ResultSet object.
             *
             *  @return         new <code>ResultSet</code> object.
             */
            ResultSet getResultSet();

            /**
             * Closes any open cursors associated with the query,
             * making it available for re-use.
             */
            virtual void    close();

            /**
             * Move to the statement object's next result set, if available.
             *
             *  @return         <code>true</code> if another result set is
             *                  available; otherwise <code>false</code>.
             */
            bool getMoreResults();
            /**
             * Copy constructor.  Uses reference counting to prevent premature
             * release of the statement handle.
             *
             *  @param  st      reference to the object to be copied.
             */
            Statement(const Statement& st);

         protected:

            /**
             * Creates a new <code>Statement</code> object using an open CDR
             * database connection.  Can only be invoked by the
             * <code>Connection</code> class or by the derived
             * <code>PreparedStatement</code> class.
             *
             *  @param  c           reference to the open CDR database
             *                      connection object.
             */
            Statement(Connection& c);

            /**
             * Reference to the object representing the open CDR database
             * connection.
             */
            Connection& conn;

            /**
             * ODBC statement handle.
             */
            HSTMT       hstmt;

            /**
             * Constructs a string containing the concatenation of all ODBC
             * driver and database errors.
             *
             *  @param  rc          result code from last ODBC call.
             */
            cdr::String getErrorMessage(SQLRETURN rc);

            /**
             * Reference count used to prevent premature release of statement
             * resources.  This address is shared by all copies of the object.
             */
            int*        pRefCount;

            /**
             * Closes the ODBC statement, allowing it to be re-used.
             */
            void        closeStatement();

        private:

            /**
             * Unimplemented (blocked) assignment operator.
             *
             *  @param  st      reference to object to be copied.
             *  @return         reference to modified object.
             */
            Statement& operator=(const Statement& st);
        };
    }
}

#endif
