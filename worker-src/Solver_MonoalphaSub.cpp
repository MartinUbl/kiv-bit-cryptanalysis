#include "general.h"
#include "Solver.h"

static char* monoalpha_buffer;

static void monoalphabetic_decrypt(char* alphabet, const char* input, char* output)
{
    std::map<char, char> decrypt_table;
    for (size_t i = 0; i < 26; i++)
        decrypt_table[alphabet[i]] = (char)('a' + i);

    size_t len = strlen(input);
    for (size_t i = 0; i < len; i++)
    {
        if (input[i] >= 'a' && input[i] <= 'z')
            output[i] = decrypt_table[input[i]];
        else
            output[i] = input[i];
    }
}

bool Solver_MonoalphaSub::Initialize()
{
    return true;
}

void Solver_MonoalphaSub::RecalculateFitness()
{
    for (size_t i = 0; i < m_populationSize; i++)
    {
        if (m_population[i]->changed)
            RecalculateFitnessOn(m_population[i]);
    }

    if (m_elite->changed)
        RecalculateFitnessOn(m_elite);
}

void Solver_MonoalphaSub::RecalculateFitnessOn(Chromosome* gen)
{
    monoalphabetic_decrypt(gen->alphabet, m_message.c_str(), monoalpha_buffer);

    float* freqs = sDataHolder->GetFrequencies();
    StringFreqMap* bifreqs = sDataHolder->GetBigramFrequencies();

    const float unigram_coef = 0.2f;
    const float bigram_coef = 0.6f;

    // unigram frequency analysis
    float frqs[26];
    for (size_t i = 0; i < 26; i++)
        frqs[i] = 0.0f;

    uint32_t cnt = 0;
    for (size_t i = 0; i < m_message.length(); i++)
    {
        if (monoalpha_buffer[i] >= 'a' && monoalpha_buffer[i] <= 'z')
        {
            frqs[monoalpha_buffer[i] - 'a'] += 1.0f;
            cnt++;
        }
    }
    for (size_t i = 0; i < 26; i++)
        frqs[i] = frqs[i] / ((float)cnt);

    // bigram frequency analysis
    StringFreqMap bifrqs;

    cnt = 0;
    for (size_t i = 0; i < m_message.length() - 1; i++)
    {
        if (monoalpha_buffer[i] >= 'a' && monoalpha_buffer[i] <= 'z' && monoalpha_buffer[i + 1] >= 'a' && monoalpha_buffer[i + 1] <= 'z')
        {
            std::string tmbi = { monoalpha_buffer[i], monoalpha_buffer[i + 1], '\0' };
            if (bifrqs.find(tmbi) == bifrqs.end())
                bifrqs[tmbi] = 0;

            bifrqs[tmbi] = bifrqs[tmbi] + 1.0f;
            cnt++;
        }
    }
    for (auto &bif : bifrqs)
        bif.second = bif.second / ((float)cnt);

    // count fitness
    float unigram_score = 0.0f;
    for (size_t i = 0; i < 26; i++)
        unigram_score += - freqs[i] + frqs[i];
    //unigram_score *= unigram_coef;

    float bigram_score = 0.0f;
    for (auto &bif : bifrqs)
    {
        if (bifreqs->find(bif.first.c_str()) == bifreqs->end())
            bigram_score += bif.second;
        else
            bigram_score += - (*bifreqs)[bif.first.c_str()] + bif.second;
    }
    //bigram_score *= bigram_coef;

    ResultScore sc(monoalpha_buffer, 220.0f);

    gen->fitness = pow(1 - (unigram_score + bigram_score) / 4, 8) + sc.GetDictionaryScore() / 50.0f;

    gen->changed = false;
}

void Solver_MonoalphaSub::PerformCrossover()
{
    for (size_t i = 0; i < m_populationSize; i++)
    {
        for (size_t j = 0; j < m_populationSize; j++)
        {
            if (rand() % 100 < 8)
            {
                if (m_population[i]->fitness > m_population[j]->fitness)
                {
                    PerformCrossoverOn(m_population[i], m_population[j]);
                    RecalculateFitnessOn(m_population[j]);
                }
                else
                {
                    PerformCrossoverOn(m_population[j], m_population[i]);
                    RecalculateFitnessOn(m_population[i]);
                }
            }
        }
    }
}

void Solver_MonoalphaSub::PerformMutation()
{
    for (size_t i = 0; i < m_populationSize; i++)
    {
        if (rand() % 100 < 7)
            PerformMutationOn(m_population[i]);

        if (rand() % 100 < 3)
            PerformVowelPermutationOn(m_population[i]);
    }
}

void Solver_MonoalphaSub::ElitismStore()
{
    size_t maxpos = 0;
    for (size_t i = 1; i < m_populationSize; i++)
    {
        if (m_population[i]->fitness > m_population[maxpos]->fitness)
            maxpos = i;
    }

    if (m_population[maxpos]->fitness > m_elite->fitness)
        memcpy(m_elite, m_population[maxpos], sizeof(Chromosome));
}

