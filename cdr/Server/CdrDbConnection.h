/*
 * $Id: CdrDbConnection.h,v 1.5 2000-04-22 18:57:38 bkline Exp $
 *
 * Interface for CDR wrapper for an ODBC database connection.
 *
 * $Log: not supported by cvs2svn $
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

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

        // Forward references.
        class Statement;
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
             * These will be made private, with a new Driver friend class,
             * which will handle connection pooling in its connect() method.
             */
            Connection();
            ~Connection();

            /**
             * Turning off auto-commit starts an open transaction,
             * which can be committed or rolled back.  A new connection
             * is started with auto-commit turned on, effectively making 
             * each query into a self-contained transaction, committed
             * if successful.
             */
            bool getAutoCommit() const { return autoCommit; }
            void setAutoCommit(bool);
            void commit();
            void rollback();

            /**
             * Extracts the current @@IDENTITY value from SQL Server for
             * the current connection.
             */
            int getLastIdent();

        private:
            static cdr::String getErrorMessage(SQLRETURN, HENV, HDBC, HSTMT);
            HDBC hdbc;
            static HENV henv;
            bool autoCommit;
        };
    }
}

#endif
