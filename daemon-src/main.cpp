#include <WS2tcpip.h>
#include "general.h"

#ifdef _WIN32
#define SOCK SOCKET
#define ADDRLEN int

#define SOCKETWOULDBLOCK WSAEWOULDBLOCK
#define SOCKETCONNRESET  WSAECONNRESET
#define SOCKETCONNABORT  WSAECONNABORTED
#define LASTERROR() WSAGetLastError()
#define INET_PTON(fam,addrptr,buff) InetPton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) InetNtop(fam,addrptr,buff,socksize)
#else
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <fcntl.h>

#define SOCK int
#define ADDRLEN socklen_t

#define INVALID_SOCKET -1

#define SOCKETWOULDBLOCK EAGAIN
#define SOCKETCONNABORT ECONNABORTED
#define SOCKETCONNRESET ECONNRESET
#define LASTERROR() errno
#define INET_PTON(fam,addrptr,buff) inet_pton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) inet_ntop(fam,addrptr,buff,socksize)
#endif

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

int main(int argc, char** argv)
{
    SOCK mysocket;
    sockaddr_in mysockAddr;

#ifdef _WIN32
    WORD version = MAKEWORD(1, 1);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
    {
        cerr << "Unable to start winsock service" << endl;
        return 1;
    }
#endif

    if ((mysocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        cerr << "Unable to create socket" << endl;
        return 1;
    }

    int param = 1;
    if (setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&param, sizeof(int)) == -1)
    {
        cerr << "Failed to use SO_REUSEADDR flag, bind may fail due to orphan connections to old socket" << endl;
        // do not fail whole process, this is not mandatory
    }

    // retrieve address
    std::string bindAddr = argv[1];
    uint16_t m_port = atoi(argv[2]);

    mysockAddr.sin_family = AF_INET;
    mysockAddr.sin_port = htons(m_port);

    // resolve IP address from entered host/IP address in config
    if (INET_PTON(AF_INET, bindAddr.c_str(), &mysockAddr.sin_addr.s_addr) != 1)
    {
        cerr << "Unable to resolve bind address" << endl;
        return 1;
    }

    // detect invalid address supplied in config
    if (mysockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        cerr << "Invalid bind address specified. Please, specify valid IPv4 address" << endl;
        return 1;
    }

    // bind to network interface/address
    if (bind(mysocket, (sockaddr*)&mysockAddr, sizeof(mysockAddr)) == -1)
    {
        cerr << "Failed to bind socket" << endl;
        return 1;
    }

    // create listen queue to be checked
    if (listen(mysocket, 10) == -1)
    {
        cerr << "Couldn't create connection queue" << endl;
        return 1;
    }

#ifdef _WIN32
    u_long arg = 1;
    if (ioctlsocket(mysocket, FIONBIO, &arg) == SOCKET_ERROR)
#else
    int oldFlag = fcntl(mysocket, F_GETFL, 0);
    if (fcntl(mysocket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
#endif
    {
        cerr << "Failed to switch socket to non-blocking mode" << endl;
        return 1;
    }

    fd_set sockset;

    FD_ZERO(&sockset);
    FD_SET(mysocket, &sockset);

    char buffer[32];

    while (true)
    {
        fd_set workset;
        memcpy(&workset, &sockset, sizeof(fd_set));

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 1;

        int res = select(1, &workset, nullptr, nullptr, &tv);
        if (res < 0)
        {
            // err
        }
        else if (res == 0)
        {
            // timeout
        }
        else
        {
            sockaddr_in addr;
            int addrlen = sizeof(sockaddr_in);
            int remotesocket = accept(mysocket, (sockaddr*)&addr, &addrlen);

            if (remotesocket > 0)
            {
                do
                {
                    res = recv(remotesocket, buffer, 32, 0);

                    if (res > 0)
                    {
                        cout << "Creating process, args: " << buffer << endl;
                        STARTUPINFO siStartupInfo;
                        PROCESS_INFORMATION piProcessInfo;
                        memset(&siStartupInfo, 0, sizeof(siStartupInfo));
                        memset(&piProcessInfo, 0, sizeof(piProcessInfo));
                        siStartupInfo.cb = sizeof(siStartupInfo);

                        std::string cmdline = "kiv-bit-cryptanalysis-worker.exe ";
                        cmdline += buffer;

                        CreateProcess("kiv-bit-cryptanalysis-worker.exe", (LPSTR)cmdline.c_str(), 0, 0, false, 0, nullptr, nullptr, &siStartupInfo, &piProcessInfo);
                    }

                } while (LASTERROR() == SOCKETWOULDBLOCK);
            }
        }
    }

    return 0;
}
