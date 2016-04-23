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

    unsigned int frequency[26];
    size_t i;

    for (i = 0; i < 26; i++)
        frequency[i] = 0;

    for (string& w : words)
    {
        fprintf(f, "%s\n", w.c_str());

        for (i = 0; i < w.size(); i++)
            frequency[w[i] - 'a']++;
    }

    fclose(f);

    unsigned int sumf = 0;
    for (i = 0; i < 26; i++)
        sumf += frequency[i];

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

    cout << "Dictionary " << argv[2] << " successfuly parsed" << endl;

    return 0;
}
