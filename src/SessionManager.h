#ifndef KIV_BIT_SESSIONMANAGER_H
#define KIV_BIT_SESSIONMANAGER_H

#include "Singleton.h"

#ifdef _WIN32
#define SOCK SOCKET
#define ADDRLEN int

#define SOCKETWOULDBLOCK WSAEWOULDBLOCK
#define SOCKETCONNRESET  WSAECONNRESET
#define SOCKETCONNABORT  WSAECONNABORTED
#define SOCKETINPROGRESS WSAEINPROGRESS
#define LASTERROR() WSAGetLastError()
#define INET_PTON(fam,addrptr,buff) InetPton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) InetNtop(fam,addrptr,buff,socksize)
#define CLOSESOCKET closesocket
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
#define SOCKETINPROGRESS EINPROGRESS
#define LASTERROR() errno
#define INET_PTON(fam,addrptr,buff) inet_pton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) inet_ntop(fam,addrptr,buff,socksize)
#define CLOSESOCKET close
#endif

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

class ClientSolver;
class SmartPacket;

class SessionManager
{
    friend class Singleton<SessionManager>;
    public:
        ~SessionManager();

        bool InitListener();
        void Update();

        void SendPacket(SmartPacket &pkt, SOCK targetSocket);

    protected:
        SessionManager();

    private:
        SOCK m_socket;
        sockaddr_in m_sockAddr;

        SOCK nfds;

        fd_set m_socketSet;
        std::set<SOCK> m_activeSockets;
        std::map<SOCK, ClientSolver*> m_clientSolverMap;
};

#define sSessionManager Singleton<SessionManager>::getInstance()

#endif
