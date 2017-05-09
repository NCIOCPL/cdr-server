/*
 * Write log strings to log database or file.
 *
 *                                          Alan Meyer  June, 2000
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

    class LogTime {

        private:
            /**
             * Milliseconds since system boot.  Used for comparing times.
             *
             * NOTE: Clock resolution may differ on different hardware and
             * different versions of Windows.  The same program produced
             * different results when tested on one of our Windows Server
             * 2008 machines and one of our Windows 7 machines.
             *
             * The Windows API function always produces a millisecond count
             * but it's not always accurate to the millisecond.
             */
            DWORD tickCount;

            // Structure containing year, month, day, etc.
            SYSTEMTIME sysTime;

        public:
            /**
             * Constructor.  Initializes the LogTime object with the current
             * time at the instant of construction.
             */
            LogTime();

            /**
             * Get the count of millisecond clock ticks since the last
             * boot.
             *
             *  @return     Milliseconds since last boot as an unsigned long.
             */
            unsigned long getTickCount() {
                return (unsigned long) tickCount;
            }

            /**
             * Format the local time stored in the object to a string.
             *
             * Example: "2013-08-13 17:07:44.123"
             *
             *  @return             String formatted as above.
             */
            cdr::String getLocalTimeStr();

            /**
             * Get the difference, in milliseconds, between two LogTime
             * objects.
             *
             *  @param startTime    Reference to a LogTime object previously
             *                      initialized.
             *
             *  @return             Difference, in seconds between this
             *                      and the passed object.
             */
            unsigned long diffTime(const LogTime &startTime);

            /**
             * Difference in seconds, as a string, with decimal milliseconds.
             *
             * Example: "297.123"
             *
             *  @param startTime    Reference to a LogTime object previously
             *                      initialized.
             *
             *  @return             Difference, in seconds between this
             *                      and the passed object.
             */
            cdr::String diffSecondsStr(const LogTime &startTime);

            /**
             * Difference between two objects in a human readable string,
             * formatted into days, hours, minutes, seconds.
             *
             * Example: "3d 17h 5m 1.513s"
             *
             *  @param startTime    Reference to a LogTime object previously
             *                      initialized.
             *
             *  @return             Difference, as human readable string.
             */
            cdr::String diffTimeStr(const LogTime &startTime);

            /**
             * Add milliseconds to a tickCount.  Used only for testing.
             *
             *  @param addCount     Number of milliseconds to add.
             */
            void addTickCount(unsigned long addCount);
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
