/*
 * Implementation of writing info to the log table in the database.
 * If that can't be done, takes an alternative action to write to file.
 *
 *                                          Alan Meyer  June, 2000
 */

#include <Windows.h> // For security descriptors for mutex
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <wchar.h>
#include <time.h>
#include "CdrString.h"
#include "CdrCommand.h"
#include "CdrDbConnection.h"
#include "CdrDbPreparedStatement.h"
#include "CdrDbStatement.h"
#include "CdrDbResultSet.h"
#include "CdrLog.h"

// Globals
extern short Glbl_DEFAULT_CDR_PORT;
extern short Glbl_CurrentServerPort;

/**
 * Thread global pointer to thread specific instance of a log object.
 * Microsoft specific approach to creating a variable in a single thread.
 *
 * This global variable makes it possible for any function, at any level
 * below the point where the pointer has been initialized (in
 * CdrServer.cpp - dispatcher()) to write the log table.
 *
 * The variable is thread specific so that we can write a thread identifier
 * to the log table.  Using this a programmer can read entries in the
 * log table which have been interleaved from multiple concurrent threads
 * and find entries from the same thread.
 */
cdr::log::Log __declspec(thread) * cdr::log::pThreadLog;

/**
 * Thread global variable to tell us if we're inside logging already.
 *
 * Problem we're dealing with here is that if an exception occurs
 * while processing an exception, we can keep trying to log things
 * inappropriately.
 */
static bool __declspec(thread) inLogging1 = false;
static bool __declspec(thread) inLogging2 = false;

/**
 * Directory for logging - for OSLogFile.
 * Also may be used elsewhere.
 * String is initially empty.  See getDefaultLogDir().
 * Changed from constant to variable to allow overrides
 * in invocation of the CdrServer.
 */
static std::string CdrLogDir;

/**
 * Name of OS based logfile.
 * This file is used if, and only if, the logger is unable to
 *   write to the debug_log table in the database.
 * Otherwise all log messages go to table debug_log.
 */
static std::string OSLogFile = "/CdrLogErrs";

// Time constants based on milliseconds
static const unsigned long LogT_sec = 1000LU;
static const unsigned long LogT_min = LogT_sec * 60LU;
static const unsigned long LogT_hr  = LogT_min * 60LU;
static const unsigned long LogT_day = LogT_hr  * 24LU;

// Class static variables
int    cdr::log::Log::s_LogId     = 0;
HANDLE cdr::log::Log::s_hLogMutex = 0;

// LogTime contructor
cdr::log::LogTime::LogTime() {
    // Win API function for milliseconds since sys startup
    tickCount = GetTickCount();

    // Win API function for date time
    GetLocalTime(&sysTime);
}

// Get date time in ISO format
cdr::String cdr::log::LogTime::getLocalTimeStr() {
    char buf[80];
    std::sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                 sysTime.wYear, sysTime.wMonth, sysTime.wDay,
                 sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
                 sysTime.wMilliseconds);
    cdr::String timeStr(buf);

    return timeStr;
}

// How many milliseconds between startTime and this.time
unsigned long cdr::log::LogTime::diffTime(const cdr::log::LogTime &startTime) {

    unsigned long difference =
        (unsigned long) (tickCount - startTime.tickCount);
    return difference;
}

// Difference between times, string format
cdr::String cdr::log::LogTime::diffSecondsStr(const cdr::log::LogTime &startTime) {
    unsigned long diffSecs = diffTime(startTime);
    unsigned long secs     = diffSecs / 1000UL;
    unsigned long mills    = diffSecs % 1000UL;

    std::stringstream os;
    os << secs << "." << mills;

    return cdr::String(os.str());
}


