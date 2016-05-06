#include "general.h"
#include "Solver.h"

#include <thread>
#include <mutex>

const char* vigenere_table = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
static const int topcharcount = 6;
static int topchars[topcharcount];
static char* keyBase;

std::mutex resf_lock;
void vigenere_threadfunc(int fdiff, int guesskeylen, const char* msg, std::list<SolverResult>* resf);

static int ClipChar(int inp)
{
    while (inp < 'a')
        inp += 26;
    while (inp > 'z')
        inp -= 26;
    return inp;
}

static void vigenere_reverse(const char* input, char* target, const char* key)
{
    int len = strlen(input);
    int klen = strlen(key);
    int keypos = 0;

    for (int i = 0; i < len; i++)
    {
        if (input[i] >= 'a' && input[i] <= 'z')
        {
            int index = -(key[keypos % klen] - 'a') + (input[i] - 'a');
            if (index < 0)
                index = 26 + index;
            target[i] = vigenere_table[index];
            keypos++;
        }
        else
            target[i] = input[i];
    }
}

bool Solver_Vigenere::Initialize()
{
    return true;
}

void Solver_Vigenere::Solve()
{
    size_t len = m_message.length();
    const char* msg = m_message.c_str();

    map<string, int> countmap;
    map<string, int> lastfoundmap;

    std::list<int> repetitons;

    int keylen;

    // remove spaces and other non-alphabetical characters
    size_t spaceless_len;
    char* spaceless = new char[len+1];
    int cur = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (msg[i] >= 'a' && msg[i] <= 'z')
            spaceless[cur++] = msg[i];
    }
    for (size_t i = cur; i <= len; i++)
        spaceless[i] = '\0';
    spaceless_len = strlen(spaceless);

    // >= 3 --> sanity,  <= 7 --> because longer keys are less likely to be deciphered correctly
    for (keylen = 7; keylen >= 3; keylen--)
    {
        countmap.clear();
        lastfoundmap.clear();

        // for each letter from the beginning to the end minus N
        for (size_t i = 0; i < spaceless_len - keylen; i++)
        {
            // cut portion of string
            string portion = string(&spaceless[i], keylen);

            // have we found that portion yet?
            if (countmap.find(portion) == countmap.end())
                countmap[portion] = 0;
            else // if yes, remember repetition
                repetitons.push_back(i - lastfoundmap[portion]);

            // increment frequency
            countmap[portion] = countmap[portion] + 1;
            // note last found occurence
            lastfoundmap[portion] = i;
        }
    }

    map<int, int> factors;

    if (repetitons.size() < 10)
        return;

    // for all repetitions... find all factors
    for (auto itr : repetitons)
    {
        if (factors.find(itr) == factors.end())
            factors[itr] = 0;

        for (int i = 2; i < itr; i++)
        {
            if (itr % i == 0)
            {
               
                if (factors.find(i) == factors.end())
                    factors[i] = 0;

                factors[i] = factors[i] + 1;
            }
        }
    }

    if (factors.size() == 0)
        return;

    std::vector<int> keyLens;
    // select 10 most significant factors
    for (int i = 0; i < 10; i++)
    {
        int max = -1;
        int maxi;
        for (auto itr : factors)
        {
            if (itr.second > max)
            {
                max = itr.second;
                maxi = itr.first;
            }
        }

        keyLens.push_back(maxi);
        factors[maxi] = -1;
    }

    int maxthreads = std::thread::hardware_concurrency();
    if (maxthreads == 0)
        maxthreads = 2;

    cout << "Using " << maxthreads << " solver threads" << endl;

    for (int guesskeylen : keyLens)
    {
        // skip >10 character keys
        if (guesskeylen > 10)
            continue;

        cout << "Processing key length: " << guesskeylen << endl;

        // preparation for frequency analysis
        int freqs[26];
        int sum, p;
        double relfreq[26];

        // solve base (will be later biased by permutation)
        keyBase = new char[guesskeylen + 1];
        memset(keyBase, 0, guesskeylen + 1);

        // for every character of key...
        for (int pok = 0; pok < guesskeylen; pok++)
        {
            for (int i = 0; i < 26; i++)
                freqs[i] = 0;

            // sum frequencies of every n-th character
            p = 0;
            while (spaceless[p] != '\0')
            {
                if ((p - pok) % guesskeylen == 0)
                {
                    if (spaceless[p] >= 'a' && spaceless[p] <= 'z')
                        freqs[spaceless[p] - 'a']++;
                }
                p++;
            }

            // sum frequencies
            sum = 0;
            for (int i = 0; i < 26; i++)
                sum += freqs[i];

            int mostfreq = -1;

            // calculate relative frequencies and select the most frequent character
            for (int i = 0; i < 26; i++)
            {
                relfreq[i] = (double)freqs[i] / (double)sum;

                if (mostfreq == -1 || relfreq[mostfreq] < relfreq[i])
                    mostfreq = i;
            }

            keyBase[pok] = mostfreq;
        }

        // get frequencies and 
        float* realfreqs = new float[26];
        memcpy(realfreqs, sDataHolder->GetFrequencies(), 26 * sizeof(float));

        for (int i = 0; i < topcharcount; i++)
        {
            float max = -1.0f;
            int maxi;
            for (int j = 0; j < 26; j++)
            {
                if (realfreqs[j] > max)
                {
                    max = realfreqs[j];
                    maxi = j;
                }
            }

            topchars[i] = maxi;
            realfreqs[maxi] = -2.0f;
        }
        delete realfreqs;

        std::list<SolverResult> results;

        std::thread* thrs[topcharcount];

        int activethrs = 0;
        for (int i = 0; i < topcharcount; i++)
        {
            thrs[i] = new std::thread(vigenere_threadfunc, i, guesskeylen, msg, &results);
            activethrs++;

            if (activethrs >= maxthreads)
            {
                for (int j = 0; j < i; j++)
                {
                    if (thrs[i]->joinable())
                        thrs[j]->join();
                }

                activethrs = 0;
            }
        }

        for (int i = 0; i < topcharcount; i++)
        {
            if (thrs[i]->joinable())
                thrs[i]->join();
        }

        for (int i = 0; i < topcharcount; i++)
            delete thrs[i];

        for (SolverResult& fr : results)
            AddResult(fr.freqScore, fr.dictScore, fr.result.c_str());

        // we expect at least 10 results, which is suitable minimum
        if (results.size() > 10)
            break;
    }
}

