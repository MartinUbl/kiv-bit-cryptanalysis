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

void DataLoader::LoadWords()
{
    string wordFileName = m_languageFile + ".stripped";
    string freqFileName = m_languageFile + ".freq";
    char buffer[256];
    int len;

    FILE* f = fopen(wordFileName.c_str(), "r");
    if (!f)
    {
        cerr << "Unable to load dictionary" << endl;
    }
    else
    {
        while (fgets(buffer, 256, f))
        {
            len = strlen(buffer) - 1;
            while (buffer[len] == '\n' || buffer[len] == '\r')
                buffer[len--] = '\0';
            m_words.push_back(buffer);
        }

        fclose(f);
    }

    f = fopen(freqFileName.c_str(), "r");
    if (!f)
    {
        cerr << "Unable to load frequency map" << endl;
    }
    else
    {
        int i = 0;
        while (fgets(buffer, 256, f))
        {
            if (i >= ALPHABET_SIZE)
            {
                cerr << "Frequency map contains more than " << ALPHABET_SIZE << " entries" << endl;
                break;
            }
            strtok(buffer, ";");
            strtok(nullptr, ";");
            char* freq = strtok(nullptr, "\n");
            m_frequencies[i] = atof(freq);
            i++;
        }
    }
}

double* DataLoader::GetFrequencyMap()
{
    return m_frequencies;
}

std::vector<string>* DataLoader::GetDictionary()
{
    return &m_words;
}
