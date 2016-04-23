#include <WS2tcpip.h>
#include "general.h"
#include "DaemonManager.h"
#include "SessionManager.h"

#include <string>

DaemonConfig::DaemonConfig()
{
    host = "";
    port = 0;
    backhost = "";
    backport = 0;
    name = "";
    worker_count = 1;
}

DaemonManager::DaemonManager()
{
    //
}

bool DaemonManager::LoadDaemons()
{
    FILE* f = fopen("daemons.cf", "r");
    if (!f)
    {
        cerr << "Could not load daemons.cf file" << endl;
        return false;
    }

    char buffer[256];
    buffer[255] = '\0';
    char* bufptr;
    char* valptr;

    DaemonConfig* dcfg = nullptr;

    while (fgets(buffer, 255, f))
    {
        bufptr = buffer;
        while (*bufptr == ' ')
            bufptr++;

        for (int i = strlen(bufptr); i >= 0; i--)
            if (bufptr[i] == '\n' || bufptr[i] == '\r')
                bufptr[i] = '\0';

        if (strlen(bufptr) == 0)
            continue;
        if (bufptr[0] == '#')
            continue;

        if (bufptr[0] == '[')
        {
            dcfg = new DaemonConfig;

            int hostread = 0;
            for (int i = 1; i < 255; i++)
            {
                if (!hostread && (bufptr[i] == ':' || bufptr[i] == ']'))
                {
                    dcfg->host = std::string(&bufptr[1], i - 1);
                    hostread = i;
                }
                else if (hostread && bufptr[i] == ']')
                {
                    dcfg->port = atoi(&bufptr[hostread+1]);
                }
            }

            m_daemonPaths.push_back(dcfg);
            continue;
        }

        if (!dcfg)
        {
            cerr << "Invalid line in daemons config file; everything needs to be in host regions" << endl;
            return false;
        }

        valptr = bufptr;
        while (*valptr != ' ' && *valptr != '\0')
            valptr++;
        std::string identifier(bufptr, valptr);
        while (*valptr == ' ')
            valptr++;

        if (*valptr == '\0')
        {
            cerr << "Key " << bufptr << " without value" << endl;
            return false;
        }

        if (identifier == "name")
            dcfg->name = valptr;
        else if (identifier == "worker_count")
            dcfg->worker_count = atoi(valptr);
        else if (identifier == "back_address")
            dcfg->backhost = valptr;
        else if (identifier == "back_port")
            dcfg->backport = atoi(valptr);
    }

    return true;
}

void DaemonManager::RunDaemons()
{
    for (DaemonConfig* cfg : m_daemonPaths)
    {
#ifdef _WIN32
        WORD version = MAKEWORD(1, 1);
        WSADATA data;
        if (WSAStartup(version, &data) != 0)
        {
            cerr << "Unable to start winsock service" << endl;
            break;
        }
#endif

        SOCK daemonsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in daemonsockaddr;

        daemonsockaddr.sin_family = AF_INET;
        daemonsockaddr.sin_port = htons(cfg->port);

        if (INET_PTON(AF_INET, cfg->host.c_str(), &daemonsockaddr.sin_addr.s_addr) != 1)
        {
            cerr << "Unable to resolve bind address" << endl;
            continue;
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(daemonsocket, &set);

#ifdef _WIN32
        u_long arg = 1;
        if (ioctlsocket(daemonsocket, FIONBIO, &arg) == SOCKET_ERROR)
#else
        int oldFlag = fcntl(daemonsocket, F_GETFL, 0);
        if (fcntl(daemonsocket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
#endif
        {
            cerr << "Failed to switch socket to non-blocking mode" << endl;
            continue;
        }

        if (connect(daemonsocket, (sockaddr*)&daemonsockaddr, sizeof(sockaddr_in)) < 0)
        {
            if (LASTERROR() != SOCKETINPROGRESS && LASTERROR() != SOCKETWOULDBLOCK)
            {
                cerr << "Unable to connect to daemon" << endl;
                continue;
            }
        }

        timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 1;

        select(1, nullptr, &set, nullptr, &tv);
        if (!FD_ISSET(daemonsocket, &set))
        {
            cerr << "Unable to connect to daemon" << endl;
            continue;
        }

        string tosend = cfg->backhost + " " + std::to_string(cfg->backport);
        char* buf = new char[tosend.length() + 1];
        strcpy(buf, tosend.c_str());
        buf[tosend.length()] = '\0';

        send(daemonsocket, buf, tosend.length() + 1, 0);

        delete buf;

        closesocket(daemonsocket);
    }
}
