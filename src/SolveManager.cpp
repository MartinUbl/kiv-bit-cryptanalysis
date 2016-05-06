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
    if (argc < 3 || argc > 4)
    {
        cerr << "Invalid count of parameters" << endl;
        cerr << "Usage: " << argv[0] << " solve <input file> [dictionary base]" << endl;
        return 1;
    }

    // open source stream
    std::ifstream srcstream(argv[2]);
    if (srcstream.bad() || srcstream.fail())
    {
        cerr << "Cannot open file " << argv[2] << endl;
        return 2;
    }

    // read whole message
    m_encrypted = std::string((std::istreambuf_iterator<char>(srcstream)), std::istreambuf_iterator<char>());

    // load language file
    m_dataLoader.SetLanguageFile(argc > 3 ? argv[3] : "words");
    if (!m_dataLoader.LoadWords())
    {
        cerr << "Unable to load some of dictionary files, cannot continue" << endl;
        return 4;
    }

    // init farmer work queue
    for (int i = 0; i < MAX_CIPHER_TYPE; i++)
    {
        for (size_t j = 0; j < _solverCounts[i]; j++)
            m_workToDo.push((CipherType)i);
    }

    // initialize randomness
    srand((unsigned int)time(nullptr));

    // init daemon manager
    if (!sDaemonManager->LoadDaemons())
    {
        cerr << "Unable to load daemons" << endl;
        return 3;
    }

    cout << "Daemon manager initialized" << endl;

    // init network listener
    if (!sSessionManager->InitListener())
    {
        cerr << "Unable to initialize session manager" << endl;
        return 1;
    }

    // contact remote daemons to start workers
    int dcount = sDaemonManager->RunDaemons();

    if (dcount <= 0)
    {
        cerr << "No daemons available, exiting." << endl;
        return 5;
    }

    cout << "Available daemons: " << dcount << endl;

    cout << "Initiating solver sequence..." << endl;

    // override output file
    FILE* log_m = fopen("output.txt", "w");
    if (log_m)
        fclose(log_m);

    // time values
    time_t startTime = time(nullptr);
    time_t lastMsg = time(nullptr);
    const time_t repeatTime = 30;

    // main worker/updater loop
    while (!m_workToDo.empty() || !m_workInProgress.empty())
    {
        sSessionManager->Update();

        if (lastMsg + repeatTime < time(nullptr))
        {
            cout << "Time: " << (time(nullptr) - startTime) << "s, workers: " << sSessionManager->GetClientCount() << ", results: " << m_solveResults.size() << endl;
            lastMsg = time(nullptr);
        }
    }

    // find most suitable result
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

    // log most suitable score to output
    log_m = fopen("output.txt", "a");
    if (log_m)
    {
        fprintf(log_m, "--------------------------------");
        fprintf(log_m, "Most suitable result\n");
        fprintf(log_m, "Total score: %f\n", suitScore);
        fprintf(log_m, "Message: %s\n", suitable.c_str());
        fclose(log_m);
    }

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

    auto itr = m_workInProgress.find(undoneWork);
    if (itr != m_workInProgress.end())
        m_workInProgress.erase(itr);
}

void SolveManager::finishWork(CipherType doneWork)
{
    auto itr = m_workInProgress.find(doneWork);
    if (itr != m_workInProgress.end())
        m_workInProgress.erase(itr);
}

void SolveManager::AddClientResult(float freqScore, float dictScore, const char* result)
{
    m_solveResults.push_back({ freqScore, dictScore, result });
}
