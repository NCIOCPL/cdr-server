/*
 * $Id: CdrServer.cpp,v 1.4 2000-04-22 09:33:56 bkline Exp $
 *
 * Server for ICIC Central Database Repository (CDR).
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2000/04/16 21:55:48  bkline
 * Fixed code for getting sessionId.  Added calls to lookupSession() and
 * setLastActivity().  Added code to set Status attribute in <CdrResponse>
 * element.
 *
 * Revision 1.2  2000/04/15 14:12:07  bkline
 * Added conditional Sleep() for 0-byte recv.  Replaced cdr::DbConnection*
 * with cdr::db::Connection&.  Catch added for exception thrown by
 * command implementation.
 *
 * Revision 1.1  2000/04/13 17:08:44  bkline
 * Initial revision
 *
 */

// System headers.
#include <winsock.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>

// Project headers.
#include "CdrCommand.h"
#include "CdrSession.h"
#include "CdrDom.h"
#include "CdrDbConnection.h"
#include "CdrException.h"

// Local constants.
const short CDR_PORT = 2019;
const int   CDR_QUEUE_SIZE = 10;

// Local functions.
static void             cleanup() { WSACleanup(); }
static int              readRequest(int, std::string&);
static void             sendResponse(int, const cdr::String&);
static void             processCommands(int, const std::string&);
static cdr::String      processCommand(cdr::Session&, 
                                       const cdr::dom::Node&,
                                       cdr::db::Connection&);
static void             sendErrorResponse(int, const cdr::String&);
static DWORD __stdcall  dispatcher(LPVOID);

/**
 * Creates a socket and listens for connections on it.  Spawns
 * a new thread to handle each incoming connection.
 */
main(int ac, char **av)
{
    WSAData             wsadata;
    int                 sock;
    struct sockaddr_in  addr;

    if (WSAStartup(0x0101, &wsadata) != 0) {
        std::cerr << "WSAStartup: " << WSAGetLastError() << '\n';
        return EXIT_FAILURE;
    }
    atexit(cleanup);
    std::cout << "initialized...\n";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    std::cout << "socket created...\n";
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CDR_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&addr, sizeof addr) == SOCKET_ERROR) {
        perror("bind");
        return EXIT_FAILURE;
    }
    std::cout << "bound...\n";
    if (listen(sock, CDR_QUEUE_SIZE) == SOCKET_ERROR) {
        perror("listen");
        return EXIT_FAILURE;
    }
    std::cout << "listening...\n";

    for (;;) {
        struct sockaddr_in client_addr;
        int len = sizeof client_addr;
        memset(&client_addr, 0, sizeof client_addr);
        int fd = accept(sock, (struct sockaddr *)&client_addr, &len);
        if (fd < 0) {
            perror("accept");
            return EXIT_FAILURE;
        }
        if (!CreateThread(0, 0, dispatcher, (LPVOID)fd, 0, 0)) {
            std::cerr << "CreateThread: " << GetLastError() << '\n';
            return EXIT_FAILURE;
        }
    }
}

/**
 * Implements thread processing to handle a new connection.  Capable
 * of handling multiple command sets for the connection.
 */
DWORD __stdcall dispatcher(LPVOID arg) {
    int fd = (int)arg;
    std::string request;
    int nBytes;
    int response = 0;
    while ((nBytes = readRequest(fd, request)) > 0) {
        std::cout << "received request with " << nBytes << " bytes...\n";
        processCommands(fd, request);
    }
    return EXIT_SUCCESS;
}

/**
 * Reads the 4-byte count of bytes in the command transmission,
 * then reads those bytes into the caller's string object.  Length
 * prefix must be sent in network byte order (most significant bytes
 * first).
 */
int readRequest(int fd, std::string& request) {

    // Determine the length of the command buffer.
    unsigned long length;
    char lengthBytes[4];
    size_t totalRead = 0;
    bool canSleep = true;
    while (totalRead < sizeof lengthBytes) {
        int n = recv(fd, lengthBytes + totalRead, 
                     sizeof lengthBytes - totalRead, 0);
        if (n < 0)
            return -1;
        if (n == 0) {
            if (canSleep) {
                Sleep(500);
                canSleep = false;
            }
            else
                return 0;
        }
        else
            canSleep = true;
        totalRead += n;
    }
    memcpy(&length, lengthBytes, sizeof length);
    length = ntohl(length);

    // Allocate a working buffer.
    char *buf = new char[length + 1];
    memset(buf, 0, length + 1);

    // Keep reading until we have all the bytes.
    totalRead = 0;
    canSleep = true;
    while (totalRead < length) {
        size_t bytesLeft = length - totalRead;
        int nRead = recv(fd, buf + totalRead, bytesLeft, 0);
        if (nRead < 0) {
            sendErrorResponse(fd, "Failure reading command buffer");
            return 0;
        }
        if (nRead == 0) {
            if (canSleep) {
                Sleep(500);
                canSleep = false;
            }
            else  {
                sendErrorResponse(fd, "Failure reading command buffer");
                return 0;
            }
        }
        else
            canSleep = true;
        totalRead += nRead;
    }
    request = std::string(buf, totalRead);
    delete [] buf;
    return totalRead;
}

