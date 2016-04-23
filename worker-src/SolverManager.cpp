#include "general.h"
#include "Solver.h"
#include "Session.h"

SolverManager::SolverManager()
{
    //
}

void SolverManager::Init()
{
    m_myWork = CT_NONE;
    m_solverThread = new std::thread(&SolverManager::Run, this);
}

void SolverManager::JoinThread()
{
    m_solverThread->join();
}

void SolverManager::SetObtainedWork(CipherType ct)
{
    m_myWork = ct;
}

SolverTemplate* SolverManager::CreateSolver(CipherType ct)
{
    switch (ct)
    {
        case CT_ATBAS:
            return new Solver_Atbas();
        case CT_CAESAR:
            return new Solver_Caesar();
        case CT_VIGENERE:
            return new Solver_Vigenere();
        default:
            return nullptr;
    }
}

void SolverManager::Run()
{
    sSession->WAITFOR(SP_HELLO_RESPONSE)->SendHello();
    sSession->WAITFOR(SP_ENC_MESSAGE)->SendGetEncMessage();
    sSession->WAITFOR(SP_END_OF_DICTIONARY)->SendGetDictionary();
    sSession->WAITFOR(SP_FREQ_MAP)->SendGetFreqMap();

    std::list<SolverResult>* resList;

    do
    {
        sSession->WAITFOR(SP_GIVE_WORK)->SendGetWork();

        SolverTemplate* st = CreateSolver(m_myWork);
        if (!st)
            continue;

        st->SetMessage(sDataHolder->GetEncryptedMessage());
        st->Initialize();
        st->Solve();

        resList = st->GetResultList();

        for (SolverResult& sr : *resList)
            sSession->SendSubmitResult(sr.freqScore, sr.dictScore, sr.result.c_str());

        delete st;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    while (m_myWork != CT_NONE);
}
