#ifndef KIV_BIT_SOLVEMANAGER_H
#define KIV_BIT_SOLVEMANAGER_H

#include "Singleton.h"
#include "SolveEnums.h"

#define ALPHABET_SIZE 26

static uint32_t _solverCounts[MAX_CIPHER_TYPE] = {
    0, // CT_NONE
    1, // CT_ATBAS
    1, // CT_CAESAR
    1, // CT_VIGENERE
    0, // CT_MONOALPHABETIC_SUB
    0, // CT_BIALPHABETIC_SUB (NYI!)
};

class DataLoader
{
    public:
        DataLoader();

        void SetLanguageFile(const char* langFile);

        void LoadWords();

        double* GetFrequencyMap();
        std::vector<string>* GetDictionary();

    private:
        std::vector<string> m_words;
        double m_frequencies[ALPHABET_SIZE];
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

        void AddClientResult(float freqScore, float dictScore, const char* result);

    protected:
        SolveManager();

    private:
        DataLoader m_dataLoader;
        string m_encrypted;

        CipherType m_currentCipher;
        uint32_t m_currentCounter;

        std::vector<ClientSolveResult> m_solveResults;
};

#define sSolveManager Singleton<SolveManager>::getInstance()

#endif
