#include "general.h"
#include "SolveManager.h"
#include "SessionManager.h"
#include "DaemonManager.h"

#include <fstream>
#include <ctime>

SolveManager::SolveManager()
{
    m_currentCipher = CT_NONE;
    m_currentCounter = 0;
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

    while (m_currentCipher != MAX_CIPHER_TYPE)
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

    cin.get();

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
    while (m_currentCipher != MAX_CIPHER_TYPE)
    {
        while (m_currentCounter >= _solverCounts[m_currentCipher])
        {
            m_currentCounter = 0;
            m_currentCipher = (CipherType)(m_currentCipher + 1);

            if (m_currentCipher == MAX_CIPHER_TYPE)
                return CT_NONE;
        }

        m_currentCounter++;
        return m_currentCipher;
    }

    return CT_NONE;
}

void SolveManager::AddClientResult(float freqScore, float dictScore, const char* result)
{
    m_solveResults.push_back({ freqScore, dictScore, result });
}
