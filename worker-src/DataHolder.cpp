#include "general.h"
#include "Solver.h"

DataHolder::DataHolder()
{
    //
}

void DataHolder::SetFrequencies(uint32_t alphabet_len, float* freqs)
{
    // store to internal fields
    m_frequencies = new float[alphabet_len];
    m_alphabetLength = alphabet_len;

    for (uint32_t i = 0; i < alphabet_len; i++)
        m_frequencies[i] = freqs[i];
}

void DataHolder::AddWord(string word)
{
    m_words.push_back(word);
}

void DataHolder::AddBigram(const char* bigram, float freq)
{
    m_bigramFrequencies[bigram] = freq;
}

uint32_t DataHolder::GetAlphabetLength()
{
    return m_alphabetLength;
}

float* DataHolder::GetFrequencies()
{
    return m_frequencies;
}

Dictionary* DataHolder::GetDictionary()
{
    return &m_words;
}

void DataHolder::SetEncryptedMessage(std::string str)
{
    m_origMessage = str;
}

const char* DataHolder::GetEncryptedMessage()
{
    return m_origMessage.c_str();
}

StringFreqMap* DataHolder::GetBigramFrequencies()
{
    return &m_bigramFrequencies;
}
