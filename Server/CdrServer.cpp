/*
 * $Id: CdrServer.cpp,v 1.48 2008-12-07 21:14:22 bkline Exp $
 *
 * Server for ICIC Central Database Repository (CDR).
 *
 * BZIssue::4767
 */

// System headers.
#include <winsock.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <ctime>
#include <process.h>
#include <signal.h>
#include <sys/stat.h>

// Project headers.
#include "catchexp.h"
#include "CdrCommand.h"
#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrDbConnection.h"
#include "CdrException.h"
#include "CdrDbStatement.h"
#include "CdrString.h"
#include "HeapDebug.h"
#include "CdrFilter.h"
#include "CdrBlob.h"

// Local constants.
const short CDR_PORT = 2019;
const int   CDR_QUEUE_SIZE = 10;
const int   MAX_DIR_SIZE = 256;
const unsigned int MAX_REQUEST_LENGTH = 150000000;

// Local types.
struct ThreadArgs {
    int  fd;                // file descriptor for socket connection
    char clientAddress[65]; // IP address from which request was received
};

// Local functions.
static void             cleanup();
static void __cdecl     controlC(int);
static int              readRequest(int, std::string&, const cdr::String&);
static void             sendResponse(int, const cdr::String&);
static void             processCommands(const ThreadArgs*, const std::string&,
                                        cdr::db::Connection&,
                                        const cdr::String&);
static cdr::String      processCommand(cdr::Session&,
                                       const cdr::dom::Node&,
                                       cdr::db::Connection&,
                                       const cdr::String&);
static const std::string stripBlob(const std::string&,
                                   cdr::Session&);
static cdr::String      getElapsedTime(DWORD);
static void             sendErrorResponse(int, const cdr::String&,
                                          const cdr::String&);
static void __cdecl     dispatcher(void*);
static void             realDispatcher(const ThreadArgs* args);
static void __cdecl     sessionSweep(void*);
static int              handleNextClient(int sock);
static void             logTopLevelFailure(const cdr::String what,
                                           unsigned long code);
static bool             timeToShutdown = false;
static bool             logCommands = true;
static cdr::log::Log    log;

/* This is a C++ function, but needs a C interface */
static void excep_trans_func(unsigned int u, struct _EXCEPTION_POINTERS *pExp);

/**
 * Creates a socket and listens for connections on it.  Spawns
 * a new thread to handle each incoming connection.
 *
 * usage: CdrServer {port {log drive}}
 *    default port          = CDR_PORT = 2019
 *    default logging drive = "d"  See CdrLog.cpp
 *
 */
