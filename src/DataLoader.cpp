#include "general.h"
#include "SolveManager.h"

DataLoader::DataLoader()
{
    //
}

void DataLoader::SetLanguageFile(const char* langFile)
{
    m_languageFile = langFile;
}

bool DataLoader::LoadWords()
{
    // prepare filenames
    string wordFileName = m_languageFile + ".stripped";
    string freqFileName = m_languageFile + ".freq";
    string bigramfreqFileName = m_languageFile + ".freq2";
    char buffer[256];
    int len;

    // open dictionary
    FILE* f = fopen(wordFileName.c_str(), "r");
    if (!f)
    {
        cerr << "Unable to load dictionary (" << wordFileName.c_str() << ")" << endl;
        return false;
    }
    else
    {
        // read lines
        while (fgets(buffer, 256, f))
        {
            len = strlen(buffer) - 1;
            // cut line endings
            while (buffer[len] == '\n' || buffer[len] == '\r')
                buffer[len--] = '\0';
            m_words.push_back(buffer);
        }

        fclose(f);
    }

    // open frequency map file
    f = fopen(freqFileName.c_str(), "r");
    if (!f)
    {
        cerr << "Unable to load frequency map (" << freqFileName.c_str() << ")" << endl;
        return false;
    }
    else
    {
        // parse all unigrams
        int i = 0;
        while (fgets(buffer, 256, f))
        {
            // secure length
            if (i >= ALPHABET_SIZE)
            {
                cerr << "Frequency map contains more than " << ALPHABET_SIZE << " entries" << endl;
                break;
            }

            // tokenize the string
            strtok(buffer, ";");
            strtok(nullptr, ";");
            char* freq = strtok(nullptr, "\n");
            m_frequencies[i] = atof(freq);
            i++;
        }

        fclose(f);
    }

    // load bigram frequency map file
    f = fopen(bigramfreqFileName.c_str(), "r");
    if (!f)
    {
        cerr << "Unable to load bigram frequency map (" << bigramfreqFileName.c_str() << ")" << endl;
        return false;
    }
    else
    {
        // read lines
        while (fgets(buffer, 256, f))
        {
            // tokenize and store
            char* bigram = strtok(buffer, ";");
            strtok(nullptr, ";");
            char* freq = strtok(nullptr, "\n");

            m_bigramFrequencies[bigram] = (float)atof(freq);
        }

        fclose(f);
    }

    return true;
}

double* DataLoader::GetFrequencyMap()
{
    return m_frequencies;
}

std::vector<string>* DataLoader::GetDictionary()
{
    return &m_words;
}

std::map<string, float>* DataLoader::GetBigramFrequencyMap()
{
    return &m_bigramFrequencies;
}
