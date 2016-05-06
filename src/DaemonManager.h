#ifndef KIV_BIT_CRYPTOANALYSIS_DAEMONMANAGER_H
#define KIV_BIT_CRYPTOANALYSIS_DAEMONMANAGER_H

#include "Singleton.h"

struct DaemonConfig
{
    DaemonConfig();

    // daemon host
    string host;
    // daemon port
    uint16_t port;

    // daemon name
    string name;
    // how many workers should daemon spawn for this instance
    uint16_t worker_count;
    // IP/host which is passed to daemon for connecting back
    string backhost;
    // port used for reaching master
    uint16_t backport;
};

class DaemonManager
{
    friend class Singleton<DaemonManager>;
    public:
        // loads daemons from config file
        bool LoadDaemons();
        // runs daemons
        int RunDaemons();

    protected:
        DaemonManager();

    private:
        std::list<DaemonConfig*> m_daemonPaths;
};

#define sDaemonManager Singleton<DaemonManager>::getInstance()

#endif
