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
    cout << "KIV/BIT, Cryptanalysis - worker" << endl;
    cout << "Author: Martin Ubl (A13B0453P)" << endl << endl;

    // secure parameter count
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
    // attempt to connect to remote master
    if (!sSession->Connect(m_host.c_str(), m_port))
        return 1;

    sSolverManager->Init();
    sSolverManager->JoinThread();

    return 0;
}
