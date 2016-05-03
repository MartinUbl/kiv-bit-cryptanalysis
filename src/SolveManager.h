#ifndef KIV_BIT_SOLVEMANAGER_H
#define KIV_BIT_SOLVEMANAGER_H

#include "Singleton.h"
#include "SolveEnums.h"

#include <queue>
#include <set>

#define ALPHABET_SIZE 26

static uint32_t _solverCounts[MAX_CIPHER_TYPE] = {
    0, // CT_NONE
    1, // CT_ATBAS
    1, // CT_CAESAR
    1, // CT_VIGENERE
    4, // CT_MONOALPHABETIC_SUB_GA
    0, // CT_BIALPHABETIC_SUB (NYI!)
};

class DataLoader
{
    public:
        DataLoader();

        void SetLanguageFile(const char* langFile);

        void LoadWords();

        double* GetFrequencyMap();
        std::map<string, float>* GetBigramFrequencyMap();
        std::vector<string>* GetDictionary();

    private:
        std::vector<string> m_words;
        double m_frequencies[ALPHABET_SIZE];
        std::map<string, float> m_bigramFrequencies;
        string m_languageFile;
};

struct ClientSolveResult
{
    float freqScore;
    float dictScore;
    string result;
};

class SolveManager
{
    friend class Singleton<SolveManager>;
    public:
        int Run(int argc, char** argv);

        DataLoader& getDataLoader();
        string getMessage();

        CipherType getWork();
        void returnWork(CipherType undoneWork);
        void finishWork(CipherType doneWork);

        void AddClientResult(float freqScore, float dictScore, const char* result);

    protected:
        SolveManager();

    private:
        DataLoader m_dataLoader;
        string m_encrypted;

        std::vector<ClientSolveResult> m_solveResults;

        std::queue<CipherType> m_workToDo;
        std::multiset<CipherType> m_workInProgress;
};

#define sSolveManager Singleton<SolveManager>::getInstance()

#endif