int main(int ac, char **av)
{
    WSAData             wsadata;
    int                 sock;
    struct sockaddr_in  addr;
    short               port = CDR_PORT;
    char                *defaultDir = NULL;

    // Find out whether we should suppress command logging.
    if (getenv("SUPPRESS_CDR_COMMAND_LOGGING"))
        logCommands = false;

    if (ac > 1) {
        port = atoi(av[1]);
        SET_HEAP_DEBUGGING(true);
        std::cout << "heap debugging\n" << std::endl;
    }

    // Should logs go somewhere else than the default?
    // Set default drive to NULL or indicated drive, adding ":"
    defaultDir = getenv("CDR_LOG_DIR");
    if (ac > 2)
        // Override both default and environment
        defaultDir = av[2];

    if (defaultDir) {
        // Test to be sure it's okay
        bool drvOK = true;
        struct _stat statBuf;
        int statResult;
        if (strlen(defaultDir) < MAX_DIR_SIZE - 1) {
            statResult = _stat(defaultDir, &statBuf);
            if (statResult != 0)
                drvOK = false;
        }
        else
            drvOK = false;

        // Failed
        if (!drvOK) {
            std::cerr << "Invalid logging directory \"" << defaultDir
                      << "\"" << std::endl;
            exit(1);
        }

        // Set it
        cdr::log::setDefaultLogDir(defaultDir);
    }

    // Announce start to log file
    cdr::log::WriteFile("CdrServer", "Starting server");

    // In case of catastrophe, don't hang up on console, but do abort.
    // My experiments so using a C++ wrapper work in the
    //   individual threads, but not at the top level of the
    //   program.  So I'm still using a plain old C style
    //   structured exception handler.
    // We replace the exception catcher here with one with a C++ wrapper
    //   around cdr::Exception in each started thread.
    if (!getenv ("NOCATCHCRASH")) {
        char catchLog[MAX_DIR_SIZE + 20];
        strcpy(catchLog, cdr::log::getDefaultLogDir().c_str());
        strcat(catchLog, "/CdrServer.crash");
        set_exception_catcher (catchLog, 1);
    }

    // Set new structured exception handler that throws cdr::Exception
    // Couldn't get this to work here.  It rethrew itself recursively
    //   in a nasty loop
    // _set_se_translator (excep_trans_func);

    if (WSAStartup(0x0101, &wsadata) != 0) {
        int err = WSAGetLastError();
        std::cerr << "WSAStartup: " << err << '\n';
        logTopLevelFailure(L"WSAStartup", (unsigned long)err);
        return EXIT_FAILURE;
    }
    atexit(cleanup);
    signal(SIGINT, controlC);
    std::cout << "initialized...\n";
    log.Write("CdrServer", "Starting");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        logTopLevelFailure(L"socket", (unsigned long)WSAGetLastError());
        return EXIT_FAILURE;
    }
    std::cout << "socket created...\n";
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
        unsigned long wsaError = (unsigned long)WSAGetLastError();
        perror("bind");
        logTopLevelFailure(L"bind", wsaError);
        return EXIT_FAILURE;
    }
    std::cout << "bound...\n";
    if (listen(sock, CDR_QUEUE_SIZE) == SOCKET_ERROR) {
        unsigned long wsaError = (unsigned long)WSAGetLastError();
        perror("listen");
        logTopLevelFailure(L"listen", wsaError);
        return EXIT_FAILURE;
    }
    std::cout << "listening...\n";

#ifndef SINGLE_THREAD_DEBUGGING
    std::cout << "sweeping for expired sessions...\n";
    if (_beginthread(sessionSweep, 0, (void*)0) == -1) {
        std::cerr << "_beginthread: " << GetLastError() << '\n';
        logTopLevelFailure(L"listen", (unsigned long)WSAGetLastError());
        return EXIT_FAILURE;
    }
#else
    std::cout << "running single-threaded for debugging...\n";
#endif

    // If profiling filters, we need to create
    //   a map of filter strings to IDs.
    // Will be used to find the ID of any filter used in the
    //   filter module, and then track timings by filter.
    // This only actually happens in debugging modes.  See
    //   CdrFilter.h / .cpp.
    // It's easier and more efficient to initialize this
    //   before multi-threading begins.
    cdr::buildFilterString2IdMap();

    while (!timeToShutdown) {
        try {
            MEM_START();
            int rc = handleNextClient(sock);
            if (rc != EXIT_SUCCESS)
                return rc;
            SHOW_HEAP_USED("Bottom of main processing loop");
            MEM_REPORT();
        }
        catch (const cdr::Exception& e) {
            logTopLevelFailure(L"main processing loop exception: "
                               + e.what(), 0L);
        }
        // We used to catch (...) here, but no longer
        // realDispatcher() now registers a routine for unhandled
        //   exceptions that rethrows them as cdr::Exceptions.
        // If error occurs before that it's almost certainly fatal
        //   andway and is caught by routine registered by
        //   set_exception_catcher above this.
        // This is not perfect.  I think there is still a small
        //   window within which an exception will be uncaught,
        //   but it's as close to perfect as I've gotten.
        // If we reinstate the code below, it can intercept exceptions
        //   headed for SH exception handler registered at program
        //   entry and prevent aborting on serious errors.
        // catch (...) {
        //     logTopLevelFailure(L"main processing loop (...) exception: ",
        //                        GetLastError());
        // }
    }
    return EXIT_SUCCESS;
}

