#ifdef _WIN32
#include <WS2tcpip.h>
#endif
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
    // load daemons from file
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

    // parse file by lines
    while (fgets(buffer, 255, f))
    {
        // erase initial whitespaces
        bufptr = buffer;
        while (*bufptr == ' ' || *bufptr == '\t')
            bufptr++;

        // cut line endings (CR and LF)
        for (int i = strlen(bufptr); i >= 0; i--)
            if (bufptr[i] == '\n' || bufptr[i] == '\r')
                bufptr[i] = '\0';

        // skip empty lines
        if (strlen(bufptr) == 0)
            continue;
        // skip comments (beginning with # character)
        if (bufptr[0] == '#')
            continue;

        // lines beginning with [ character begins new single daemon block
        if (bufptr[0] == '[')
        {
            dcfg = new DaemonConfig;

            int hostread = 0;
            for (int i = 1; i < 255; i++)
            {
                // parse host
                if (!hostread && (bufptr[i] == ':' || bufptr[i] == ']'))
                {
                    dcfg->host = std::string(&bufptr[1], i - 1);
                    hostread = i;
                }
                // parse port
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

        // go to the end of identifier
        valptr = bufptr;
        while (*valptr != ' ' && *valptr != '\0')
            valptr++;

        // identifier is now parsed
        std::string identifier(bufptr, valptr);
        // skip spaces
        while (*valptr == ' ')
            valptr++;

        // if no value present.. exit
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

int DaemonManager::RunDaemons()
{
    // daemons, that we succeeded to contact
    int okcount = 0;

    for (DaemonConfig* cfg : m_daemonPaths)
    {
        cout << "Connecting to " << cfg->name.c_str() << " - " << cfg->host.c_str() << ", port: " << cfg->port << " ... ";

        // init socket
        SOCK daemonsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in daemonsockaddr;

        daemonsockaddr.sin_family = AF_INET;
        daemonsockaddr.sin_port = htons(cfg->port);

        if (INET_PTON(AF_INET, cfg->host.c_str(), &daemonsockaddr.sin_addr.s_addr) != 1)
        {
            cerr << endl << "Unable to resolve address" << endl;
            continue;
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(daemonsocket, &set);

        // switch to nonblocking mode
#ifdef _WIN32
        u_long arg = 1;
        if (ioctlsocket(daemonsocket, FIONBIO, &arg) == SOCKET_ERROR)
#else
        int oldFlag = fcntl(daemonsocket, F_GETFL, 0);
        if (fcntl(daemonsocket, F_SETFL, oldFlag | O_NONBLOCK) == -1)
#endif
        {
            cerr << endl << "Failed to switch socket to non-blocking mode" << endl;
            continue;
        }

        // connect to daemon
        if (connect(daemonsocket, (sockaddr*)&daemonsockaddr, sizeof(sockaddr_in)) < 0)
        {
            // if connection could not be estabilished, skip this daemon
            if (LASTERROR() != SOCKETINPROGRESS && LASTERROR() != SOCKETWOULDBLOCK)
            {
                cout << "FAILED (" << LASTERROR() << ")" << endl;
                continue;
            }
        }

        // 3 seconds timeout
        timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 1;

        // wait 3 seconds to connect
        select((int)(daemonsocket + 1), nullptr, &set, nullptr, &tv);
        // socket would be in writable fdset if connection succeeded
        if (!FD_ISSET(daemonsocket, &set))
        {
            cout << "FAILED (" << LASTERROR() << ")" << endl;
            continue;
        }

        // pass connection info
        string tosend = std::to_string(cfg->worker_count) + " " + cfg->backhost + " " + std::to_string(cfg->backport);
        char* buf = new char[tosend.length() + 1];
        strcpy(buf, tosend.c_str());
        buf[tosend.length()] = '\0';

        // send it
        send(daemonsocket, buf, tosend.length() + 1, 0);

        delete buf;

        // and close socket
        CLOSESOCKET(daemonsocket);

        cout << "OK" << endl;
        okcount++;
    }

    return okcount;
}