// Difference in long string format
cdr::String cdr::log::LogTime::diffTimeStr(const cdr::log::LogTime &startTime) {
    unsigned long diff  = diffTime(startTime);
    unsigned long days  = 0L;
    unsigned long hours = 0L;
    unsigned long mins  = 0L;
    unsigned long secs  = 0L;

    std::stringstream os;
    if (diff > LogT_day) {
        days  = diff / LogT_day;
        diff -= days * LogT_day;
        os << days << "d ";
    }
    if (diff > LogT_hr) {
        hours = diff / LogT_hr;
        diff -= hours * LogT_hr;
        os << hours << "h ";
    }
    if (diff > LogT_min) {
        mins  = diff / LogT_min;
        diff -= mins * LogT_min;
        os << mins << "m ";
    }
    if (diff > LogT_sec) {
        secs  = diff / LogT_sec;
        diff -= secs * LogT_sec;
        os << secs;
    }

    os << "." << diff << "s";

    return os.str();
}

// For test
void cdr::log::LogTime::addTickCount(unsigned long addCount) {
    tickCount += addCount;
}
/**
 * Log constructor.
 *   Creates object suitable for use anywhere in the current thread.
 *   Creates a unique thread identifier to stamp all messages written
 *      from this thread.
 */

cdr::log::Log::Log ()
{
    cdr::String ErrMsg = L"";        // For use in case of mutex error

    // No log id established yet for this instance
    LogId = 0;

    // If this is the first thread in the current process, create a mutex
    //   to control the static thread id counter.
    // Windows requires a security descriptor.
    // This is what we have to do if we want a mutex that works
    //   across processes.
    // 0 as security descriptor doesn't work right - causing failure
    //   to create or acquire the mutex if a second CdrServer starts.
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
    SECURITY_ATTRIBUTES sa = {sizeof sa, &sd, FALSE};
    if (s_hLogMutex == 0)
        s_hLogMutex = CreateMutex (&sa, false, "CdrLogMutex");

    // Create unique id for this log object
    if (s_hLogMutex != 0) {

        DWORD stat = WaitForSingleObject (s_hLogMutex, 5000);

        switch (stat) {

            case WAIT_OBJECT_0:
            case WAIT_ABANDONED:
                LogId = ++s_LogId;
                if (!ReleaseMutex (s_hLogMutex)) {
                    ErrMsg =
                        L"Can't release mutex!  Error=";
                    ErrMsg += cdr::String::toString (GetLastError ());
                }
                break;
            case WAIT_TIMEOUT:
                ErrMsg =
                   L"Timeout waiting for mutex for log id";
                break;
            default:
                ErrMsg =
                   L"Unknown mutex error - can't happen!";
        }
    }
    else {
        ErrMsg = L"Can't create mutex.  Error=";
        ErrMsg += cdr::String::toString (GetLastError ());
    }

    if (!ErrMsg.empty()) {

        // This should never happen
        // If it does, let object write itself to wherever critical errors go
        cdr::String src = L"Log constructor";
        WriteFile (src, ErrMsg);
    }
}

/**
 * Write ()
 *   Write something to the log file.
 *   Front end when no connection available.
 */

void cdr::log::Log::Write (
    const cdr::String MsgSrc,   // Name of module or whatever caller wants
    const cdr::String Msg       // Message
) {
    if (inLogging1) {
        cdr::String srcPlusMsg = MsgSrc + L"=" + Msg;
        WriteFile(L"Recursion in 2 arg Write", srcPlusMsg);
        return;
    }
    inLogging1 = true;

    // Create a database connection and write data
    try {
        cdr::db::Connection dbConn =
            cdr::db::DriverManager::getConnection (cdr::db::url,
                                                   cdr::db::uid,
                                                   cdr::db::getCdrDbPw(),
                                                   cdr::db::connLogWrite);
        this->Write (MsgSrc, Msg, dbConn);
    }
    catch (cdr::Exception& e) {
        // Failed to connect or failed to write
        // Use alternative instead
        WriteFile (L"CdrLog DB Write Failed", e.what());
        WriteFile (MsgSrc, Msg);
    }
    catch (...) {
        // Use alternative instead
        WriteFile (L"CdrLog DB Write Failed", L"Unknown exception");
        WriteFile (MsgSrc, Msg);
    }
    inLogging1 = false;
}


