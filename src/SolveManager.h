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

        // sets filename base for data files
        void SetLanguageFile(const char* langFile);
        // loads words from specified files
        bool LoadWords();

        // retrieve frequency map
        double* GetFrequencyMap();
        // retrieve bigram frequency map
        std::map<string, float>* GetBigramFrequencyMap();
        // retrieves dictionary
        std::vector<string>* GetDictionary();

    private:
        // internal dictionary
        std::vector<string> m_words;
        // frequency map
        double m_frequencies[ALPHABET_SIZE];
        // bigram frequency map
        std::map<string, float> m_bigramFrequencies;
        // source language file
        string m_languageFile;
};

// solver result structure
struct ClientSolveResult
{
    // frequency score
    float freqScore;
    // dictionary score
    float dictScore;
    // result itself
    string result;
};

class SolveManager
{
    friend class Singleton<SolveManager>;
    public:
        // runs solving process
        int Run(int argc, char** argv);

        // retrieve data loader
        DataLoader& getDataLoader();
        // reteieve message
        string getMessage();

        // get work (farmer-worker scheme)
        CipherType getWork();
        // return unfinished work (when i.e. client disconnects)
        void returnWork(CipherType undoneWork);
        // mark work as finished
        void finishWork(CipherType doneWork);

        // adds solver result
        void AddClientResult(float freqScore, float dictScore, const char* result);

    protected:
        SolveManager();

    private:
        // data loader instance
        DataLoader m_dataLoader;
        // encrypted message
        string m_encrypted;

        // vector of results
        std::vector<ClientSolveResult> m_solveResults;

        // farmer work queue
        std::queue<CipherType> m_workToDo;
        // work currently assigned to worker
        std::multiset<CipherType> m_workInProgress;
};

#define sSolveManager Singleton<SolveManager>::getInstance()

#endif