void vigenere_thread_recur(int level, int guesskeylen, char* finalkey, char* result, const char* msg, std::list<SolverResult>* resf)
{
    for (int kdff = 0; kdff < topcharcount; kdff++)
    {
        finalkey[level] = ClipChar('a' - (('a' + topchars[kdff]) - ('a' + keyBase[level])));

        if (level == 0)
        {
            vigenere_reverse(msg, result, finalkey);

            ResultScore sc(result, 230.0f);
            if (sc.GetFrequencyScore() > 230.0f && sc.GetDictionaryScore() > 80.0f)
            {
                cout << "Possible matching key: " << finalkey << ", freq score: " << sc.GetFrequencyScore() << ", dict score: " << sc.GetDictionaryScore() << endl;

                std::unique_lock<std::mutex> lck(resf_lock);
                resf->push_back({ sc.GetFrequencyScore(), sc.GetDictionaryScore(), result });
            }
        }
        else
            vigenere_thread_recur(level - 1, guesskeylen, finalkey, result, msg, resf);
    }
}

void vigenere_threadfunc(int fdiff, int guesskeylen, const char* msg, std::list<SolverResult>* resf)
{
    char* finalkey = new char[guesskeylen+1];
    finalkey[guesskeylen] = '\0';

    char* result = new char[strlen(msg)+1];
    result[strlen(msg)] = '\0';

    finalkey[guesskeylen - 1] = ClipChar('a' - (('a' + topchars[fdiff]) - ('a' + keyBase[guesskeylen - 1])));
    vigenere_thread_recur(guesskeylen - 2, guesskeylen, finalkey, result, msg, resf);

    delete finalkey;
    delete result;
}
