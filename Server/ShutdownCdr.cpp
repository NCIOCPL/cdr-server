/*
 * $Id$
 *
 * Program for submitting command to shut down the CDR server.
 *
 * Usage:
 *  ShutdownCdr uid pwd [host [port]]
 *
 * Example:
 *  ShutdownCdr rmk pw localhost 2020
 *
 * Default for host is "localhost"; default for port is 2019.
 */

// System headers.
#include <winsock.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

// Local constants.
const short CDR_PORT = 2019;

// Local functions.
static void         cleanup() { WSACleanup(); }

int main(int ac, char **av) {

    // Check the command line.
    if (ac < 3) {
        std::cerr << "User ID and password are required\n";
        return EXIT_FAILURE;
    }
    std::string     uid     = av[1];
    std::string     pwd     = av[2];
    char*           host    = ac > 3 ? av[3] : "localhost";
    int             port    = ac > 4 ? atoi(av[4]) : CDR_PORT;

    // Initialize socket I/O.
    WSAData wsadata;
    if (WSAStartup(0x0101, &wsadata) != 0) {
        std::cerr << "WSAStartup: " << WSAGetLastError() << '\n';
        return EXIT_FAILURE;
    }
    atexit(cleanup);

    // Connect a socket to the server.
    struct sockaddr_in  addr;
    struct hostent *    ph;
    int                 sock;
    long                address;
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
            perror("gethostbyname");
            return EXIT_FAILURE;
        }
        addr.sin_family = ph->h_addrtype;
        memcpy(&addr.sin_addr, ph->h_addr, ph->h_length);
    }
    addr.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    // Tell the server to shut down.
    std::string cmd = "<CdrCommandSet><CdrCommand><CdrLogon><UserName>";
    cmd += uid 
        +  "</UserName><Password>" 
        +  pwd 
        +  "</Password></CdrLogon></CdrCommand>"
           "<CdrCommand><CdrShutdown/></CdrCommand>"
           "<CdrCommand><CdrLogoff/></CdrCommand></CdrCommandSet>";

    unsigned long bytes = cmd.size();
    unsigned long length = htonl(bytes);
    if (send(sock, (char *)&length, sizeof length, 0) < 0) {
        std::cerr << "send error: " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }
    if (send(sock, cmd.c_str(), bytes, 0) < 0) {
        std::cerr << "send error: " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }

    // Determine the number of bytes in the server's reply.
    size_t totalRead = 0;
    char lengthBytes[4];
    while (totalRead < sizeof lengthBytes) {
        int leftToRead = sizeof lengthBytes - totalRead;
        int bytesRead = recv(sock, lengthBytes + totalRead, leftToRead, 0);
        if (bytesRead < 0) {
            std::cerr << "recv error: " << GetLastError() << '\n';
            return EXIT_FAILURE;
        }
        totalRead += bytesRead;
    }
    memcpy(&length, lengthBytes, sizeof length);
    length = ntohl(length);

    // Catch the server's response.
    char *response = new char[length + 1];
    memset(response, 0, length + 1);
    totalRead = 0;
    while (totalRead < length) {
        int leftToRead = length - totalRead;
        int bytesRead = recv(sock, response + totalRead, leftToRead, 0);
        if (bytesRead < 0) {
            std::cerr << "recv error: " << GetLastError() << '\n';
            return EXIT_FAILURE;
        }
        totalRead += bytesRead;
    }
    std::cout << "Server response:\n" << response << '\n';
    closesocket(sock);
    
    // Make sure we succeeded.
    if (!strstr(response, "Status='success'"))
        return EXIT_FAILURE;

    // Open another socket to break the server out of its loop.
    std::cout << "Shutting down server ";
    for (int i = 0; i < 2; ++i) {
        Sleep(1000);
        std::cout << '.';
    }
    std::cout << '\n';
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    connect(sock, (struct sockaddr *)&addr, sizeof addr);

    return EXIT_SUCCESS;
}
