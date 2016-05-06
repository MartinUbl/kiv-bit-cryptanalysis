#include "general.h"
#include "StripWords.h"

#define WORD_BUFFER_SIZE 64

map<int, int> characterConversionMap;

void FillConversionMap()
{
    map<int, int> &ccm = characterConversionMap;

    // ignore following few lines and act like this method is written properly, multi-encoding
    // compatible and in very fancy and optimal way
    ccm['ì'] = 'e'; ccm['é'] = 'e'; ccm['á'] = 'a'; ccm['š'] = 's'; ccm['è'] = 'c'; ccm['ø'] = 'r';
    ccm['ž'] = 'z'; ccm['ý'] = 'y'; ccm['í'] = 'i'; ccm['ó'] = 'o'; ccm['ù'] = 'u';
    ccm['ú'] = 'u'; ccm['ò'] = 'n'; ccm[''] = 't'; ccm['ï'] = 'd'; ccm['ò'] = 'n';
    ccm['Ì'] = 'e'; ccm['É'] = 'e'; ccm['Á'] = 'a'; ccm['Š'] = 's'; ccm['È'] = 'c'; ccm['Ø'] = 'r';
    ccm['Ž'] = 'z'; ccm['Ý'] = 'y'; ccm['Í'] = 'i'; ccm['Ó'] = 'o'; ccm['Ù'] = 'u';
    ccm['Ú'] = 'u'; ccm['Ò'] = 'n'; ccm[''] = 't'; ccm['Ï'] = 'd'; ccm['Ò'] = 'n';
}

// strips single character
char StripChar(char chr)
{
    // desired range is already fulfilled
    if (chr >= 'a' && chr <= 'z')
        return chr;
    // uppercase range - convert to lowercase
    if (chr >= 'A' && chr <= 'Z')
        return (chr - 'A') + 'a';

    // find character in conversion map
    if (characterConversionMap.find(chr) != characterConversionMap.end())
        return characterConversionMap[chr];
    else
        cout << "Cannot find char " << (char)chr << endl;

    return '\0';
}

int StripInputWords(int argc, char** argv)
{
    // secure character count
    if (argc != 3)
    {
        cerr << "Invalid parameter count for 'strip' mode" << endl;
        cout << "Usage: ./" << argv[0] << " strip <filename>" << endl;
        return 1;
    }

    // open input file
    FILE* f = fopen(argv[2], "r");
    if (!f)
    {
        cerr << "Could not open file " << argv[2] << endl;
        return 2;
    }

    // prepare conversion map
    FillConversionMap();

    list<string> words;

    int cursor = 0;
    char buffer[WORD_BUFFER_SIZE];
    int tmp;

    // read while there's something to read
    while (!feof(f))
    {
        tmp = fgetc(f);

        // read until we reach space, CR, LF, zero or end of buffer
        if (tmp == ' ' || tmp == '\r' || tmp == '\n' || tmp == '\0' || cursor == WORD_BUFFER_SIZE - 1)
        {
            buffer[cursor] = '\0';
            if (cursor > 0)
                words.push_back(buffer);
            cursor = 0;

            continue;
        }

        // strip read char
        tmp = StripChar(tmp);
        if (tmp == '\0')
            continue;

        buffer[cursor++] = tmp;
    }

    fclose(f);

    string outfilename = argv[2];
    outfilename += ".stripped";
    // open output file for parsed dictionary
    f = fopen(outfilename.c_str(), "w");

    if (!f)
    {
        cerr << "Could not open output file " << outfilename.c_str() << endl;
        return 3;
    }

    std::map< uint16_t, int > bigramFrequency;
    unsigned int frequency[26];
    size_t i;
    uint16_t tmpbi;

    // analyse unigram frequency
    for (i = 0; i < 26; i++)
        frequency[i] = 0;

    // go through all words...
    for (string& w : words)
    {
        fprintf(f, "%s\n", w.c_str());

        // count unigrams
        for (i = 0; i < w.size(); i++)
            frequency[w[i] - 'a']++;

        // and count bigrams
        for (i = 0; i < w.size() - 1; i++)
        {
            // store character pair in one 16bit field
            tmpbi = ((char)w[i]);
            tmpbi |= ((char)w[i+1]) << 8;

            // increase bigram frequency
            if (bigramFrequency.find(tmpbi) == bigramFrequency.end())
                bigramFrequency[tmpbi] = 0;
            bigramFrequency[tmpbi] = bigramFrequency[tmpbi] + 1;
        }
    }

    fclose(f);

    // sum frequency
    unsigned int sumf = 0;
    for (i = 0; i < 26; i++)
        sumf += frequency[i];
    // sum bigram frequency
    unsigned int sumbif = 0;
    for (auto &fr : bigramFrequency)
        sumbif += fr.second;

    float pctfrequency[26];

    // calculate frequency in percentages
    for (i = 0; i < 26; i++)
        pctfrequency[i] = ((float)frequency[i]) / ((float)sumf);

    outfilename = argv[2];
    outfilename += ".freq";

    // open frequency map file
    f = fopen(outfilename.c_str(), "w");

    if (!f)
    {
        cerr << "Could not open frequency output file " << outfilename.c_str() << endl;
        return 4;
    }

    // store frequencies
    for (i = 0; i < 26; i++)
        fprintf(f, "%c;%u;%f\n", 'a'+i, frequency[i], pctfrequency[i]);

    fclose(f);

    ///////

    outfilename = argv[2];
    outfilename += ".freq2";

    // open bigram frequency output file
    f = fopen(outfilename.c_str(), "w");

    if (!f)
    {
        cerr << "Could not open bigram frequency output file " << outfilename.c_str() << endl;
        return 4;
    }

    char tmpbi2[3];
    tmpbi2[2] = '\0';
    for (auto &fr : bigramFrequency)
    {
        float relfr = ((float)fr.second) / ((float)sumbif);
        if (relfr > 0.001f)
        {
            strncpy(tmpbi2, (char*)&fr.first, 2);
            fprintf(f, "%s;%u;%f\n", tmpbi2, fr.second, relfr);
        }
    }

    fclose(f);

    cout << "Dictionary " << argv[2] << " successfuly parsed" << endl;

    return 0;
}
