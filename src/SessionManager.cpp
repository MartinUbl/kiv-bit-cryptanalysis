#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif
#include "general.h"
#include "SmartPacket.h"
#include "SessionManager.h"
#include "Opcodes.h"
#include "ClientSolver.h"

#define RECV_BUFFER_SIZE 2048
uint8_t _receiveBuffer[RECV_BUFFER_SIZE];

SessionManager::SessionManager()
{
    //
}

SessionManager::~SessionManager()
{
    //
}

bool SessionManager::InitListener()
{
#ifdef _WIN32
    WORD version = MAKEWORD(1, 1);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
    {
        cerr << "Unable to start winsock service" << endl;
        return false;
    }
#endif

    if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        cerr << "Unable to create socket" << endl;
        return false;
    }

    int param = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&param, sizeof(int)) == -1)
    {
        cerr << "Failed to use SO_REUSEADDR flag, bind may fail due to orphan connections to old socket" << endl;
        // do not fail whole process, this is not mandatory
    }

    // retrieve address
    std::string bindAddr = "0.0.0.0";
    uint16_t m_port = 8170;

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons(m_port);

    // resolve IP address from entered host/IP address in config
    if (INET_PTON(AF_INET, bindAddr.c_str(), &m_sockAddr.sin_addr.s_addr) != 1)
    {
        cerr << "Unable to resolve bind address" << endl;
        return false;
    }

    // detect invalid address supplied in config
    if (m_sockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        cerr << "Invalid bind address specified. Please, specify valid IPv4 address" << endl;
        return false;
    }

    // bind to network interface/address
    if (bind(m_socket, (sockaddr*)&m_sockAddr, sizeof(m_sockAddr)) == -1)
    {
        cerr << "Failed to bind socket" << endl;
        return false;
    }

    // create listen queue to be checked
    if (listen(m_socket, 10) == -1)
    {
        cerr << "Couldn't create connection queue" << endl;
        return false;
    }

#ifdef _WIN32
    u_long arg = 1;
    if (ioctlsocket(m_socket, FIONBIO, &arg) == SOCKET_ERROR)
#else
    int oldFlag = fcntl(m_socket, F_GETFL, 0);
    if (fcntl(m_socket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
#endif
    {
        cerr << "Failed to switch socket to non-blocking mode" << endl;
        return false;
    }

    FD_ZERO(&m_socketSet);
    FD_SET(m_socket, &m_socketSet);
    m_activeSockets.insert(m_socket);

    nfds = m_socket;

    return true;
}

void SessionManager::SendPacket(SmartPacket &pkt, SOCK targetSocket)
{
    uint16_t op, sz;
    uint8_t* tosend = new uint8_t[SMARTPACKET_HEADER_SIZE + pkt.GetSize()];

    op = htons(pkt.GetOpcode());
    sz = htons(pkt.GetSize());

    // write opcode
    memcpy(tosend, &op, 2);
    // write contents size
    memcpy(tosend + 2, &sz, 2);
    // write contents
    memcpy(tosend + 4, pkt.GetData(), pkt.GetSize());

    // send response
    send(targetSocket, (const char*)tosend, pkt.GetSize() + SMARTPACKET_HEADER_SIZE, MSG_NOSIGNAL);
}

void SessionManager::Update()
{
    fd_set acs;
    timeval tv;
    int res;
    struct
    {
        uint16_t opcode;
        uint16_t size;
    } pktheader;

    memcpy(&acs, &m_socketSet, sizeof(fd_set));

    tv.tv_sec = 1;
    tv.tv_usec = 1;

    res = select(nfds + 1, &acs, nullptr, nullptr, &tv);
    if (res < 0)
    {
        cerr << "select(): error " << LASTERROR() << endl;
    }
    else if (res == 0)
    {
        // timeout, try again later
        return;
    }

    std::set<SOCK> toremove;
    std::set<SOCK> toadd;

    for (SOCK sc : m_activeSockets)
    {
        if (FD_ISSET(sc, &acs))
        {
            if (sc == m_socket)
            {
                sockaddr_in* addr = new sockaddr_in;
                int addrlen = sizeof(sockaddr_in);
                res = accept(m_socket, (sockaddr*)addr, (socklen_t*)&addrlen);

                if (res > 0)
                {
                    cout << "Accepted connection!" << endl;

                    toadd.insert((SOCK)res);
                    FD_SET(res, &m_socketSet);
                    m_clientSolverMap[(SOCK)res] = new ClientSolver((SOCK)res);

                    if (res > nfds)
                        nfds = res;
                }
                else
                {
                    cerr << "Error when accepting connection" << endl;
                    delete addr;
                }
            }
            else
            {
                res = recv(sc, (char*)&pktheader, SMARTPACKET_HEADER_SIZE, 0);
                if (res < SMARTPACKET_HEADER_SIZE)
                {
                    if (res == -1 || LASTERROR() == SOCKETCONNRESET || LASTERROR() == SOCKETCONNABORT)
                        cout << "Client disconnected" << endl;
                    else
                        cerr << "Malformed packet received, disconnecting client" << endl;

                    toremove.insert(sc);
                    m_clientSolverMap.erase(sc);
                    FD_CLR(sc, &m_socketSet);

                    continue;
                }

                pktheader.opcode = ntohs(pktheader.opcode);
                pktheader.size = ntohs(pktheader.size);

                if (pktheader.size > 0)
                {
                    int recbytes = 0;

                    while (recbytes != pktheader.size)
                    {
                        res = recv(sc, (char*)(_receiveBuffer + recbytes), pktheader.size - recbytes, 0);
                        if (res <= 0)
                        {
                            cerr << "Received less bytes than expected, not handling" << endl;
                            recbytes = -1;
                            break;
                        }
                        recbytes += res;
                    }

                    if (recbytes == -1)
                        continue;
                }

                SmartPacket pkt(pktheader.opcode, pktheader.size);
                pkt.SetData(_receiveBuffer, pktheader.size);

                m_clientSolverMap[sc]->HandlePacket(pkt);
            }
        }
    }

    // remove disconnected socket handles
    for (SOCK sc : toremove)
        m_activeSockets.erase(sc);
    // add newly accepted
    for (SOCK sc : toadd)
        m_activeSockets.insert(sc);
}
