/*
 * $Id: CdrDbStatement.h,v 1.4 2000-04-22 15:36:15 bkline Exp $
 *
 * Wrapper for ODBC HSTMT.  Modeled after JDBC interface.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/04/22 09:36:24  bkline
 * Added close() method.
 *
 * Revision 1.2  2000/04/17 21:27:02  bkline
 * Added nullability for ints and strings.
 *
 * Revision 1.1  2000/04/15 12:16:29  bkline
 * Initial revision
 */

#ifndef CDR_DB_STATEMENT_
#define CDR_DB_STATEMENT_

#include <vector>
#include "CdrDbConnection.h"
#include "CdrInt.h"

namespace cdr {
    namespace db {

        class ResultSet;

        /**
         * Implements JDBC-like interface of same name for queries
         * submitted to the CDR database.
         */
        class Statement {

            /**
             * The ResultSet class is granted access to the private
             * members of Statement in order to eliminate the need for
             * public accessor methods for ODBC-specific handles.
             */
            friend ResultSet;

         public:
            Statement(Connection&);
            ~Statement();

            /**
             * Registers a wide string parameter for a query.
             */
            void        setString(int, const cdr::String&);

            /**
             * Registers a narrow string parameter for a query.
             *
             *  @deprecated
             */
            void        setString(int, const std::string&, bool = false);

            /**
             * Registers an integer parameter for a query.  Uses
             * the cdr::Int class, which can represent a NULL value.
             */
            void        setInt(int, const cdr::Int&);

            /**
             * Submits a query to the CDR database, and returns a
             * ResultSet object for retrieving rows, if any.
             */
            ResultSet   executeQuery(const char*);

            /**
             * Closes any open cursors associated with the query,
             * making it available for re-use.
             */
            void        close();

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
            cdr::String getErrorMessage(SQLRETURN);
            void clearParms();
        };
    }
}

#endif