void Solver_MonoalphaSub::ElitismRestore()
{
    size_t minpos = 0;
    for (size_t i = 1; i < m_populationSize; i++)
    {
        if (m_population[i]->fitness < m_population[minpos]->fitness)
            minpos = i;
        // when elite is present, do not restore
        if (strncmp(m_population[i]->alphabet, m_elite->alphabet, 26) == 0)
            return;
    }

    if (m_population[minpos]->fitness < m_elite->fitness)
        memcpy(m_population[minpos], m_elite, sizeof(Chromosome));
}

void Solver_MonoalphaSub::PerformCrossoverOn(Chromosome* src, Chromosome* dst)
{
    std::vector<size_t> diffpos;
    for (size_t i = 0; i < 26; i++)
        if (src->alphabet[i] != dst->alphabet[i])
            diffpos.push_back(i);

    if (diffpos.size() == 0)
        return;

    size_t pos = diffpos[rand() % diffpos.size()];

    char toswap1 = src->alphabet[pos]; // this belongs to dst->alphabet[pos] later
    char toswap2 = dst->alphabet[pos]; // this belongs to position, where toswap1 was in dst->alphabet

    for (size_t i = 0; i < 26; i++)
    {
        if (dst->alphabet[i] == toswap1)
        {
            dst->alphabet[i] = toswap1;
            dst->alphabet[pos] = toswap2;
            dst->changed = true;
            break;
        }
    }
}

void Solver_MonoalphaSub::PerformMutationOn(Chromosome* dst)
{
    // TODO: better mutation scheme
    size_t pos1 = rand() % 26;
    size_t pos2;

    do
    {
        pos2 = rand() % 26;
    } while (pos1 == pos2);

    char tmp = dst->alphabet[pos1];
    dst->alphabet[pos1] = dst->alphabet[pos2];
    dst->alphabet[pos2] = tmp;

    dst->changed = true;
}

void Solver_MonoalphaSub::PerformVowelPermutationOn(Chromosome* dst)
{
    const char* vowelmap = "aeiyou";
    int pos1 = rand() % strlen(vowelmap);
    int pos2;
    
    do
    {
        pos2 = rand() % strlen(vowelmap);
    } while (pos2 == pos1);

    pos1 = vowelmap[pos1] - 'a';
    pos2 = vowelmap[pos2] - 'a';

    char tmp = dst->alphabet[pos1];
    dst->alphabet[pos1] = dst->alphabet[pos2];
    dst->alphabet[pos2] = tmp;
}

void Solver_MonoalphaSub::PerformWordMapping()
{
    size_t minpos = 0;
    for (size_t i = 1; i < m_populationSize; i++)
    {
        if (m_population[i]->fitness < m_population[minpos]->fitness)
            minpos = i;
        // when elite is present, do not restore
        if (strncmp(m_population[i]->alphabet, m_elite->alphabet, 26) == 0)
            return;
    }

    for (int i = 0; i < 2; i++)
        PerformWordMappingOn(m_population[minpos]);
}

void Solver_MonoalphaSub::PerformWordMappingOn(Chromosome* dst)
{
    Dictionary* dict = sDataHolder->GetDictionary();

    std::string* randword;

    do
    {
        randword = &m_cipherDictionary[rand() % m_cipherDictionary.size()];
    } while (randword->size() < 4 || randword->size() > 10); // 4-10 letters

    size_t const& len = randword->size();

    int last = 0;
    std::map<char, int> lettermap_cipher;
    std::vector<int> lettervector_cipher;
    lettervector_cipher.resize(len);

    const char* wptr = randword->c_str();
    for (size_t a = 0; a < randword->size(); a++)
    {
        if (lettermap_cipher.find(wptr[a]) == lettermap_cipher.end())
            lettermap_cipher[wptr[a]] = ++last;

        lettervector_cipher[a] = lettermap_cipher[wptr[a]];
    }

    // find appropriate words in dictionary

    std::vector<std::string> suitableWords;

    std::map<char, int> lettermap_real;
    std::vector<int> lettervector_real;
    lettervector_real.resize(len);
    bool found;

    for (std::string& wrd : *dict)
    {
        if (wrd.size() != len)
            continue;

        last = 0;
        lettermap_real.clear();
        lettervector_real.clear();
        lettervector_real.resize(len);

        const char* wptr = wrd.c_str();
        for (size_t a = 0; a < wrd.size(); a++)
        {
            if (lettermap_real.find(wptr[a]) == lettermap_real.end())
                lettermap_real[wptr[a]] = ++last;

            lettervector_real[a] = lettermap_real[wptr[a]];
        }

        found = true;
        for (size_t a = 0; a < len; a++)
        {
            if (lettervector_cipher[a] != lettervector_real[a])
            {
                found = false;
                break;
            }
        }

        if (!found)
            continue;

        suitableWords.push_back(wrd);
    }

    if (suitableWords.size() == 0)
        return;

    size_t chosen = rand() % suitableWords.size();
    const char* chptr = suitableWords[chosen].c_str();
    wptr = randword->c_str();

    for (size_t a = 0; a < len; a++)
    {
        dst->alphabet[chptr[a] - 'a'] = wptr[a];
    }

    dst->changed = true;

    //std::cout << "Mapped " << chptr << " to " << wptr << ", alphabet: " << std::string(dst->alphabet, 26).c_str() << endl;
}

