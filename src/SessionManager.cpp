#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif
#include "general.h"
#include "SmartPacket.h"
#include "SessionManager.h"
#include "Opcodes.h"
#include "ClientSolver.h"
#include "SolveManager.h"

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
    // init winsock if on windows
#ifdef _WIN32
    WORD version = MAKEWORD(1, 1);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
    {
        cerr << "NET: Unable to start winsock service" << endl;
        return false;
    }
#endif

    if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        cerr << "NET: Unable to create socket" << endl;
        return false;
    }

    int param = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&param, sizeof(int)) == -1)
    {
        cerr << "NET: Failed to use SO_REUSEADDR flag, bind may fail due to orphan connections to old socket" << endl;
        // do not fail whole process, this is not mandatory
    }

    // TODO: make this customizable - config file?
    std::string bindAddr = "0.0.0.0";
    uint16_t m_port = 8170;

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons(m_port);

    // resolve IP address from entered host/IP address in config
    if (INET_PTON(AF_INET, bindAddr.c_str(), &m_sockAddr.sin_addr.s_addr) != 1)
    {
        cerr << "NET: Unable to resolve bind address" << endl;
        return false;
    }

    // detect invalid address supplied in config
    if (m_sockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        cerr << "NET: Invalid bind address specified. Please, specify valid IPv4 address" << endl;
        return false;
    }

    // bind to network interface/address
    if (bind(m_socket, (sockaddr*)&m_sockAddr, sizeof(m_sockAddr)) == -1)
    {
        cerr << "NET: Failed to bind socket" << endl;
        return false;
    }

    // create listen queue to be checked
    if (listen(m_socket, 10) == -1)
    {
        cerr << "NET: Couldn't create connection queue" << endl;
        return false;
    }

    // switch to nonblocking mode
#ifdef _WIN32
    u_long arg = 1;
    if (ioctlsocket(m_socket, FIONBIO, &arg) == SOCKET_ERROR)
#else
    int oldFlag = fcntl(m_socket, F_GETFL, 0);
    if (fcntl(m_socket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
#endif
    {
        cerr << "NET: Failed to switch socket to non-blocking mode" << endl;
        return false;
    }

    // init fdset
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

    // temp space for address due to logs
    char tmpaddr[18];
    memset(tmpaddr, 0, 18);

    memcpy(&acs, &m_socketSet, sizeof(fd_set));

    // 1 second timeout
    tv.tv_sec = 1;
    tv.tv_usec = 1;

    // select sockets for reading
    res = select(nfds + 1, &acs, nullptr, nullptr, &tv);
    if (res < 0)
    {
        cerr << "NET: select(): error " << LASTERROR() << endl;
    }
    else if (res == 0)
    {
        // timeout, try again later
        return;
    }

    std::set<SOCK> toremove;
    std::set<SOCK> toadd;

    // go through active sockets
    for (SOCK sc : m_activeSockets)
    {
        // if socket is set...
        if (FD_ISSET(sc, &acs))
        {
            // if it's our socket, someone is at the door
            if (sc == m_socket)
            {
                // try to accept the connection
                sockaddr_in* addr = new sockaddr_in;
                int addrlen = sizeof(sockaddr_in);
                res = accept(m_socket, (sockaddr*)addr, (socklen_t*)&addrlen);

                if (res > 0)
                {
                    INET_NTOP(AF_INET, &addr->sin_addr, tmpaddr, addrlen);
                    cout << "NET: Accepted connection from " << tmpaddr << endl;

                    // put to solver map
                    toadd.insert((SOCK)res);
                    FD_SET(res, &m_socketSet);
                    m_clientSolverMap[(SOCK)res] = new ClientSolver((SOCK)res);

                    // increase nfds for select call
                    if (res > (int)nfds)
                        nfds = res;
                }
                else
                {
                    cerr << "NET: Error when accepting connection" << endl;
                    delete addr;
                }
            }
            else
            {
                // receive what's on input
                res = recv(sc, (char*)&pktheader, SMARTPACKET_HEADER_SIZE, 0);
                // secure minimum length
                if (res < SMARTPACKET_HEADER_SIZE)
                {
                    if (res == -1 || LASTERROR() == SOCKETCONNRESET || LASTERROR() == SOCKETCONNABORT)
                        cout << "NET: Client disconnected" << endl;
                    else
                        cerr << "NET: Malformed packet received, disconnecting client" << endl;

                    // return work, if any - it now counts as undone
                    if (m_clientSolverMap[sc]->GetMyWork() != CT_NONE)
                        sSolveManager->returnWork(m_clientSolverMap[sc]->GetMyWork());

                    toremove.insert(sc);
                    m_clientSolverMap.erase(sc);
                    FD_CLR(sc, &m_socketSet);

                    continue;
                }

                // retrieve opcode and size
                pktheader.opcode = ntohs(pktheader.opcode);
                pktheader.size = ntohs(pktheader.size);

                if (pktheader.size > 0)
                {
                    int recbytes = 0;

                    // receive while there's something to receive
                    while (recbytes != pktheader.size)
                    {
                        res = recv(sc, (char*)(_receiveBuffer + recbytes), pktheader.size - recbytes, 0);
                        if (res <= 0)
                        {
                            if (LASTERROR() == SOCKETWOULDBLOCK)
                                continue;
                            cerr << "NET: Received less bytes than expected, not handling" << endl;
                            recbytes = -1;
                            break;
                        }
                        recbytes += res;
                    }

                    if (recbytes == -1)
                        continue;
                }

                // build packet
                SmartPacket pkt(pktheader.opcode, pktheader.size);
                pkt.SetData(_receiveBuffer, pktheader.size);

                // handle it
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

uint32_t SessionManager::GetClientCount()
{
    return (uint32_t)m_clientSolverMap.size();
}
