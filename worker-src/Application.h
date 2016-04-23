#ifndef KIV_BIT_CRYPTANALYSIS_APPLICATION_H
#define KIV_BIT_CRYPTANALYSIS_APPLICATION_H

#include "Singleton.h"

class Application
{
    friend class Singleton<Application>;
    public:
        bool Init(int argc, char** argv);
        int Run();

    protected:
        Application();

    private:
        std::string m_host;
        uint16_t m_port;
};

#define sApplication Singleton<Application>::getInstance()

#endif
