/*
 * $Id: CdrDbStatement.h,v 1.6 2000-05-03 15:40:45 bkline Exp $
 *
 * Wrapper for ODBC HSTMT.  Modeled after JDBC interface.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2000/04/22 18:57:38  bkline
 * Added ccdoc comment markers for namespaces and @pkg directives.
 *
 * Revision 1.4  2000/04/22 15:36:15  bkline
 * Filled out documentation comments.  Made all ODBC-specific members
 * private.
 *
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
            virtual ~Statement();

            /**
             * Submits a query to the CDR database, and returns a
             * ResultSet object for retrieving rows, if any.
             */
            ResultSet       executeQuery(const char*);

            /**
             * Closes any open cursors associated with the query,
             * making it available for re-use.
             */
            virtual void    close();

            /**
             * Copy constructor.
             */
            Statement(const Statement&);

         protected:

            Statement(Connection&);
            Connection& conn;
            HSTMT       hstmt;
            cdr::String getErrorMessage(SQLRETURN);
            int         refCount;
            int*        pRefCount;
            void        closeStatement();

        private:

            Statement& operator=(const Statement&);     // Block this.
        };
    }
}

#endif
