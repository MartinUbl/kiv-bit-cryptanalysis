#ifndef KIV_BIT_CRYPTANALYSIS_SOLVER_H
#define KIV_BIT_CRYPTANALYSIS_SOLVER_H

#include "Singleton.h"
#include "SolveEnums.h"

class ResultScore
{
    public:
        // only constructor
        ResultScore(string msg, float freqScoreThreshold = 0.0f);

        // retrieve frequency score
        float GetFrequencyScore();
        // retrieve dictionary score
        float GetDictionaryScore();

    protected:
        // calculate unigram frequencies on input message
        float* CalculateFrequencies();

        // calculate frequency score
        void CalculateFrequencyScore();
        // calculate dictionary score
        void CalculateDictionaryScore();

        // frequency score
        float m_frequencyScore;
        // dictionary score
        float m_dictionaryScore;

        // input message
        string m_message;
};

typedef std::vector<string> Dictionary;
typedef std::map<string, float> StringFreqMap;

class DataHolder
{
    friend class Singleton<DataHolder>;
    public:
        // sets stored frequencies
        void SetFrequencies(uint32_t alphabet_len, float* freqs);
        // adds word to dictionary
        void AddWord(string word);
        // adds bigram with its frequency
        void AddBigram(const char* bigram, float freq);

        // retrieves alphabet length
        uint32_t GetAlphabetLength();
        // retrieves frequency map
        float* GetFrequencies();
        // retrieves dictionary
        Dictionary* GetDictionary();
        // retrueves bigram frequencies
        StringFreqMap* GetBigramFrequencies();

        // stores encrypted message
        void SetEncryptedMessage(std::string str);
        // retrieves encrypted message
        const char* GetEncryptedMessage();

    protected:
        // protected singleton constructor
        DataHolder();

    private:
        // alphabet length
        uint32_t m_alphabetLength;
        // unigram frequency map
        float* m_frequencies;
        // bigram frequency map
        StringFreqMap m_bigramFrequencies;
        // encrypted message
        std::string m_origMessage;
        // main dictionary
        Dictionary m_words;
};

#define sDataHolder Singleton<DataHolder>::getInstance()

// stored solver result
struct SolverResult
{
    // frequency score
    float freqScore;
    // dictionary score
    float dictScore;
    // result
    std::string result;
};

// abstract template for every solver
class SolverTemplate
{
    public:
        // Constructor retaining type
        SolverTemplate(CipherType solverType) : m_solverType(solverType) { };

        // sets message for decryption
        void SetMessage(std::string msg) { m_message = msg.c_str(); };

        // init solver
        virtual bool Initialize() = 0;
        // run solver
        virtual void Solve() = 0;

        // retrieve result list
        std::list<SolverResult>* GetResultList() { return &m_resultList; };

    protected:
        // adds result to list
        void AddResult(float freqScore, float dictScore, const char* result) { m_resultList.push_back({ freqScore, dictScore, result }); };

        // message to be decrypted
        std::string m_message;

    private:
        // result list
        std::list<SolverResult> m_resultList;
        // type of solver
        CipherType m_solverType;
};

// Solver for ATBAS cipher
class Solver_Atbas : public SolverTemplate
{
    public:
        Solver_Atbas() : SolverTemplate(CT_ATBAS) { };

        bool Initialize();
        void Solve();

    protected:
        //

    private:
        //
};

// Solver for caesar cipher
class Solver_Caesar : public SolverTemplate
{
    public:
        Solver_Caesar() : SolverTemplate(CT_CAESAR) { };

        bool Initialize();
        void Solve();

    protected:
        //

    private:
        //
};

// Solver for Vigenere cipher
class Solver_Vigenere : public SolverTemplate
{
    public:
        Solver_Vigenere() : SolverTemplate(CT_VIGENERE) { };

        bool Initialize();
        void Solve();

    protected:
        //

    private:
        //
};

// Solver for monoalphabetic substitution cipher
class Solver_MonoalphaSub : public SolverTemplate
{
    public:
        Solver_MonoalphaSub() : SolverTemplate(CT_MONOALPHABETIC_SUB_GA) { };

        bool Initialize();
        void Solve();

    protected:
        // chromosome structure
        struct Chromosome
        {
            // ciphertext alphabet
            char alphabet[26];
            // fitness value
            float fitness;
            // needs fitness recalculation?
            bool changed;
        };

        // recalculates fitness where it's needed
        void RecalculateFitness();
        // recalculate fitness of specific chromosome
        void RecalculateFitnessOn(Chromosome* gen);
        // performs crossover on random chromosomes
        void PerformCrossover();
        // perform mutation on random chromosomes
        void PerformMutation();
        // stores elite
        void ElitismStore();
        // restores elite into population
        void ElitismRestore();
        // performs word mapping
        void PerformWordMapping();
        // performs crossover of two input chromosomes
        void PerformCrossoverOn(Chromosome* src, Chromosome* dst);
        // performs mutation on supplied chromosome
        void PerformMutationOn(Chromosome* dst);
        // performs vowel permutation on supplied chromosome
        void PerformVowelPermutationOn(Chromosome* dst);
        // performs word mapping on chromosome
        void PerformWordMappingOn(Chromosome* dst);

    private:
        // all population
        Chromosome** m_population;
        // stored elite
        Chromosome* m_elite;
        // stored original frequency mapped chromosome
        Chromosome* m_original;
        // size of population
        uint32_t m_populationSize;

        // dictionary of ciphertext
        Dictionary m_cipherDictionary;
};

class SolverManager
{
    friend class Singleton<SolverManager>;
    public:
        // init solver
        void Init();

        // join solver thread
        void JoinThread();

        // sets what we are solving right now
        void SetObtainedWork(CipherType ct);

    protected:
        // protected singleton constructor
        SolverManager();
        // runs solver manager
        void Run();

        // creates solver instance
        SolverTemplate* CreateSolver(CipherType ct);

    private:
        // solver thread
        std::thread *m_solverThread;
        // currently processed work
        CipherType m_myWork;
};

#define sSolverManager Singleton<SolverManager>::getInstance()

#endif