/**
 * Meat of main processing loop, broken out to make stack cleanup
 * easier to track during heapdebuggin.
 */
int handleNextClient(int sock)
{
    struct sockaddr_in client_addr;
    int len = sizeof client_addr;
    memset(&client_addr, 0, sizeof client_addr);
    int fd = accept(sock, (struct sockaddr *)&client_addr, &len);
    if (timeToShutdown)
        return EXIT_SUCCESS;
    if (fd < 0) {
        perror("accept");
        logTopLevelFailure(L"accept", (unsigned long)WSAGetLastError());
        return EXIT_FAILURE;
    }
    ThreadArgs* threadArgs = new ThreadArgs();
    memset(threadArgs, 0, sizeof(ThreadArgs));
    threadArgs->fd = fd;
    strncpy(threadArgs->clientAddress, inet_ntoa(client_addr.sin_addr),
            sizeof(threadArgs->clientAddress) - 1);
    std::cout << "client IP address: " << threadArgs->clientAddress
              << std::endl;
#ifndef SINGLE_THREAD_DEBUGGING
    int tries = 5;
    while (_beginthread(dispatcher, 0, (void*)threadArgs) == -1) {
        DWORD err = errno; // GetLastError();
        std::cerr << "_beginthread: " << err << '\n';
        if (tries-- <= 0) {
            logTopLevelFailure(L"_beginthread", err);
            closesocket(fd);
            delete threadArgs;
            return EXIT_FAILURE;
        }
        Sleep(1000);
    }
#else
    // Use this version (and turn off the invocation of the session cleanup
    // thread below) when you want to debug without dealing with multiple
    // threads.
    realDispatcher(threadArgs);
#endif
    return EXIT_SUCCESS;
}

/**
 * Logs server shutdown and cleans up winsock resources.
 */
void cleanup()
{
    log.Write("CdrServer", "Stopping");
    WSACleanup();
}

/**
 * Catch Control-C.
 */
static void __cdecl controlC(int s)
{
    std::cerr << "Use ShutdownCdr command to shut down the CDR\n";
    signal(SIGINT, controlC);
}

/**
 * Implements thread processing to handle a new connection.  Capable
 * of handling multiple command sets for the connection.
 */
void realDispatcher(const ThreadArgs* threadArgs) {
    cdr::db::Connection conn =
        cdr::db::DriverManager::getConnection(cdr::db::url,
                                              cdr::db::uid,
                                              cdr::db::pwd);
    cdr::String now = conn.getDateTimeString();
    now[10] = L'T';
#ifndef _NDEBUG
    std::wcout << L"NOW=" << now << L"\n";
#endif

    // Create thread specific log pointer
    // Done early in thread creation so anything in the thread can
    //   log whatever it wants to.
    cdr::log::pThreadLog = new cdr::log::Log;
    cdr::String threadId = cdr::String::toString(GetCurrentThreadId());
    cdr::log::pThreadLog->Write(L"Thread Starting", threadId);

    // Set new structured exception handler that throws cdr::Exception
    _set_se_translator (excep_trans_func);

    std::string request;
    int nBytes;
    int response = 0;
    int fd = threadArgs->fd;
    try {
        while ((nBytes = readRequest(fd, request, now)) > 0) {
#ifndef _NDEBUG
            std::cout << "received request with " << nBytes << " bytes...\n";
#endif
            processCommands(threadArgs, request, conn, now);
        }
    }
    catch (const cdr::Exception e) {
        cdr::log::pThreadLog->Write (L"realDispatcher1", e.what());
    }
    catch (...) {
        cdr::log::pThreadLog->Write (L"realDispatcher2", L"Exception caught");
    }

    // Thread is about to go, done with thread specific log
    cdr::log::pThreadLog->Write(L"Thread Stopping", threadId);
    delete cdr::log::pThreadLog;
    closesocket(threadArgs->fd);
    delete threadArgs;
}