void Solver_MonoalphaSub::Solve()
{
    m_populationSize = 25;

    float originalFreqs[26];
    memcpy(originalFreqs, sDataHolder->GetFrequencies(), 26*sizeof(float));

    float myFreqs[26];
    for (size_t i = 0; i < 26; i++)
        myFreqs[i] = 0;

    const char* msgptr = m_message.c_str();
    size_t len = m_message.length();

    monoalpha_buffer = new char[len+1];
    monoalpha_buffer[len] = '\0';

    uint32_t count = 0;

    for (size_t i = 0; i < len; i++)
    {
        if (msgptr[i] >= 'a' && msgptr[i] <= 'z')
        {
            count++;
            myFreqs[msgptr[i] - 'a'] += 1.0f;
        }
    }

    for (size_t i = 0; i < 26; i++)
        myFreqs[i] = myFreqs[i] / (float)count;

    uint8_t expectPositions[26];
    uint8_t currPositions[26];

    size_t pos;
    float max;
    for (size_t i = 0; i < 26; i++)
    {
        max = -1.0f;
        pos = 0;
        for (size_t j = 0; j < 26; j++)
        {
            if (originalFreqs[j] > max)
            {
                max = originalFreqs[j];
                pos = j;
            }
        }
        expectPositions[pos] = (uint8_t)i;
        originalFreqs[pos] = -1.0f;

        max = -1.0f;
        pos = 0;
        for (size_t j = 0; j < 26; j++)
        {
            if (myFreqs[j] > max)
            {
                max = myFreqs[j];
                pos = j;
            }
        }
        currPositions[pos] = (uint8_t)i;
        myFreqs[pos] = -1.0f;
    }

    char expectAlphabet[26];
    for (size_t i = 0; i < 26; i++)
    {
        for (size_t j = 0; j < 26; j++)
        {
            if (currPositions[j] == expectPositions[i])
            {
                expectAlphabet[i] = (char)('a' + j);
                break;
            }
        }
    }

    // analyse dictionary

    m_cipherDictionary.clear();

    char const* lastptr = msgptr;
    int wordlen = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (msgptr[i] >= 'a' && msgptr[i] <= 'z')
        {
            wordlen++;
        }
        else
        {
            if (msgptr[i] == ' ' && wordlen > 0)
            {
                m_cipherDictionary.push_back(std::string(lastptr, wordlen));
            }
            lastptr = &msgptr[i+1];
            wordlen = 0;
        }
    }

    // create initial population

    m_original = new Chromosome;
    memcpy(m_original->alphabet, expectAlphabet, 26 * sizeof(char));
    RecalculateFitnessOn(m_original);

    m_elite = new Chromosome;
    memcpy(m_elite, m_original, sizeof(Chromosome));

    m_population = new Chromosome*[m_populationSize];
    for (size_t i = 0; i < m_populationSize; i++)
    {
        m_population[i] = new Chromosome;
        memcpy(m_population[i]->alphabet, expectAlphabet, 26 * sizeof(char));
        m_population[i]->changed = true;
    }

    for (size_t i = 0; i < m_populationSize; i++)
    {
        for (int a = 0; a < 5; a++)
            PerformMutationOn(m_population[i]);
        m_population[i]->changed = true;
    }

    /*memcpy(m_population[0]->alphabet, "utxbmfiglvdzjokwncpqrasyeh", 26);
    m_population[0]->changed = true;*/

    RecalculateFitness();

    /////////////////////////////////////////

    int generation_count = 10000;

    for (int i = 0; i < generation_count; i++)
    {
        ElitismStore();
        PerformCrossover();
        PerformMutation();

        RecalculateFitness();

        if (i % 15 == 0)
            PerformWordMapping();

        RecalculateFitness();

        if ((i+1) % 100 == 0)
            ElitismRestore();

        cout << "Generation " << (i + 1) << "; Elite: " << m_elite->fitness << ", alphabet: " << std::string(m_elite->alphabet, 26).c_str() << endl;
    }

    monoalphabetic_decrypt(m_elite->alphabet, m_message.c_str(), monoalpha_buffer);

    ResultScore rs(monoalpha_buffer);

    AddResult(rs.GetFrequencyScore(), rs.GetDictionaryScore(), monoalpha_buffer);
}
