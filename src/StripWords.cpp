#include "general.h"
#include "StripWords.h"

map<int, int> characterConversionMap;

void FillConversionMap()
{
    map<int, int> &ccm = characterConversionMap;

    ccm['ì'] = 'e'; ccm['é'] = 'e'; ccm['á'] = 'a'; ccm['š'] = 's'; ccm['è'] = 'c'; ccm['ø'] = 'r';
    ccm['ž'] = 'z'; ccm['ý'] = 'y'; ccm['í'] = 'i'; ccm['ó'] = 'o'; ccm['ù'] = 'u';
    ccm['ú'] = 'u'; ccm['ò'] = 'n'; ccm[''] = 't'; ccm['ï'] = 'd'; ccm['ò'] = 'n';
    ccm['Ì'] = 'e'; ccm['É'] = 'e'; ccm['Á'] = 'a'; ccm['Š'] = 's'; ccm['È'] = 'c'; ccm['Ø'] = 'r';
    ccm['Ž'] = 'z'; ccm['Ý'] = 'y'; ccm['Í'] = 'i'; ccm['Ó'] = 'o'; ccm['Ù'] = 'u';
    ccm['Ú'] = 'u'; ccm['Ò'] = 'n'; ccm[''] = 't'; ccm['Ï'] = 'd'; ccm['Ò'] = 'n';
}

char StripChar(char chr)
{
    if (chr >= 'a' && chr <= 'z')
        return chr;
    if (chr >= 'A' && chr <= 'Z')
        return (chr - 'A') + 'a';

    if (characterConversionMap.find(chr) != characterConversionMap.end())
        return characterConversionMap[chr];
    else
        cout << "Cannot find char " << (char)chr << endl;

    return '\0';
}

int StripInputWords(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << "Invalid parameter count for 'strip' mode" << endl;
        cout << "Usage: ./" << argv[0] << " strip <filename>" << endl;
        return 1;
    }

    FILE* f = fopen(argv[2], "r");
    if (!f)
    {
        cerr << "Could not open file " << argv[2] << endl;
        return 2;
    }

    FillConversionMap();

    list<string> words;

    int cursor = 0;
    char buffer[64];
    int tmp;

    while (!feof(f))
    {
        tmp = fgetc(f);

        if (tmp == ' ' || tmp == '\r' || tmp == '\n' || tmp == '\0' || cursor == 63)
        {
            buffer[cursor] = '\0';
            if (cursor > 0)
                words.push_back(buffer);
            cursor = 0;

            continue;
        }

        tmp = StripChar(tmp);
        if (tmp == '\0')
            continue;

        buffer[cursor++] = tmp;
    }

    fclose(f);

    string outfilename = argv[2];
    outfilename += ".stripped";

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

    for (i = 0; i < 26; i++)
        frequency[i] = 0;

    for (string& w : words)
    {
        fprintf(f, "%s\n", w.c_str());

        for (i = 0; i < w.size(); i++)
            frequency[w[i] - 'a']++;

        for (i = 0; i < w.size() - 1; i++)
        {
            tmpbi = ((char)w[i]);
            tmpbi |= ((char)w[i+1]) << 8;

            if (bigramFrequency.find(tmpbi) == bigramFrequency.end())
                bigramFrequency[tmpbi] = 0;
            bigramFrequency[tmpbi] = bigramFrequency[tmpbi] + 1;
        }
    }

    fclose(f);

    unsigned int sumf = 0;
    for (i = 0; i < 26; i++)
        sumf += frequency[i];
    unsigned int sumbif = 0;
    for (auto &fr : bigramFrequency)
        sumbif += fr.second;

    float pctfrequency[26];

    for (i = 0; i < 26; i++)
        pctfrequency[i] = ((float)frequency[i]) / ((float)sumf);

    outfilename = argv[2];
    outfilename += ".freq";

    f = fopen(outfilename.c_str(), "w");

    if (!f)
    {
        cerr << "Could not open frequency output file " << outfilename.c_str() << endl;
        return 4;
    }

    for (i = 0; i < 26; i++)
        fprintf(f, "%c;%u;%f\n", 'a'+i, frequency[i], pctfrequency[i]);

    fclose(f);

    ///////

    outfilename = argv[2];
    outfilename += ".freq2";

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
