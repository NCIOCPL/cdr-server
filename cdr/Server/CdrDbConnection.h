/*
 * $Id: CdrDbConnection.h,v 1.3 2000-04-22 09:35:43 bkline Exp $
 *
 * Interface for CDR wrapper for an ODBC database connection.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/15 12:08:40  bkline
 * First non-stub version.
 *
 * Revision 1.1  2000/04/14 15:58:42  bkline
 * Initial revision
 *
 */

#ifndef CDR_DB_CONNECTION_
#define CDR_DB_CONNECTION_

#include <windows.h>
#include <sqlext.h>
#include "CdrString.h"
#include "CdrException.h"

namespace cdr {
    namespace db {
        class Connection {
        public:
            Connection();
            ~Connection();
            bool getAutoCommit() const { return autoCommit; }
            void setAutoCommit(bool);
            void commit();
            void rollback();
            int getLastIdent();
            HENV getEnv() { return henv; }
            HDBC getDbc() { return hdbc; }
            static cdr::String getErrorMessage(SQLRETURN, HENV, HDBC, HSTMT);
        private:
            HDBC hdbc;
            static HENV henv;
            bool autoCommit;
        };
    }
}

#endif