/**
 * Write ()
 *   Write something to the log file.
 *   Full write function.
 */

void cdr::log::Log::Write (
    const cdr::String MsgSrc,   // Name of module or whatever caller wants
    const cdr::String Msg,      // Message
    cdr::db::Connection& dbConn // New or existing connection
) {

    // Avoid looping
    if (inLogging2)
        return;
    inLogging2 = true;

    // Copies to handle truncation if necessary
    // Efficient since copy is only deep when truncation occurs
    cdr::String cpySrc = MsgSrc;
    cdr::String cpyMsg = Msg;

    // Truncate strings if necessary
    if (cpySrc.size() > cdr::log::SrcMaxLen)
        cpySrc = cpySrc.substr (0, cdr::log::SrcMaxLen);
    if (cpyMsg.size() > cdr::log::MsgMaxLen)
        cpyMsg = cpyMsg.substr (0, cdr::log::MsgMaxLen);

    // Construct update query
    std::string sqlstmt = "INSERT INTO debug_log ("
                            "thread, "
                            "recorded, "
                            "source, "
                            "msg) "
                          "VALUES ("
                            "?, "
                            "GETDATE(), "
                            "?, "
                            "?)";

    cdr::db::PreparedStatement insert = dbConn.prepareStatement (sqlstmt);

    insert.setInt    (1, LogId);
    insert.setString (2, cpySrc);
    insert.setString (3, cpyMsg);

    // Store it
    try {
        insert.executeQuery ();
    }
    catch (cdr::Exception&) {
        // Couldn't write.  Use exception reporting instead
        // Don't re-throw exception.  Would probably cause a loop.
        WriteFile (MsgSrc, Msg);
    }

    inLogging2 = false;
}

/**
 * setDefaultLogDir()
 *   Modifies the default log directory for WriteFile and any other software
 *   that might check via any subsequent calls to getDefaultLogDir().
 */
void cdr::log::setDefaultLogDir(std::string dir) {
    CdrLogDir = dir;
}

/**
 * getDefaultLogDir()
 *   Returns drive + directory.
 */
std::string cdr::log::getDefaultLogDir() {
    // If it's not set, initialize with default on current drive
    if (CdrLogDir.empty()) {
        char fname[] = "x:/cdr/log";
        cdr::db::replaceDriveLetter(fname);
        CdrLogDir = std::string(fname);
    }

    return CdrLogDir;
}


/**
 * WriteFile ()
 *   Writes to ordinary os file, if it can't be logged to db.
 *   But can also be called directly.
 *
 *  RMK (2001-10-29): replaced stdio output with ofstream output, to
 *                    avoid limitation on string length imposed by ANSI
 *                    standard for fwprintf.
 */

void cdr::log::WriteFile (
    const cdr::String msgSrc,   // Name of module or whatever caller wants
    const cdr::String msg,      // Message
          std::string fname     // Filename, may be defaulted
) {
    // First try to get exclusive control of the file
    HANDLE hMutex = CreateMutex (0, false, "CdrWriteFileMutex");
    DWORD stat    = WaitForSingleObject (hMutex, 5000);


    // Get datetime
    LogTime     logTime;
    cdr::String timeStr = logTime.getLocalTimeStr();

    // If no filename given, construct a default
    if (fname.empty())
        fname = getDefaultLogDir() + OSLogFile;

    // If non-standard port, append port number to log file
    // Avoids interspersing log messages from different processes in one file
    // Distinguishes standard from experimental outputs
    if (Glbl_CurrentServerPort != Glbl_DEFAULT_CDR_PORT) {
        char buf[10];
        sprintf(buf, "_%d", Glbl_CurrentServerPort);
        fname += buf;
    }

    // Try to log the message whether or not we got exclusive access
    // When writing to a file, we don't truncate the data - don't
    //   have the database string limits on output
    std::wofstream os(fname.c_str(), std::ios::app);
    if (os) {

        // Datetime, source, message
        os << timeStr << L"  ";

        if (msgSrc.length() > 0)
           os << msgSrc.c_str() << ": ";
        os << msg.c_str() << std::endl;
    }
    else {
        // Last resort is stderr
        std::wcerr << timeStr << L"  ";
        if (msgSrc.length() > 0)
            std::wcerr << msgSrc.c_str() << L": ";
        std::wcerr << msg.c_str() << std::endl;
    }

    // Release mutex
    if (stat == WAIT_OBJECT_0 || stat == WAIT_ABANDONED)
        ReleaseMutex (hMutex);
    if (hMutex)
        CloseHandle(hMutex);
}

