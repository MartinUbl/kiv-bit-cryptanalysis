#include "general.h"
#include "SolveManager.h"
#include "SessionManager.h"
#include "DaemonManager.h"

#include <fstream>
#include <ctime>

SolveManager::SolveManager()
{
    //
}

int SolveManager::Run(int argc, char** argv)
{
    if (argc < 3)
    {
        cerr << "Not enough parameters" << endl;
        cerr << "Usage: " << argv[0] << " solve <input file>" << endl;
        return 1;
    }

    std::ifstream srcstream(argv[2]);

    if (srcstream.bad() || srcstream.fail())
    {
        cerr << "Cannot open file " << argv[2] << endl;
        return 2;
    }

    m_encrypted = std::string((std::istreambuf_iterator<char>(srcstream)), std::istreambuf_iterator<char>());

    m_dataLoader.SetLanguageFile("cs_words");
    m_dataLoader.LoadWords();

    for (int i = 0; i < MAX_CIPHER_TYPE; i++)
    {
        for (int j = 0; j < _solverCounts[i]; j++)
            m_workToDo.push((CipherType)i);
    }

    srand((unsigned int)time(nullptr));

    if (!sDaemonManager->LoadDaemons())
    {
        cerr << "Unable to load daemons" << endl;
        return 3;
    }

    if (!sSessionManager->InitListener())
    {
        cerr << "Unable to initialize session manager" << endl;
        return 1;
    }

    sDaemonManager->RunDaemons();

    cout << "Waiting for clients..." << endl;

    while (!m_workToDo.empty() || !m_workInProgress.empty())
    {
        sSessionManager->Update();
    }

    std::string suitable;
    float suitScore = 0.0f;

    for (ClientSolveResult& sr : m_solveResults)
    {
        if (sr.freqScore*sr.dictScore > suitScore)
        {
            suitScore = sr.freqScore*sr.dictScore;
            suitable = sr.result.c_str();
        }
    }

    cout << "Most suitable score: " << suitScore << endl << suitable.c_str() << endl;

    return 0;
}

DataLoader& SolveManager::getDataLoader()
{
    return m_dataLoader;
}

string SolveManager::getMessage()
{
    return m_encrypted;
}

CipherType SolveManager::getWork()
{
    if (m_workToDo.empty())
        return CT_NONE;

    CipherType work = m_workToDo.front();
    m_workToDo.pop();
    m_workInProgress.insert(work);

    return work;
}

void SolveManager::returnWork(CipherType undoneWork)
{
    m_workToDo.push(undoneWork);
    m_workInProgress.erase(undoneWork);
}

void SolveManager::finishWork(CipherType doneWork)
{
    m_workInProgress.erase(doneWork);
}

void SolveManager::AddClientResult(float freqScore, float dictScore, const char* result)
{
    m_solveResults.push_back({ freqScore, dictScore, result });
}
