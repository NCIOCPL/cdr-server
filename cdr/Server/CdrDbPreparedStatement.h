/*
 * $Id: CdrDbPreparedStatement.h,v 1.1 2000-05-03 15:38:34 bkline Exp $
 *
 * Specialized Statement class for handling parameterized queries.
 *
 * $Log: not supported by cvs2svn $
 */

#ifndef CDR_DB_PREPARED_STATEMENT_
#define CDR_DB_PREPARED_STATEMENT_

#include "CdrDbStatement.h"

/**@#-*/

namespace cdr {
    namespace db {

/**@#+*/

        /** @pkg cdr.db */

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
             */
            ResultSet   executeQuery();

            /**
             * Releases the parameter information for the current query.
             */
            void        clearParameters();

            /**
             * Registers a wide string parameter for a query.
             */
            void        setString(int, const cdr::String&);

            /**
             * Registers an integer parameter for a query.  Uses
             * the cdr::Int class, which can represent a NULL value.
             */
            void        setInt(int, const cdr::Int&);

            /**
             * Closes any open cursors associated with the query,
             * making it available for re-use.  Also clears parameter list.
             */
            virtual void    close();

            ~PreparedStatement();

            /**
             * Copy constructor which blocks copying of the parameter
             * vector.
             */
            PreparedStatement(const PreparedStatement& s);

        private:

            PreparedStatement(Connection& c, const std::string& q) 
                : Statement(c), query(q) {}

            std::string query;
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
            PreparedStatement& operator=(const PreparedStatement&); // Block.
        };
    }
}

#endif