/**
 * Records an event which occurred in a CDR client process.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::logClientEvent(Session&         session,
                                const dom::Node& node,
                                db::Connection&  conn)
{
    String eventDescription(true);
    Int    sessionId(true);

    // Extract the parameters.
    dom::Node child = node.getFirstChild();
    while (child != 0) {
        String name = child.getNodeName();
        if (name == L"EventDescription")
            eventDescription = dom::getTextContent(child);
        child = child.getNextSibling();
    }
    if (eventDescription.isNull())
        throw Exception(L"Missing required event description");

    // Get the user ID, if available.
    int sid = session.getId();
    if (sid)
        sessionId = sid;

    // Record the event.
    db::PreparedStatement s = conn.prepareStatement(
            "INSERT INTO client_log (event_time, event_desc, session) "
            "     VALUES (GETDATE(), ?, ?)");
    s.setString(1, eventDescription);
    s.setInt   (2, sessionId);
    s.executeUpdate();
    s.close();
    conn.commit();
    int eventId = conn.getLastIdent();

    // Report success.
    std::wostringstream os;
    os << L"<CdrLogClientEventResp><EventId>" << eventId
       << L"</EventId></CdrLogClientEventResp>";
    return os.str();
}

/**
 * Saves a client DLL trace log.
 *
 *  @param      session     contains information about the current user.
 *  @param      node        contains the XML for the command.
 *  @param      conn        reference to the connection object for the
 *                          CDR database.
 *  @return                 String object containing the XML for the
 *                          command response.
 *  @exception  cdr::Exception if a database or processing error occurs.
 */
cdr::String cdr::saveClientTraceLog(Session&         session,
                                    const dom::Node& node,
                                    db::Connection&  conn)
{
    String logData(true);
    String userName(true);
    String sessionId(true);

    // Extract the parameters.
    dom::Node child = node.getFirstChild();
    while (child != 0) {
        String name = child.getNodeName();
        if (name == L"LogData")
            logData = dom::getTextContent(child);
        child = child.getNextSibling();
    }
    if (logData.isNull())
        throw Exception(L"Missing required log data");

    // Extract the user name and session from the login line.
    size_t start = logData.find(L"logon(");
    if (start != String::npos) {
        start += 6;
        size_t comma = logData.find(L',', start);
        if (comma != String::npos) {
            userName = logData.substr(start, comma - start);
            start = comma + 2;
            size_t paren = logData.find(L')', start);
            if (paren != String::npos)
                sessionId = logData.substr(start, paren - start);
        }
    }

    // Save the log.
    db::PreparedStatement s = conn.prepareStatement(
            "INSERT INTO dll_trace_log "
            "(log_saved, cdr_user, session_id, log_data)"
            "VALUES (GETDATE(), ?, ?, ?)");
    s.setString(1, userName);
    s.setString(2, sessionId);
    s.setString(3, logData);
    s.executeUpdate();
    s.close();
    conn.commit();
    int logId = conn.getLastIdent();

    // Report success.
    std::wostringstream os;
    os << L"<CdrSaveClientTraceLogResp><LogId>" << logId
       << L"</LogId></CdrSaveClientTraceLogResp>";
    return os.str();
}
