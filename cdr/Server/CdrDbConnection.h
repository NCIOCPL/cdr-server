/*
 * $Id: CdrDbConnection.h,v 1.2 2000-04-15 12:08:40 bkline Exp $
 *
 * Interface for CDR wrapper for an ODBC database connection.
 *
 * $Log: not supported by cvs2svn $
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
            HENV getEnv() { return henv; }
            HDBC getDbc() { return hdbc; }
            static cdr::String getErrorMessage(SQLRETURN, HENV, HDBC, HSTMT);
        private:
            HDBC hdbc;
            static HENV henv;
        };
    }
}

#endif
