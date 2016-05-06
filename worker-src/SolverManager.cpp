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
        case CT_MONOALPHABETIC_SUB_GA:
            return new Solver_MonoalphaSub();
        default:
            return nullptr;
    }
}

void SolverManager::Run()
{
    // initialization sequence (fully synchronized)
    sSession->WAITFOR(SP_HELLO_RESPONSE)->SendHello();
    sSession->WAITFOR(SP_ENC_MESSAGE)->SendGetEncMessage();
    sSession->WAITFOR(SP_END_OF_DICTIONARY)->SendGetDictionary();
    sSession->WAITFOR(SP_FREQ_MAP)->SendGetFreqMap();

    std::list<SolverResult>* resList;

    do
    {
        // retrieve work
        sSession->WAITFOR(SP_GIVE_WORK)->SendGetWork();

        // if solver not available, skip and get next work
        SolverTemplate* st = CreateSolver(m_myWork);
        if (!st)
            continue;

        // sets encrypted message to solver
        st->SetMessage(sDataHolder->GetEncryptedMessage());
        // init solver
        st->Initialize();
        // solve!
        st->Solve();

        // retrieve result list and send all results to master
        resList = st->GetResultList();

        for (SolverResult& sr : *resList)
            sSession->SendSubmitResult(sr.freqScore, sr.dictScore, sr.result.c_str());

        delete st;

        // sleep for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    while (m_myWork != CT_NONE);
}
