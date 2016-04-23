#include "general.h"
#include "Application.h"
#include "Session.h"
#include "Solver.h"

Application::Application()
{
    //
}

bool Application::Init(int argc, char** argv)
{
    if (argc < 3)
    {
        cerr << "Wrong number of parameters!" << endl
             << "Usage: " << argv[0] << " <master ip> <master port>" << endl;
        return false;
    }

    m_host = argv[1];
    m_port = (uint16_t)atoi(argv[2]);

    return true;
}

int Application::Run()
{
    if (!sSession->Connect(m_host.c_str(), m_port))
        return 1;

    sSolverManager->Init();
    sSolverManager->JoinThread();

    return 0;
}