/**
 * Do the real work in a separate function to force Microsoft to clean
 * up the stack.
 */
void __cdecl dispatcher(void* threadArgs) {
    realDispatcher((const ThreadArgs*)threadArgs);
    _endthread();
}

size_t readBytes(int fd, size_t requested, char* buf)
{
    // Keep reading until we have all the bytes, an error occurs, or we give
    // up after getting no bytes, sleeping for awhile, and then trying again.
    size_t totalRead = 0;
    bool canSleep = true;
    while (totalRead < requested) {
        size_t bytesLeft = requested - totalRead;

        // This is needed because Microsoft's implementation of recv, instead
        // of reading what it can and returning the number of bytes it got,
        // decides that if it can't read it all at once it won't read anything
        // at all.  So "bytesLeft" is no longer really "bytes left."
        if (bytesLeft > 1024 * 1024)
            bytesLeft = 1024 * 1024;
        int nRead = recv(fd, buf + totalRead, bytesLeft, 0);
        if (nRead < 0)
            return 0;
        if (nRead == 0) {
            if (canSleep) {
                Sleep(500);
                canSleep = false;
            }
            else
                return totalRead;
        }
        else
            canSleep = true;
        totalRead += nRead;
    }
    return totalRead;
}

/**
 * Reads the 4-byte count of bytes in the command transmission,
 * then reads those bytes into the caller's string object.  Length
 * prefix must be sent in network byte order (most significant bytes
 * first).
 */
int readRequest(int fd, std::string& request, const cdr::String& when) {

    // Determine the length of the command buffer.
    unsigned long length;
    char lengthBytes[4];
    size_t totalRead = readBytes(fd, sizeof lengthBytes, lengthBytes);
    if (totalRead != sizeof lengthBytes)
        return 0;
    memcpy(&length, lengthBytes, sizeof length);
    length = ntohl(length);
    std::cout << "getting " << length << " bytes\n";

    // Avoid bogus requests from attackers (the only attacks we have had
    // so far are from NIH network administration software).
    if (length > MAX_REQUEST_LENGTH || length == 0) {
        cdr::log::pThreadLog->Write (L"readRequest",
                L"refusing " +
                cdr::String::toString(length) +
                L"-byte request");
        return 0;
    }

    // Check the first bytes for expected substring.
    char firstBytes[4097];
    memset(firstBytes, 0, sizeof firstBytes);
    size_t nFirstBytes = sizeof firstBytes - 1;
    if (nFirstBytes > length)
        nFirstBytes = length;
    totalRead = readBytes(fd, nFirstBytes, firstBytes);
    if (totalRead < nFirstBytes)
        return 0;
    if (!strstr(firstBytes, "<CdrCommandSet")) {
        cdr::log::pThreadLog->Write (L"readRequest",
                L"refusing bogus request string: " + cdr::String(firstBytes));
        return 0;
    }

    // See if we need to read any more.
    if (totalRead == length)
        request = std::string(firstBytes, totalRead);
    else {

        // Allocate a working buffer.
        char *buf = new char[length + 1];
        if (!buf) {
            char tmp[256];
            sprintf(tmp, "Failure allocating %lu bytes", length);
            sendErrorResponse(fd, tmp, when);
            return 0;
        }
        memset(buf, 0, length + 1);
        memcpy(buf, firstBytes, totalRead);
        totalRead += readBytes(fd, length - totalRead, buf + totalRead);
        if (totalRead == length)
            request = std::string(buf, totalRead);
        delete [] buf;
        if (totalRead != length) {
            char errMsg[256];
            sprintf(errMsg, "Expected %u bytes reading client request; "
                    "got %u bytes", length, totalRead);
            sendErrorResponse(fd, errMsg, when);
            return 0;
        }
    }
    return totalRead;
}

void logCommand(cdr::db::Connection& conn, const std::string& buf)
{
    try {
        cdr::db::PreparedStatement s = conn.prepareStatement(
                " INSERT INTO command_log (thread, received, command) "
                "      VALUES (?, GETDATE(), ?)                       ");
        s.setInt(1, cdr::log::pThreadLog->GetId());
        s.setString(2, buf);
        s.executeUpdate();
    }
    catch (...) {}
}

