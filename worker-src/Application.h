#ifndef KIV_BIT_CRYPTANALYSIS_APPLICATION_H
#define KIV_BIT_CRYPTANALYSIS_APPLICATION_H

#include "Singleton.h"

class Application
{
    friend class Singleton<Application>;
    public:
        // inits application run
        bool Init(int argc, char** argv);
        // runs the app
        int Run();

    protected:
        // protected singleton constructor
        Application();

    private:
        // target host
        std::string m_host;
        // target port
        uint16_t m_port;
};

#define sApplication Singleton<Application>::getInstance()

#endif
