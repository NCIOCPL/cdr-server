/*
 * $Id: CdrDbConnection.h,v 1.7 2000-05-03 18:49:50 bkline Exp $
 *
 * Interface for CDR wrapper for an ODBC database connection.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2000/05/03 15:37:54  bkline
 * Fixed creation of db statements.
 *
 * Revision 1.5  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.4  2000/04/22 15:35:15  bkline
 * Filled out documentation comments.  Made all ODBC-specific members
 * private.
 *
 * Revision 1.3  2000/04/22 09:35:43  bkline
 * Added transaction support and getLastIdent() method.
 *
 * Revision 1.2  2000/04/15 12:08:40  bkline
 * First non-stub version.
 *
 * Revision 1.1  2000/04/14 15:58:42  bkline
 * Initial revision
 */

#ifndef CDR_DB_CONNECTION_
#define CDR_DB_CONNECTION_

#include <windows.h>
#include <sqlext.h>
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDbStatement.h"
#include "CdrDbPreparedStatement.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        // Forward references.
        class ResultSet;

        /**
         * Implements JDBC-like interface of same name for CDR database
         * connections.  Also includes a non-standard method for asking 
         * SQL Server for the current IDENTITY value.
         */
        class Connection {

            /**
             * These classes are granted access to Connection's private
             * members to eliminate the need for public getEnv() and
             * getDbc() accessor methods.
             */
            friend Statement;
            friend ResultSet;

        public:

            /**
             * Default constructor.  A future version will be made private, 
             * with a new Driver friend class, which will handle connection 
             * pooling in its connect() method (in keeping with JDBC
             * interfaces).
             */
            Connection();

            /**
             * Closes the database connection, releasing all resources
             * allocated by the connection.
             */
            ~Connection();

            /**
             * Creates an object which can handle a parameterized SQL
             * query.
             *
             *  @param  query       reference to a string object containing
             *                      the SQL query to be prepared.  Note that
             *                      the query string uses 8-bit ASCII
             *                      characters, but string parameters are
             *                      Unicode, encoded as UCS-16.
             *  @return             newly constructed
             *                      <code>PreparedStatement</code> object.
             */
            PreparedStatement prepareStatement(const std::string& query);

            /**
             * Creates an object which can handle an unparamaterized
             * SQL query.
             *
             *  @return             new <code>Statement</code> object.
             */
            Statement createStatement();

            /**
             * Turning off auto-commit starts an open transaction,
             * which can be committed or rolled back.  A new connection
             * is started with auto-commit turned on, effectively making 
             * each query into a self-contained transaction, committed
             * if successful.
             *
             *  @param  setting     <code>true</code> if subsequent queries
             *                      are to be treated as separate
             *                      self-contained transactions;
             *                      <code>false</code> to open a
             *                      multi-statement transaction.
             */
            void setAutoCommit(bool setting);

            /**
             * Returns <code>false</code> if the statements issued for the
             * connection are enclosed in a single transaction until the next
             * <code>commit()</code> or <code>rollback</code> transaction.
             *
             *  @return             current <code>autoCommit</code> setting.
             */
            bool getAutoCommit() const { return autoCommit; }

            /**
             * Causes the outstanding SQL statements for the current
             * transaction to be committed to the database.
             */
            void commit();

            /**
             * Causes the outstanding SQL statements for the current
             * transaction to be discarded.
             */
            void rollback();

            /**
             * Extracts the current @@IDENTITY value from SQL Server for
             * the current connection.
             *
             *  @return             current <code>@@IDENTITY</code value.
             */
            int getLastIdent();

            /**
             * Returns a string representing the CDR DBMS's idea of the
             * current time.
             *
             *  @return             date/time string in ISO format.
             */
            cdr::String getDateTimeString();

        private:

            /**
             * Creates a string which holds the concatenated ODBC error
             * messages from the ODBC driver and from the database.
             *
             *  @param  rc          return code from last ODBC call.
             *  @param  henv        ODBC environment handle.
             *  @param  hdbc        ODBC database connection handle.
             *  @param  hstmt       ODBC statement handle.
             *  @return             error information string.
             */
            static cdr::String getErrorMessage(SQLRETURN, HENV, HDBC, HSTMT);

            /**
             * ODBC database connection handle.
             */
            HDBC hdbc;

            /**
             * ODBC environment handle.
             */
            static HENV henv;

            /**
             * Flag remembering whether a multi-statement transaction is open.
             */
            bool autoCommit;

            /**
             * Blocked copy constructor.
             *
             *  @param  c           reference to <code>Connection</code>
             *                      object.
             */
            Connection(const Connection&);

            /**
             * Blocked assignment operator.
             *
             *  @param  c           reference to <code>Connection</code>
             *                      object.
             *  @return             reference to modified
             *                      <code>Connection</code> object.
             */
            Connection& operator=(const Connection&);
        };
    }
}

#endif