/**
 * Remove the blob from the client's buffer and store it separately
 * (since the DOM parser chokes when a large (or even medium-sized)
 * blob is present in the buffer).
 */
const std::string stripBlob(const std::string& buf, cdr::Session& session) {

    // Look for the opening tag for the blob; everything before will be kept.
    size_t openTagPos = buf.find("<CdrDocBlob");

    // If we don't have a blob the caller gets his own buffer back.
    if (openTagPos == std::string::npos)
        return buf;

    // Find the final character of the opening tag, skipping attributes.
    // We bump this up to mark the beginning of the blob bytes.
    size_t blobStart = buf.find('>', openTagPos);

    if (blobStart++ == std::string::npos)
        throw cdr::Exception("malformed CdrDocBlob tag");

    // Start with the possibility that we have an empty element tag.
    size_t tailStart = blobStart;
    size_t blobLen = 0;
    if (buf[blobStart - 2] == '/')
        session.setClientBlob(cdr::Blob(false));
    else {

        // The start of the closing tag marks the position just past the blob
        // bytes.
        size_t blobEnd = buf.find("</CdrDocBlob", blobStart);
        if (blobEnd == std::string::npos)
            throw cdr::Exception("missing CdrDocBlob end tag");

        // Look for the closing delimiter of the end tag separately,
        // because the spec allows optional whitespace between this
        // delimiter and the rest of the end tag.
        size_t endTagClosingDelimiter = buf.find('>', blobEnd);
        if (endTagClosingDelimiter == std::string::npos)
            throw cdr::Exception("malformed CdrDocBlob end tag");

        // Decode the base64 representation of the blob.
        cdr::Blob blob(buf.data() + blobStart, blobEnd - blobStart);

        // Put the blob somewhere where the save command can find it.
        session.setClientBlob(blob);

        // Adjust the values we need for reassembling the caller's buffer.
        tailStart = endTagClosingDelimiter + 1;
        blobLen = blob.size();
    }

    // Safety check to make sure other blobs don't fall on the floor.
    if (buf.find("<CdrDocBlob", tailStart) != std::string::npos)
        throw cdr::Exception("only one CdrDocBlob element allowed "
                             "in command set");

    // Reassemble the caller's buffer, replacing the blob with an
    // empty element containing only a size attribute.
    char blobTag[80];
    sprintf(blobTag, "<CdrDocBlob size='%u'/>", blobLen);
    return buf.substr(0, openTagPos) + blobTag + buf.substr(tailStart);
}
 
/**
 * Parses command set buffer, extracts each command and has it
 * processed.  Wraps all the responses in a buffer, which is returned
 * to the client.
 */
