/*
 * $Id: CdrDbStatement.h,v 1.2 2000-04-17 21:27:02 bkline Exp $
 *
 * Wrapper for ODBC HSTMT.  Modeled after JDBC interface.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/04/15 12:16:29  bkline
 * Initial revision
 *
 */

#ifndef CDR_DB_STATEMENT_
#define CDR_DB_STATEMENT_

#include <vector>
#include "CdrDbConnection.h"
#include "CdrInt.h"

namespace cdr {
    namespace db {
        class ResultSet;
        class Statement {
         public:
            Statement(Connection&);
            ~Statement();
            void        setString(int, const cdr::String&);
            void        setString(int, const std::string&, bool = false);
            void        setInt(int, const cdr::Int&);
            ResultSet   executeQuery(const char*);
            HSTMT       getStmt() const { return hstmt; }
            HDBC        getDbc() const { return conn.getDbc(); }
            HENV        getEnv() const { return conn.getEnv(); }
            cdr::String getErrorMessage(SQLRETURN);
        private:
            Connection& conn;
            HSTMT hstmt;
            struct Parameter {
                int     position;
                void*   value;
                int     len;
                int     cType;
                int     sType;
                SDWORD  cb;
            };
            typedef std::vector<Parameter*> ParamVector;
            ParamVector paramVector;
        };
    }
}

#endif
