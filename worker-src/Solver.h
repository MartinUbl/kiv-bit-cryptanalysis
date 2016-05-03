#ifndef KIV_BIT_CRYPTANALYSIS_SOLVER_H
#define KIV_BIT_CRYPTANALYSIS_SOLVER_H

#include "Singleton.h"
#include "SolveEnums.h"

class ResultScore
{
    public:
        ResultScore(string msg, float freqScoreThreshold = 0.0f);

        float GetFrequencyScore();
        float GetDictionaryScore();

    protected:
        float* CalculateFrequencies();

        void CalculateFrequencyScore();
        void CalculateDictionaryScore();

        float m_frequencyScore;
        float m_dictionaryScore;

        string m_message;
};

typedef std::vector<string> Dictionary;
typedef std::map<string, float> StringFreqMap;

class DataHolder
{
    friend class Singleton<DataHolder>;
    public:
        void SetFrequencies(uint32_t alphabet_len, float* freqs);
        void AddWord(string word);
        void AddBigram(const char* bigram, float freq);

        uint32_t GetAlphabetLength();
        float* GetFrequencies();
        Dictionary* GetDictionary();
        StringFreqMap* GetBigramFrequencies();

        void SetEncryptedMessage(std::string str);
        const char* GetEncryptedMessage();

    protected:
        DataHolder();

    private:
        uint32_t m_alphabetLength;
        float* m_frequencies;
        StringFreqMap m_bigramFrequencies;
        std::string m_origMessage;
        Dictionary m_words;
};

#define sDataHolder Singleton<DataHolder>::getInstance()

struct SolverResult
{
    float freqScore;
    float dictScore;
    std::string result;
};

class SolverTemplate
{
    public:
        SolverTemplate(CipherType solverType) : m_solverType(solverType) { };

        void SetMessage(std::string msg) { m_message = msg.c_str(); };

        virtual bool Initialize() = 0;
        virtual void Solve() = 0;

        std::list<SolverResult>* GetResultList() { return &m_resultList; };

    protected:
        void AddResult(float freqScore, float dictScore, const char* result) { m_resultList.push_back({ freqScore, dictScore, result }); };

        std::string m_message;

    private:
        std::list<SolverResult> m_resultList;
        CipherType m_solverType;
};

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

class Solver_MonoalphaSub : public SolverTemplate
{
    public:
        Solver_MonoalphaSub() : SolverTemplate(CT_MONOALPHABETIC_SUB_GA) { };

        bool Initialize();
        void Solve();

    protected:
        struct Chromosome
        {
            char alphabet[26];
            float fitness;
            bool changed;
        };

        void RecalculateFitness();
        void RecalculateFitnessOn(Chromosome* gen);
        void PerformCrossover();
        void PerformMutation();
        void ElitismStore();
        void ElitismRestore();
        void PerformWordMapping();
        void PerformCrossoverOn(Chromosome* src, Chromosome* dst);
        void PerformMutationOn(Chromosome* dst);
        void PerformVowelPermutationOn(Chromosome* dst);
        void PerformWordMappingOn(Chromosome* dst);

    private:
        Chromosome** m_population;
        Chromosome* m_elite;
        Chromosome* m_original;
        uint32_t m_populationSize;

        Dictionary m_cipherDictionary;
};

class SolverManager
{
    friend class Singleton<SolverManager>;
    public:
        void Init();

        void JoinThread();

        void SetObtainedWork(CipherType ct);

    protected:
        SolverManager();
        void Run();

        SolverTemplate* CreateSolver(CipherType ct);

    private:
        std::thread *m_solverThread;
        CipherType m_myWork;
};

#define sSolverManager Singleton<SolverManager>::getInstance()

#endif