void processCommands(const ThreadArgs* threadArgs, const std::string& buf,
                     cdr::db::Connection& conn,
                     const cdr::String& when)
{
    int fd = threadArgs->fd;
    try {
        cdr::Session session;
        const std::string newBuf = stripBlob(buf, session);
        if (logCommands)
            logCommand(conn, newBuf);
        // Create a parser for the entire command set.
        // This does NOT use thread static memory to control all parse
        //   trees.
        // Do NOT use this technique lower down unless you really understand
        //   the memory model of the Xerces parser, the cdr::dom::Parser,
        //   your part of the application, and any parts of the application
        //   above you that could conceivably be affected.
        cdr::dom::Parser parser(false);

        // Parse the buffer
        parser.parse(newBuf);
        cdr::dom::Document document = parser.getDocument();
        if (document == 0) {
            sendErrorResponse(fd, "Failure parsing command buffer", when);
            return;
        }
        cdr::dom::Element docElement = document.getDocumentElement();
        if (docElement == 0) {
            sendErrorResponse(fd, "Failure parsing command buffer", when);
            return;
        }
        cdr::String elementName = docElement.getNodeName();
        if (elementName != L"CdrCommandSet") {
            sendErrorResponse(fd, "Top element must be CdrCommandSet", when);
            return;
        }
        cdr::String response = L"<CdrResponseSet Time='" + when + L"'>\n";
        cdr::dom::Node n = docElement.getFirstChild();
        while (n != 0) {
            if (n.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                elementName = n.getNodeName();
                if (elementName == L"SessionId") {
                    cdr::String sessionId = cdr::dom::getTextContent(n);
                    //std::wcerr << L"sessionId=" << sessionId << L"\n";
                    session.lookupSession(sessionId, conn);
                    session.setLastActivity(conn, threadArgs->clientAddress);
                }
                else if (elementName == L"CdrCommand") {
                    // Process one command
                    response += processCommand(session, n, conn, when);

                    // Delete any/all DOM parsers and trees allocated for
                    //   this thread during the processing of the command
                    // Note that the parse tree for the command set is
                    //   unaffected by this
                    cdr::dom::Parser::deleteAllThreadParsers();
                }
            }
            n = n.getNextSibling();
        }
        response += L"</CdrResponseSet>\n";
        sendResponse(fd, response);
    }
    catch (const cdr::Exception& cdrEx) {
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, cdrEx.what(), when);
    }
    catch (const cdr::dom::XMLException& ex) {
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, cdr::String(ex.getMessage()), when);
    }
    catch (...) {
        std::string badCmd = "**** Unexpected exception on cmd ***:\n" + buf;
        logCommand (conn, badCmd);
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, L"Unexpected exception caught", when);
    }
}

/**
 * Sends a response buffer to the client reporting a general error
 * which prevents processing of any of the commands in the command
 * buffer.
 */
void sendErrorResponse(int fd, const cdr::String& errMsg,
                       const cdr::String& when)
{
    cdr::String response = L"<CdrResponseSet Time='"
                         + when
                         + L"'>\n <Errors>\n  <Err>"
                         + errMsg
                         + L"</Err>\n </Errors>\n</CdrResponseSet>\n";
    sendResponse(fd, response);
}

/**
 * Finds the command handler responsible for the current command and
 * invokes it.
 */
