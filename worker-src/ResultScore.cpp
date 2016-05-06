#include "general.h"
#include "Solver.h"

static float* origfreqs = nullptr;
static int16_t expect_positions[26];

ResultScore::ResultScore(string message, float freqScoreThreshold)
{
    m_message = message;

    // at first, calculate frequency score
    CalculateFrequencyScore();
    // if the score is high enough, calculate dictionary score
    if (GetFrequencyScore() > freqScoreThreshold)
        CalculateDictionaryScore();
    else
        m_dictionaryScore = 0.0f;
}

void ResultScore::CalculateFrequencyScore()
{
    // get frequencies of unigrams in message
    float* freqs = CalculateFrequencies();

    // if the original wasn't calculated yet,..
    bool calcorig = (origfreqs == nullptr);

    if (calcorig)
    {
        origfreqs = new float[26];
        memcpy(origfreqs, sDataHolder->GetFrequencies(), 26 * sizeof(float));
    }

    // calculate expected and real frequency positions (rank)
    int16_t real_positions[26];
    for (size_t i = 0; i < 26; i++)
    {
        float max = -1.0;
        size_t pos;

        if (calcorig)
        {
            // select maximum and rank it
            for (size_t j = 0; j < 26; j++)
            {
                if (origfreqs[j] > max)
                {
                    pos = j;
                    max = origfreqs[j];
                }
            }

            // rank it and neutralize that letter position
            expect_positions[pos] = (int16_t)i;
            origfreqs[pos] = -2.0;
        }

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

    // sum "distances" between expected and real positions for each letter
    int16_t distsum = 0;
    for (size_t i = 0; i < 26; i++)
    {
        if (real_positions[i] != -1)
            distsum += abs(real_positions[i] - expect_positions[i]);
    }

    // rate frequency score as inverse
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

    // this parses all words in message
    char const* laststart = msg;
    int wlen = 0;
    for (size_t i = 0; i < len; i++)
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

    // this attempts to find each word in dictionary
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
    float* res = new float[26]; /* hardcoded for now; freed later in CalculateFrequencyScore */
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
