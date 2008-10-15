/*
 * $Id: CdrLog.h,v 1.5 2008-10-15 02:35:49 ameyer Exp $
 *
 * Write log strings to log database or file.
 *
 *                                          Alan Meyer  June, 2000
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2002/08/10 19:27:24  bkline
 * Added GetId() access method.
 *
 * Revision 1.3  2002/07/11 18:55:27  ameyer
 * Added directory prefix for default logfile name.
 *
 * Revision 1.2  2000/10/05 15:21:40  ameyer
 * Added WriteFile for logging to OS file instead of database.
 *
 * Revision 1.1  2000/06/15 22:32:49  ameyer
 * Initial revision
 *
 */

#ifndef CDR_LOG_
#define CDR_LOG_

#include <cstdlib>
#include "CdrDbStatement.h"

/**@#-*/

namespace cdr {
  namespace log {

/**@#+*/

    /** @pkg cdr */

    /**
     * Max length of a message identifier string
     * More is truncated.
     */
    static const int SrcMaxLen = 64;

    /**
     * Max length of a message to be logged.
     * More is truncated.
     */
    static const size_t MsgMaxLen = 3800;

    /**
     * Thread global pointer to thread specific instance of a log object.
     * Defined in (CdrLog.cpp) dispatcher().
     * Any function at a call level lower than dispatcher() can write
     *   to the log database using the following semantics:
     *     cdr::log::pThreadLog->Write (Identifying string, Message string);
     *   See cdr::log::Write().
     * Can't completely eliminate the global, but at least it's hidden
     *   in the cdr::log namespace.
     */
    class Log;
    extern __declspec(thread) cdr::log::Log *pThreadLog;

    /**
     * Class for logging cdr::Strings
     */
    class Log {

        public:
            /**
             * Constructor
             */
            Log ();

            /**
             * Write source writer identifier + message
             *
             *  @param MsgSrc   Identify source of message - module or
             *                  whatever - as cdr::String.
             *  @param Msg      Message to write, as cdr::String.
             */
            void Write (const cdr::String MsgSrc, const cdr::String Msg);

            /**
             * Write source writer identifier + message using an existing
             * database connection - a more efficient approach when a
             * connection exists.
             *
             *  @param MsgSrc   Identify source of message - module or
             *                  whatever - as cdr::String.
             *  @param Msg      Message to write, as cdr::String.
             *  @param dbConn   Existing, open, database connection.
             */
            void Write (const cdr::String MsgSrc, const cdr::String Msg,
                        cdr::db::Connection& dbConn);

            /**
             * Tell which id we're using for logging this thread.
             *
             *  @return     integer used to identify logging for this thread.
             */
            int GetId() const { return LogId; }

        private:
            /**
             * Named mutex used by all threads to synchronize assignment
             * of log file thread identifiers.
             */
            static HANDLE s_hLogMutex;

            /**
             * Incrementing thread id counter for generating process unique
             * log thread ids.
             */
            static int s_LogId;

            /**
             * Thread log identifier for current thread
             */
            int LogId;

    };

    /**
     * Change the default logging directory.
     * Must call this before any logging is done.
     *
     *  @param dir      Windows directory e.g., "d:/cdr/logdir", "c:/testlogs"
     */
    extern void setDefaultLogDir(std::string dir);

    /**
     * Get the current logging directory
     *
     * @return          String like "d:/cdr/Log"
     */
    extern std::string getDefaultLogDir();

    /**
     * Write to an OS file, either passing the name or using
     * a default name.
     *
     *  @param msgSrc   Identify source of message - module or
     *                  whatever - as cdr::String.
     *  @param msg      Message to write, as cdr::String.
     *  @param fname    Name of file, if defaulted to "", uses default log
     *                  directory established by setDefaultLogDir() or by
     *                  cdr::log::WriteFile().
     */
    extern void WriteFile (const cdr::String msgSrc, const cdr::String msg,
                           std::string fname = "");
  }
}

#endif // CDR_LOG_