cdr::String processCommand(cdr::Session& session,
                           const cdr::dom::Node& cmdNode,
                           cdr::db::Connection& conn,
                           const cdr::String& when)
{
    DWORD start = GetTickCount();
    const cdr::dom::Element& cmdElement = static_cast<const cdr::dom::Element&>
        (cmdNode);
    cdr::String cmdId = cmdElement.getAttribute(L"CmdId");
    cdr::String rspTag;
    if (cmdId.size() > 0)
        rspTag = L" <CdrResponse CmdId='" + cmdId + L"' Status='";
    else
        rspTag = L" <CdrResponse Status='";
    cdr::dom::Node specificCmd = cmdNode.getFirstChild();
    cdr::String elapsedTime;
    while (specificCmd != 0) {
        int type = specificCmd.getNodeType();
        if (type == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String cmdName = specificCmd.getNodeName();
#ifndef _NDEBUG
            std::wcout << L"processing command: " << cmdName << L"...\n";
#endif

            // Log info about the command
            cdr::String cmdText = L"Cmd: " + cmdName + L"  User: "
                                + session.getUserName();
            cdr::log::pThreadLog->Write (L"processCommand", cmdText);

            cdr::Command cdrCommand = cdr::lookupCommand(cmdName);
            if (!cdrCommand) {
                elapsedTime = getElapsedTime(start);
                return cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'>\n  <Errors>\n   "
                                          + L"<Err>Unknown command: "
                                          + cmdName
                                          + L"</Err>\n  </Errors>\n"
                                          + L" </CdrResponse>\n");
            }

            cdr::String cmdResponse;
            try {
                /*
                 * Only way you can get in the door without a valid session
                 * is with a logon command.
                 */
                if (cdrCommand != cdr::logon && !session.isOpen()) {
                    elapsedTime = getElapsedTime(start);
                    return cdr::String(rspTag + L"failure' Elapsed='"
                                              + elapsedTime
                                              + L"'>\n  <"
                                              + cmdName
                                              + L"Resp>\n"
                                              + L"   <Errors>\n    <Err>"
                                              + L"Missing session ID"
                                              + L"</Err>\n   </Errors>\n"
                                              + L"  </"
                                              + cmdName
                                              + L"Resp>\n </CdrResponse>\n");
                }

                // Optimistic assumption.
                session.setStatus(L"success");
                conn.setAutoCommit(true);
                cmdResponse = cdrCommand(session, specificCmd, conn);
            }
            catch (const cdr::Exception& e) {
                if (!conn.getAutoCommit())
                    conn.rollback();

                // Exception text already logged in exception constructor
                elapsedTime = getElapsedTime(start);
                return cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'>\n  <"
                                          + cmdName
                                          + L"Resp>\n"
                                          + L"   <Errors>\n    <Err>"
                                          + e.what()
                                          + L"</Err>\n   </Errors>\n"
                                          + L"  </"
                                          + cmdName
                                          + L"Resp>\n </CdrResponse>\n");
            }
            catch (cdr::dom::DOMException* de) {
                if (!conn.getAutoCommit())
                    conn.rollback();
                wchar_t tBuf[40];
                swprintf(tBuf, L"DOM Exception Code %d: ", de->code);
                elapsedTime = getElapsedTime(start);
                cdr::String result =
                       cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'><"
                                          + cmdName
                                          + L"Resp><Errors><Err>"
                                          + cdr::String(tBuf)
                                          + cdr::String(de->msg)
                                          + L"</Err></Errors></"
                                          + cmdName
                                          + L"Resp></CdrResponse>");
                delete de;
                return result;
            }
            catch (const cdr::dom::SAXParseException& spe) {
                if (!conn.getAutoCommit())
                    conn.rollback();
                wchar_t tmpBuf[80];
                swprintf(tmpBuf, L" at line %d, Column %d",
                         spe.getLineNumber(), spe.getColumnNumber());
                cdr::String locString = tmpBuf;
                elapsedTime = getElapsedTime(start);
                return cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'><"
                                          + cmdName
                                          + L"Resp><Errors>"
                                          + L"<Err>SAX Parse Exception: "
                                          + spe.getMessage()
                                          + locString
                                          + L"</Err></Errors></"
                                          + cmdName
                                          + L"Resp></CdrResponse>");
            }
            catch (const cdr::dom::SAXException& se) {
                if (!conn.getAutoCommit())
                    conn.rollback();
                elapsedTime = getElapsedTime(start);
                return cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'><"
                                          + cmdName
                                          + L"Resp><Errors><Err>SAX Exception: "
                                          + se.getMessage()
                                          + L"</Err></Errors></"
                                          + cmdName
                                          + L"Resp></CdrResponse>");
            }
            catch (...) {
                if (!conn.getAutoCommit())
                    conn.rollback();
                elapsedTime = getElapsedTime(start);
                return cdr::String(rspTag + L"failure' Elapsed='"
                                          + elapsedTime
                                          + L"'><"
                                          + cmdName
                                          + L"Resp><Errors><Err>Unexpected "
                                            L"exception caught."
                                            L"</Err></Errors></"
                                          + cmdName
                                          + L"Resp></CdrResponse>");
            }
            elapsedTime = getElapsedTime(start);
            cdr::String response = rspTag + session.getStatus()
                                          + L"' Elapsed='"
                                          + elapsedTime
                                          + L"'>\n"
                                          + cmdResponse
                                          + L" </CdrResponse>\n";
            if (cmdName == L"CdrShutdown")
                timeToShutdown = true;

            cdr::log::pThreadLog->Write (L"processCommand result",
                                         session.getStatus());

            return response;
        }
        specificCmd = specificCmd.getNextSibling();
    }
    return cdr::String(rspTag + L"failure' Elapsed='"
                              + elapsedTime
                              + L"'>\n  <Errors>\n   "
                              + L"<Err>Missing specific command element"
                              + L"</Err>\n  </Errors>\n"
                              + L" </CdrResponse>\n");
}

