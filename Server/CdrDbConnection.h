/*
 * $Id$
 *
 * Interface for CDR wrapper for an ODBC database connection.
 */

#ifndef CDR_DB_CONNECTION_
#define CDR_DB_CONNECTION_

#include <windows.h>
#include <sqlext.h>
#include <io.h>
#include "CdrString.h"
#include "CdrException.h"
#include "CdrDbStatement.h"
#include "CdrDbPreparedStatement.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        /**
         * Flag indicating we've migrated the server to CBIIT hosting.
         */
        static const bool CBIIT_HOSTING = _access("d:\\cdr\\etc\\dbhost",
                                                  0) == 0;

        /**
         * Default url for connection.
         */
        static const wchar_t * const url = L"odbc:cdr";

        /**
         * Default user ID for connection.
         */
        static const wchar_t * const uid = CBIIT_HOSTING ? L"CDRSQLACCOUNT"
                                                         : L"cdr";

        /**
         * Lookup the database password used for the current hosting
         * environment (CBIIT, OCE), the current tier (DEV, QA, PROD),
         * and the CdrServer application.
         *
         * We used to store passwords as constants here, similar to
         * "url" above.
         *
         *  @return Password as a constant wide character C string.
         */
        const cdr::String getCdrDbPw();

        // Forward references.
        class ResultSet;
        class DriverManager;

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
            friend DriverManager;

        public:

            /**
             * Makes a safe copy of the <code>Connection</code> object
             * which knows when the master copy has been closed.
             *
             *  @param  c           reference to <code>Connection</code>
             *                      object.
             */
            Connection(const Connection&);

            /**
             * Makes a safe copy of the <code>Connection</code> object
             * which knows when the master copy has been closed.
             *
             *  @param  c           reference to <code>Connection</code>
             *                      object.
             *  @return             reference to modified
             *                      <code>Connection</code> object.
             */
            Connection& operator=(const Connection&);

            /**
             * Closes the database connection, releasing all resources
             * allocated by the connection.
             */
            ~Connection();

            /**
             * Closes the database connection, releasing all resources
             * allocated by the connection.
             */
            void close();

            /**
             * Reports status of connection.
             *
             *  @return             <code>true</code> iff connection has
             *                      been closed, or if has never been
             *                      opened.
             */
            bool isClosed() const { return master->hdbc == SQL_NULL_HDBC; }

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
             * if successful.  When auto-commit is turned off, any
             * open transactions on the connection are committed.
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
            bool getAutoCommit() const { return master->autoCommit; }

            /**
             * Causes the outstanding SQL statements for the current
             * transaction to be committed to the database.  Unlike
             * an explicit "COMMIT WORK" query (which will fail if
             * no transaction is active), this operation is a safe
             * no-op if no transaction is open when <code>commit()</code>
             * is invoked.
             */
            void commit();

            /**
             * Causes the outstanding SQL statements for the current
             * transaction to be discarded.  Unlike an explicit
             * "ROLLBACK WORK" query (which will fail if no
             * transaction is active), this operation is a safe
             * no-op if no transaction is open when <code>rollback()</code>
             * is invoked.
             */
            void rollback();

            /**
             * Extracts the current @@IDENTITY value from SQL Server for
             * the current connection.
             *
             *  @return             current <code>@@IDENTITY</code> value.
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
             * Private constructor.  Only the <code>DriverManager</code>
             * class is used for creating new <Code>Connection</code>
             * objects, in keeping with the JDBC interfaces.
             */
            Connection(const SQLCHAR*, const SQLCHAR*, const SQLCHAR*);

            /**
             * Unimplemented (blocked) default constructor.
             */
            Connection();

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
            HDBC        hdbc;

            /**
             * ODBC environment handle.
             */
            static HENV henv;

            /**
             * Flag remembering whether a multi-statement transaction is open.
             */
            bool        autoCommit;

            /**
             * Shared address of the object used by the copy constructor and
             * assignment operator.
             */
            Connection* master;

            /**
             * Reference counting support.
             */
            int         refCount;
        };

        /**
         * Class responsible for creating new connections.  Connection pooling
         * is used to reduce the performance penalty for frequent creation of
         * the <code>Connection</code> objects.
         */
        class DriverManager {

        public:

            /**
             * Opens a new connection to the CDR database.  Connection pooling
             * is used to reduce the number of actual connections which have
             * to be built up and torn down.
             *
             *  @param  url     string of the form "odbc:dsn" where dsn is
             *                  ODBC DSN for the connection.
             *  @param  uid     login ID for the database.
             *  @param  pwd     password for the database account.
             *  @return         new <code>Connection</code> object.
             */
            static Connection getConnection(const cdr::String& url,
                                            const cdr::String& uid,
                                            const cdr::String& pwd);
        };
    }
}

#endif
