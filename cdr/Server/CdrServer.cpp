/*
 * $Id: CdrServer.cpp,v 1.1 2000-04-13 17:08:44 bkline Exp $
 *
 * Server for ICIC Central Database Repository (CDR).
 *
 * $Log: not supported by cvs2svn $
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
static size_t           readRequest(int, std::string&);
static void             sendResponse(int, const cdr::String&);
static void             processCommands(int, const std::string&);
static cdr::String      processCommand(cdr::Session&, 
                                       const cdr::dom::Node&,
                                       cdr::DbConnection *);
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
        std::cout << "accepted connection...\n";
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
size_t readRequest(int fd, std::string& request) {

    // Determine the length of the command buffer.
    unsigned long length;
    char lengthBytes[4];
    size_t totalRead = 0;
    while (totalRead < sizeof lengthBytes) {
        int n = recv(fd, lengthBytes + totalRead, 
                     sizeof lengthBytes - totalRead, 0);
        if (n < 0) {
            return -1;
        }
        totalRead += n;
    }
    memcpy(&length, lengthBytes, sizeof length);
    length = ntohl(length);

    // Allocate a working buffer.
    char *buf = new char[length + 1];
    memset(buf, 0, length + 1);

    // Keep reading until we have all the bytes.
    totalRead = 0;
    while (totalRead < length) {
        size_t bytesLeft = length - totalRead;
        size_t nRead = recv(fd, buf + totalRead, length, 0);
        if (nRead < 0) {
            sendErrorResponse(fd, "Failure reading command buffer");
            return 0;
        }
        totalRead += nRead;
    }
    request = buf;
    delete [] buf;
    return request.size();
}

/**
 * Parses command set buffer, extracts each command and has it
 * processed.  Wraps all the responses in a buffer, which is returned
 * to the client.
 */
void processCommands(int fd, const std::string& buf)
{
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
        cdr::DbConnection *conn = 0;
        cdr::String response = L"<CdrResponseSet>\n";
        cdr::dom::Node n = docElement.getFirstChild();
        while (n != 0) {
            if (n.getNodeType() == cdr::dom::Node::ELEMENT_NODE) {
                elementName = n.getNodeName();
                if (elementName == L"SessionId")
                    session.id = n.getNodeValue();
                else if (elementName == L"CdrCommand")
                    response += processCommand(session, n, conn);
            }
            n = n.getNextSibling();
        }
        response += L"</CdrResponseSet>\n";
        sendResponse(fd, response);
    }
    catch (const cdr::Exception& cdrEx) {
        sendErrorResponse(fd, cdrEx.getString());
    }
    catch (const cdr::dom::DOMException& ex) {
        sendErrorResponse(fd, cdr::String(ex.getMessage()));
    }
    catch (...) {
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
                           cdr::DbConnection* conn)
{
    const cdr::dom::Element& cmdElement = static_cast<const cdr::dom::Element&>
        (cmdNode);
    cdr::String cmdId = cmdElement.getAttribute(L"CmdId");
    cdr::String rspTag;
    if (cmdId.size() > 0)
        rspTag = L" <CdrResponse CmdId='" + cmdId + L">\n";
    else
        rspTag = L" <CdrResponse>\n";
    cdr::dom::Node specificCmd = cmdNode.getFirstChild();
    while (specificCmd != 0) {
        int type = specificCmd.getNodeType();
        if (type == cdr::dom::Node::ELEMENT_NODE) {
            cdr::String cmdName = specificCmd.getNodeName();
            cdr::Command cdrCommand = cdr::lookupCommand(cmdName);
            if (!cdrCommand)
                return cdr::String(rspTag + L"  <Errors>\n   "
                                          + L"<Error>Unknown command: "
                                          + cmdName
                                          + L"</Error>\n  </Errors>\n"
                                          + L" </CdrResponse>\n");
            cdr::String cmdResponse = cdrCommand(session, specificCmd, conn);
            cdr::String response = rspTag + cmdResponse + L" </CdrResponse>\n";
            return response;
        }
        specificCmd = specificCmd.getNextSibling();
    }
    return cdr::String(rspTag + L"  <Errors>\n   "
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