/**
 * Sends the client the count of bytes in the response buffer (using
 * a network-order 4-byte integer), followed by the contents of the
 * buffer.
 */
void sendResponse(int fd, const cdr::String& response)
{
    std::string s = response.toUtf8();
    unsigned long bytes  = s.size();
    unsigned long length = htonl(bytes);
    send(fd, (char *)&length, sizeof length, 0);
    send(fd, s.c_str(), bytes, 0);
}

/**
 * Cleans up stale sessions (those which have been inactive for 24 hours or
 * longer).
 */
void __cdecl sessionSweep(void* arg) {

    const char* query = "UPDATE session"
                        "   SET ended = GETDATE()"
                        " WHERE ended IS NULL"
                        "   AND name <> 'guest'"
                        "   AND DATEDIFF(hour, last_act, GETDATE()) > 24";
    int counter = 0;
    cdr::log::Log log;
#ifdef LOGSWEEP
    FILE *fp = fopen("sweep.log", "a");
    if (fp) {
        time_t now = time(0);
        fprintf(fp, "sweep starting at %s", ctime(&now));
        fclose(fp);
    }
#endif
    while (!timeToShutdown) {
        try {
            if (counter++ % 60 == 0) {
                cdr::db::Connection conn =
                    cdr::db::DriverManager::getConnection(cdr::db::url,
                                                          cdr::db::uid,
                                                          cdr::db::pwd);
                cdr::db::Statement s = conn.createStatement();
                int rows = s.executeUpdate(query);
#ifdef LOGSWEEP
                fp = fopen("sweep.log", "a");
                if (fp) {
                    time_t now = time(0);
                    fprintf(fp, "sweep found %d rows at %s", rows, ctime(&now));
                    fclose(fp);
                }
#endif
                if (rows > 0) {
                    cdr::String now = conn.getDateTimeString();
                    now[10] = L'T';
                    std::wcerr << L"*** CLEARED " << rows
                               << L" INACTIVE SESSIONS AT " << now << L"\n";
                }
            }
            Sleep(5000);
        }
        catch (cdr::Exception& e) {
            log.Write("sessionSweep", e.what());
        }
        catch (...) {
            log.Write("sessionSweep", "unknown exception");
        }
    }
    _endthread();
}

/**
 * Returns a string containing the number of seconds elapsed since the start.
 */
cdr::String getElapsedTime(DWORD start)
{
    DWORD now = GetTickCount();
    DWORD millis = now - start; // OK if it wraps around.
    wchar_t buf[80];
    swprintf(buf, L"%lu.%03lu", millis / 1000L, millis % 1000L);
    return buf;
}

/**
 * Logs an error which occurs outside of a specific thread.
 */
void logTopLevelFailure(const cdr::String what, unsigned long code)
{
    wchar_t buf[80];
    swprintf(buf, L"%lu", code);
    cdr::log::WriteFile(what, buf);
}

/**
 * If we have told Windows to call this function via
 *      _set_se_translator (excep_trans_func);
 * then this will be called when an OS level "structured exception" occurs.
 *
 * It throws a C++ cdr::Exception, catchable in C++, with
 * the info the OS gives us.  Without this, catch (...) will
 * still catch the exception, but won't give us any info about it.
 * With this, "catch (cdr::Exception e) {...}" will get info.
 *
 * _set_se_translator() _MUST_ be called in each thread, individually.
 *
 * Note that exception code u is duplicated in pExp, and therefore
 * not needed by analyzeException().
 */
static void excep_trans_func (unsigned int u, _EXCEPTION_POINTERS* pExp) {

    char *msg = analyzeException (pExp);
    cdr::String s = cdr::String (msg);
    std::cout << "In excep_trans_func: " << msg << std::endl;
    throw cdr::Exception (s);
}
