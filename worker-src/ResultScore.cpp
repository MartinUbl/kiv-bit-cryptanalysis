#include "general.h"
#include "Solver.h"

ResultScore::ResultScore(string message, float freqScoreThreshold)
{
    m_message = message;

    CalculateFrequencyScore();
    if (GetFrequencyScore() > freqScoreThreshold)
        CalculateDictionaryScore();
    else
        m_dictionaryScore = 0.0f;
}

void ResultScore::CalculateFrequencyScore()
{
    float* freqs = CalculateFrequencies();
    float* origfreqs = new float[26];
    memcpy(origfreqs, sDataHolder->GetFrequencies(), 26 * sizeof(float));

    int16_t expect_positions[26];
    int16_t real_positions[26];
    for (size_t i = 0; i < 26; i++)
    {
        float max = -1.0;
        size_t pos;
        for (size_t j = 0; j < 26; j++)
        {
            if (origfreqs[j] > max)
            {
                pos = j;
                max = origfreqs[j];
            }
        }

        expect_positions[pos] = (int16_t)i;
        origfreqs[pos] = -2.0;

        max = -1.0;
        for (size_t j = 0; j < 26; j++)
        {
            if (freqs[j] > max)
            {
                pos = j;
                max = freqs[j];
            }
        }

        if (max < FLT_EPSILON) // " == 0 " - exclude not-found characters
            real_positions[pos] = -1;
        else
            real_positions[pos] = (int16_t)i;
        freqs[pos] = -2.0;
    }

    // 2*(25 + 23 + 21 + 19 + 17 + 15 + 13 + 11 + 9 + 7 + 5 + 3 + 1) = 2*169 = 338 = maximum sum of distances

    int16_t distsum = 0;
    for (size_t i = 0; i < 26; i++)
    {
        if (real_positions[i] != -1)
            distsum += abs(real_positions[i] - expect_positions[i]);
    }

    m_frequencyScore = (float)(338 - distsum);

    delete freqs;
}

void ResultScore::CalculateDictionaryScore()
{
    size_t len = m_message.length();
    const char* msg = m_message.c_str();
    m_dictionaryScore = 0.0f;

    Dictionary* dict = sDataHolder->GetDictionary();
    std::set<string> words;

    char const* laststart = msg;
    int wlen = 0;
    for (int i = 0; i < len; i++)
    {
        if (msg[i] == ' ')
        {
            if (wlen > 0)
            {
                words.insert(string(laststart, wlen));
                wlen = 0;
            }
            laststart = &msg[i+1];
        }
        else
            wlen++;
    }

    for (string& str : *dict)
    {
        if (words.find(str) != words.end())
            m_dictionaryScore += str.length();
    }
}

float ResultScore::GetFrequencyScore()
{
    return m_frequencyScore;
}

float ResultScore::GetDictionaryScore()
{
    return m_dictionaryScore;
}

float* ResultScore::CalculateFrequencies()
{
    float* res = new float[26]; /* hardcoded for now */
    for (size_t i = 0; i < 26; i++)
        res[i] = 0.0;

    const char* contents = m_message.c_str();

    for (size_t i = 0; i < m_message.length(); i++)
    {
        if (contents[i] >= 'a' && contents[i] <= 'z')
            res[contents[i] - 'a'] += 1.0;
    }

    for (size_t i = 0; i < 26; i++)
        res[i] /= m_message.length();

    return res;
}