/**
 * Parses command set buffer, extracts each command and has it
 * processed.  Wraps all the responses in a buffer, which is returned
 * to the client.
 */
void processCommands(int fd, const std::string& buf)
{
    cdr::db::Connection conn;
    try {
        cdr::dom::Parser parser;
        parser.parse(buf);
        cdr::dom::Document document = parser.getDocument();
        if (document == 0) {
            sendErrorResponse(fd, "Failure parsing command buffer");
            return;
        }
        cdr::dom::Element docElement = document.getDocumentElement();
        if (docElement == 0) {
            sendErrorResponse(fd, "Failure parsing command buffer");
            return;
        }
        cdr::String elementName = docElement.getNodeName();
        if (elementName != L"CdrCommandSet") {
            sendErrorResponse(fd, "Top element must be CdrCommandSet");
            return;
        }
        cdr::Session session;
        cdr::String response = L"<CdrResponseSet>\n";
        cdr::dom::Node n = docElement.getFirstChild();
        while (n != 0) {
            if (n.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                elementName = n.getNodeName();
                if (elementName == L"SessionId") {
                    cdr::String sessionId = cdr::dom::getTextContent(n);
                    std::wcerr << L"sessionId=" << sessionId << L"\n";
                    session.lookupSession(sessionId, conn);
                    session.setLastActivity(conn);
                }
                else if (elementName == L"CdrCommand")
                    response += processCommand(session, n, conn);
            }
            n = n.getNextSibling();
        }
        response += L"</CdrResponseSet>\n";
        sendResponse(fd, response);
    }
    catch (const cdr::Exception& cdrEx) {
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, cdrEx.getString());
    }
    catch (const cdr::dom::DOMException& ex) {
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, cdr::String(ex.getMessage()));
    }
    catch (...) {
        if (!conn.getAutoCommit())
            conn.rollback();
        sendErrorResponse(fd, L"Unexpected exception caught");
    }
}

/**
 * Sends a response buffer to the client reporting a general error
 * which prevents processing of any of the commands in the command
 * buffer.
 */
void sendErrorResponse(int fd, const cdr::String& errMsg)
{
    cdr::String response = L"<CdrResponseSet>\n <Errors>\n  <Error>"
                         + errMsg
                         + L"</Error>\n </Errors>\n</CdrResponseSet>\n";
    sendResponse(fd, response);
}

/**
 * Finds the command handler responsible for the current command and
 * invokes it.
 */
cdr::String processCommand(cdr::Session& session, 
                           const cdr::dom::Node& cmdNode,
                           cdr::db::Connection& conn)
{
    const cdr::dom::Element& cmdElement = static_cast<const cdr::dom::Element&>
        (cmdNode);
    cdr::String cmdId = cmdElement.getAttribute(L"CmdId");
    cdr::String rspTag;
    if (cmdId.size() > 0)
        rspTag = L" <CdrResponse CmdId='" + cmdId + L"' Status='";
    else
        rspTag = L" <CdrResponse Status='";
    cdr::dom::Node specificCmd = cmdNode.getFirstChild();
    while (specificCmd != 0) {
        int type = specificCmd.getNodeType();
        if (type == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String cmdName = specificCmd.getNodeName();
            //std::wcerr << L"Received command: " << cmdName << L"\n";
            cdr::Command cdrCommand = cdr::lookupCommand(cmdName);
            if (!cdrCommand)
                return cdr::String(rspTag + L"failure'>\n  <Errors>\n   "
                                          + L"<Error>Unknown command: "
                                          + cmdName
                                          + L"</Error>\n  </Errors>\n"
                                          + L" </CdrResponse>\n");
            cdr::String cmdResponse;
            try {
                /*
                 * Only way you can get in the door without a valid session
                 * is with a logon command.
                 */
                if (cdrCommand != cdr::logon && session.name.size() < 1)
                    return cdr::String(rspTag + L"failure'>\n  <"
                                              + cmdName
                                              + L"Resp>\n"
                                              + L"   <Errors>\n    <Error>"
                                              + L"Missing session ID"
                                              + L"</Error>\n   </Errors>\n"
                                              + L"  </"
                                              + cmdName
                                              + L"Resp>\n </CdrResponse>\n");

                // Optimistic assumption.
                session.lastStatus = L"success";
                conn.setAutoCommit(true);
                cmdResponse = cdrCommand(session, specificCmd, conn);
            }
            catch (cdr::Exception e) {
                if (!conn.getAutoCommit())
                    conn.rollback();
                return cdr::String(rspTag + L"failure'>\n  <"
                                          + cmdName
                                          + L"Resp>\n"
                                          + L"   <Errors>\n    <Error>"
                                          + e.getString()
                                          + L"</Error>\n   </Errors>\n"
                                          + L"  </"
                                          + cmdName
                                          + L"Resp>\n </CdrResponse>\n");
            }
            cdr::String response = rspTag + session.lastStatus
                                          + L"'>\n"
                                          + cmdResponse 
                                          + L" </CdrResponse>\n";
            return response;
        }
        specificCmd = specificCmd.getNextSibling();
    }
    return cdr::String(rspTag + L"failure'>\n  <Errors>\n   "
                              + L"<Error>Missing specific command element"
                              + L"</Error\n  </Errors>\n"
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
