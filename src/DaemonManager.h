#ifndef KIV_BIT_CRYPTOANALYSIS_DAEMONMANAGER_H
#define KIV_BIT_CRYPTOANALYSIS_DAEMONMANAGER_H

#include "Singleton.h"

struct DaemonConfig
{
    DaemonConfig();

    string host;
    uint16_t port;

    string name;
    uint16_t worker_count;
    string backhost;
    uint16_t backport;
};

class DaemonManager
{
    friend class Singleton<DaemonManager>;
    public:
        bool LoadDaemons();
        void RunDaemons();

    protected:
        DaemonManager();

    private:
        std::list<DaemonConfig*> m_daemonPaths;
};

#define sDaemonManager Singleton<DaemonManager>::getInstance()

#endif
