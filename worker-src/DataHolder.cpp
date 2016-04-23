#include "general.h"
#include "Solver.h"

DataHolder::DataHolder()
{
    //
}

void DataHolder::SetFrequencies(uint32_t alphabet_len, float* freqs)
{
    m_frequencies = new float[alphabet_len];
    m_alphabetLength = alphabet_len;

    for (uint32_t i = 0; i < alphabet_len; i++)
        m_frequencies[i] = freqs[i];
}

void DataHolder::AddWord(string word)
{
    m_words.push_back(word);
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
