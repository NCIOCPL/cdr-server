/*
 * $Id: CdrLog.cpp,v 1.14 2008-12-19 17:03:46 bkline Exp $
 *
 * Implementation of writing info to the log table in the database.
 * If that can't be done, takes an alternative action to write to file.
 *
 *                                          Alan Meyer  June, 2000
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.13  2008/10/15 02:36:19  ameyer
 * Changed handling of default logging directory and file.
 *
 * Revision 1.12  2006/10/04 03:45:20  ameyer
 * Revised mutex security attribute management.
 *
 * Revision 1.11  2006/01/25 01:47:47  ameyer
 * Fixed bug introduced in last version.  I was checking whether we were
 * inside the logger in one place too many - guaranteeing that we were.
 * Now fixed.
 *
 * Revision 1.10  2005/07/28 21:11:03  ameyer
 * Removed erroneous CVS comment on previous version.
 * Real change was to protect logging from inadvertent recursion - attempt
 * to log an exception fails, calling the logging function to try again.
 *
 * Revision 1.9  2005/07/28 20:39:46  ameyer
 *
 *
 * Revision 1.8  2002/03/28 22:19:53  ameyer
 * Removed unreferenced variable declaration.
 *
 * Revision 1.7  2002/03/06 21:57:16  bkline
 * Catching reference to cdr::Exception instead of object.
 *
 * Revision 1.6  2002/03/04 21:22:57  bkline
 * Added missing call to localtime().
 *
 * Revision 1.5  2002/03/04 20:51:18  bkline
 * Added workaround for memory leak caused by bug in MS CRT.
 *
 * Revision 1.4  2002/02/28 01:02:53  bkline
 * Added code to close mutex handle.
 *
 * Revision 1.3  2001/10/29 15:44:12  bkline
 * Replaced fopen/fwprintf/fclose with wofstream I/O.
 *
 * Revision 1.2  2000/10/05 17:23:21  ameyer
 * Replaced AlternateWrite with WriteFile, an externally callable
 * method that allows the caller to write to an OS file instead of
 * to the database.
 *
 * Revision 1.1  2000/06/15 22:32:24  ameyer
 * Initial revision
 *
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
static bool __declspec(thread) inLogging = false;

/**
 * Directory for logging - for OSLogFile.
 * Also may be used elsewhere.
 * Changed from constant to variable to allow overrides
 * in invocation of the CdrServer.
 */
static std::string CdrLogDir = "d:/cdr/log";

/**
 * Name of OS based logfile.
 * This file is used if, and only if, the logger is unable to
 *   write to the debug_log table in the database.
 * Otherwise all log messages go to table debug_log.
 */
static std::string OSLogFile = "/CdrLogErrs";

// Class static variables
int    cdr::log::Log::s_LogId     = 0;
HANDLE cdr::log::Log::s_hLogMutex = 0;

/**
 * Constructor.
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
    // Create a database connection and write data
    try {
        cdr::db::Connection dbConn =
            cdr::db::DriverManager::getConnection (cdr::db::url,
                                                   cdr::db::uid, cdr::db::pwd);
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
    if (inLogging)
        return;
    inLogging = true;

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

#define WE_GET_IT_WORKING 1
#ifdef WE_GET_IT_WORKING
    // Store it
    try {
        insert.executeQuery ();
    }
    catch (cdr::Exception&) {
        // Couldn't write.  Use exception reporting instead
        // Don't re-throw exception.  Would probably cause a loop.
        WriteFile (MsgSrc, Msg);
    }
#else
    WriteFile (MsgSrc, Msg);
#endif

    inLogging = false;
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


    // Set datetime
    time_t ltime;
    time (&ltime);
    /*
     * Can't use this, because of a bug in Microsoft's multi-threaded
     * runtime library.
     *
     * wchar_t *wct = _wctime (&ltime);
     */
    const char* ascTime = asctime(localtime(&ltime));
    cdr::String timeStr(ascTime);


    // If no filename given, construct a default
    if (fname.empty())
        fname = CdrLogDir + OSLogFile;

    // Try to log the message whether or not we got exclusive access
    // When writing to a file, we don't truncate the data - don't
    //   have the database string limits on output
    std::wofstream os(fname.c_str(), std::ios::app);
    if (os) {

        // Datetime, source, message
        os << L"---" << timeStr.c_str()
           << L">>>" << msgSrc.c_str()
           << L":\n" << msg.c_str() << std::endl;
    }
    else {
        // Last resort is stderr
        std::wcerr << L"---" << timeStr.c_str()
                   << L">>>" << msgSrc.c_str()
                   << L":\n" << msg.c_str() << std::endl;
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
