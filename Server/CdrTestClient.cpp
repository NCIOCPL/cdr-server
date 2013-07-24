/*
 * $Id$
 *
 * Test client (C++ version) for sending commands to CDR server.
 *
 * Usage:
 *  CdrTestClient [host [port [command-file]]]
 *
 * Example:
 *  CdrTestClient CdrCommandSamples.xml mmdb2
 *
 * Default for host is "localhost"; default for port is 2019.
 * If no command file is named, a test command is used.
 * Command buffer must be valid XML, conforming to the DTD
 * CdrCommandSet.dtd, and the top-level element must be <CdrCommandSet>.
 * The encoding for the XML must be UTF-8.
 */

// System headers.
#include <winsock.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include "catchexp.h"

// Local constants.
const short CDR_PORT = 2019;
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#define setmode _setmode
#endif

// Local functions.
static std::string  readFile(std::istream&);
static void         cleanup() { WSACleanup(); }
static void         showUsage(int, char**);
static const std::string currentDateTime();

int main(int ac, char** av)
{
    WSAData             wsadata;
    int                 sock;
    long                address;
    struct sockaddr_in  addr;
    struct hostent*     ph;
    std::string         requests;

    // Show usage if requested.
    showUsage(ac, av);

    // Extract command-line arguments.
    char* host = ac > 1 ? av[1] : "localhost";
    int   port = ac > 2 ? atoi(av[2]) : CDR_PORT;
    char* cmds = ac > 3 ? av[3] : NULL;

    // In case of catastrophe, don't hang up on console
    // But do abort
    if (!getenv ("NOCATCHCRASH"))
        set_exception_catcher ("CdrTestClient.crash", 1);

    // Load the requests (or use a canned test).
    if (cmds) {
        std::ifstream is(cmds, std::ios::binary | std::ios::in);
        if (!is) {
            std::cerr << av[1] << ": " << strerror(errno) << '\n';
            return EXIT_FAILURE;
        }
        requests = readFile(is);
    }
    else {
        requests = "<CdrCommandSet><SessionId>guest</SessionId>";
        requests += "<CdrCommand><CdrGetDoc><DocId>CDR0000043753</DocId>";
        requests += "</CdrGetDoc></CdrCommand></CdrCommandSet>";
    }

    // Initialize socket I/O.
    if (WSAStartup(0x0101, &wsadata) != 0) {
        std::cerr << "WSAStartup: " << WSAGetLastError() << '\n';
        return EXIT_FAILURE;
    }
    atexit(cleanup);

    // Connect a socket to the server.
    if (isdigit(host[0])) {
        if ((address = inet_addr(host)) == -1) {
            std::cerr << "invalid host name " << host << '\n';
            return EXIT_FAILURE;
        }
        addr.sin_addr.s_addr = address;
        addr.sin_family = AF_INET;
    }
    else {
        ph = gethostbyname(host);
        if (!ph) {
            std::cerr << "gethostbyname error: " << GetLastError() << "\n";
            return EXIT_FAILURE;
        }
        addr.sin_family = ph->h_addrtype;
        memcpy(&addr.sin_addr, ph->h_addr, ph->h_length);
    }
    addr.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << currentDateTime() << "\n";
        std::cerr << "socket error: " << GetLastError() << "\n";
        return EXIT_FAILURE;
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof addr) < 0) {
        std::cerr << currentDateTime() << "\n";
        std::cerr << "connect error: " << GetLastError() << "\n";
        return EXIT_FAILURE;
    }

    // Send the commands to the server.
    unsigned long bytes = requests.size();
    unsigned long length = htonl(bytes);
    if (send(sock, (char*)&length, sizeof length, 0) < 0) {
        std::cerr << currentDateTime() << "\n";
        std::cerr << "send1 error: " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }
    if (send(sock, requests.c_str(), bytes, 0) < 0) {
        std::cerr << currentDateTime() << "\n";
        std::cerr << "send2 error: " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }

    // Determine the number of bytes in the server's reply.
    size_t totalRead = 0;
    char lengthBytes[4];
    while (totalRead < sizeof lengthBytes) {
        int leftToRead = sizeof lengthBytes - totalRead;
        int bytesRead = recv(sock, lengthBytes + totalRead, leftToRead, 0);
        if (bytesRead < 0) {
            std::cerr << currentDateTime() << "\n";
            std::cerr << "recv1 error: " << GetLastError() << '\n';
            return EXIT_FAILURE;
        }
        totalRead += bytesRead;
    }
    memcpy(&length, lengthBytes, sizeof length);
    length = ntohl(length);

    // Catch the server's response.
    char* response = new char[length + 1];
    memset(response, 0, length + 1);
    totalRead = 0;
    while (totalRead < length) {
        int leftToRead = length - totalRead;
        int bytesRead = recv(sock, response + totalRead, leftToRead, 0);
        if (bytesRead < 0) {
            std::cerr << currentDateTime() << "\n";
            std::cerr << "recv2 error: " << GetLastError() << '\n';
            return EXIT_FAILURE;
        }
        totalRead += bytesRead;
    }
    setmode(fileno(stdout), O_BINARY);
    std::cout << "<!-- Server response: -->\n" << response << '\n';
    return EXIT_SUCCESS;
}

std::string readFile(std::istream& is)
{
    int nBytes = 0;
    char* bytes = new char[nBytes];
    char buf[4096];
    while (is) {
        is.read(buf, sizeof buf);
        int n = (int)is.gcount();
        if (n > 0) {
            char* tmp = new char[nBytes + n];
            if (nBytes > 0)
                memcpy(tmp, bytes, nBytes);
            memcpy(tmp + nBytes, buf, n);
            delete [] bytes;
            bytes = tmp;
            nBytes += n;
        }
    }
    return std::string(bytes, nBytes);
}

/**
 * Get a datetime stamp for errors.
 *
 * Returns a non-modifiable string
 */
static const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return std::string(buf);
}

static void showUsage(int ac, char** av) {

    if (ac < 2)
        return;
    if (strcmp(av[1], "-h") && strcmp(av[1], "-H") && strcmp(av[1], "/?"))
        return;
    std::cerr << "Submit commands to a CdrServer\n"
              << "usage: " << av[0]
              << " [host [port [command-file]]]"
              << std::endl;
    exit(1);
}
