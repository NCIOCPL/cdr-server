/*
 * $Id: CdrDbPreparedStatement.h,v 1.4 2001-01-17 21:52:10 bkline Exp $
 *
 * Specialized Statement class for handling parameterized queries.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/10/05 18:16:31  mruben
 * added support for Blob
 *
 * Revision 1.2  2000/05/03 22:04:43  bkline
 * More ccdoc comments.
 *
 * Revision 1.1  2000/05/03 15:38:34  bkline
 * Initial revision
 *
 */

#ifndef CDR_DB_PREPARED_STATEMENT_
#define CDR_DB_PREPARED_STATEMENT_

#include "CdrDbStatement.h"
#include "CdrBlob.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        /**
         * Forward declarations.
         */
        class ResultSet;
        class Connection;

        /**
         * Implements JDBC-like interface of same name for parameterized
         * SQL queries submitted to the CDR database.
         */
        class PreparedStatement : public cdr::db::Statement {

        public:

            /**
             * Connection is the factory class for PreparedStatement objects.
             */
            friend Connection;

            /**
             * Submits the prepared query to the CDR database, and returns
             * a ResultSet object for retrieving rows, if any.
             *
             *  @return         <code>ResultSet</code> object which can
             *                  be used to retrieve results of the query, if
             *                  any.
             */
            ResultSet   executeQuery();

            /**
             * Submits a SQL request to the CDR database and returns the
             * number of rows affected for an UPDATE, INSERT, or DELETE 
             * statement.  Other SQL statements (including DDL) can be 
             * submitted using this method, but if the query is not an
             * UPDATE, INSERT, or DELETE statement the return value is
             * undefined.
             *
             *  @return         number of rows affected, if applicable.
             */
            int         executeUpdate();

            /**
             * Releases the parameter information for the current query.
             */
            void        clearParameters();

            /**
             * Registers a wide string parameter for a query.
             *
             *  @param  pos     position of the parameter in the query
             *                  string.
             *  @param  val     value to be plugged into the query.
             */
            void        setString(int pos, const cdr::String& val);

            /**
             * Registers an integer parameter for a query.  Uses
             * the cdr::Int class, which can represent a NULL value.
             *
             *  @param  pos     position of the parameter in the query
             *                  string.
             *  @param  val     value to be plugged into the query.
             */
            void        setInt(int pos, const cdr::Int& val);

            /**
             * Registers a Blob parameter for a query.  Uses
             * the cdr::Blob class, which can represent a NULL value.
             *
             *  @param  pos     position of the parameter in the query
             *                  string.
             *  @param  val     value to be plugged into the query.
             */
            void        setBytes(int pos, const cdr::Blob& val);
            
            /**
             * Closes any open cursors associated with the query,
             * making it available for re-use.  Also clears parameter list.
             */
            virtual void    close();

            /**
             * Frees the parameter list and releases the statement handle.
             */
            ~PreparedStatement();

            /**
             * Copy constructor which blocks copying of the parameter
             * vector.
             *
             *  @param  s       reference to <code>PreparedStatement</code>
             *                  to be copied.
             *  @exception      <code>cdr::Exception</code> if the statement
             *                  is already in use (that is, the parameter
             *                  vector is not empty).
             */
            PreparedStatement(const PreparedStatement& s);

        private:

            /**
             * Creates a new <code>PreparedStatement</code> class for the
             * specified connection and query string.  Invoked by the
             * connection object, which is the factory class for statements.
             *
             *  @param  c       reference to connection for the statement.
             *  @param  q       narrow-character string containing SQL
             *                  query for the statement.
             */
            PreparedStatement(Connection& c, const std::string& q) 
                : Statement(c), query(q) {}

            /**
             * String containing the SQL query for the statement.
             */
            std::string query;

            /**
             * Positional parameter to be plugged in just before the query is 
             * run.
             */
            struct Parameter {
                
                /**
                 * Position of the parameter in the SQL query string.
                 */
                int     position;

                /**
                 * Address of the parameter value to be plugged in.
                 */
                void*   value;

                /**
                 * Length of the parameter value in bytes (even for
                 * wide-character strings), including space for a
                 * null-terminated string for string values.
                 */
                int     len;

                /**
                 * ODBC token for type used by the C++ code (e.g.,
                 * SQL_C_WCHAR).
                 */
                int     cType;

                /**
                 * ODBC token for type used by the SQL version of the value
                 * (e.g., SQL_WVARCHAR).
                 */
                int     sType;

                /**
                 * Set to SQL_NTS (SQL Null-Terminated String) for string
                 * parameters.  Set to length of binary paramenters.
                 * Set to 0 (and ignored) for numeric types.
                 */
                SDWORD  cb;
            };

            /**
             * For convenience, and as a workaround for MSVC++ bugs.
             */
            typedef std::vector<Parameter*> ParamVector;

            /**
             * Remembers parameters to be bound to the query when it is
             * executed.
             */
            ParamVector paramVector;

            /**
             * Unimplemented (blocked) assignment operator.
             *
             *  @param  ps      reference to <code>PreparedStatement</code>
             *                  to be copied.
             *  @return         reference to modified object.
             */
            PreparedStatement& operator=(const PreparedStatement& ps);
        };
    }
}

#endif
