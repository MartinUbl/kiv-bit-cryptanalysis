#include "general.h"
#include "StripWords.h"
#include "SolveManager.h"

#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
    int res = 0;

    if (argc < 2)
    {
        cout << "Usage: ./" << argv[0] << " <command> [specific parameters]" << endl;
        return 1;
    }

    if (strcmp(argv[1], "strip") == 0)
    {
        res = StripInputWords(argc, argv);
    }
    else if (strcmp(argv[1], "solve") == 0)
    {
        res = sSolveManager->Run(argc, argv);
    }
    else
    {
        cerr << "Unknown command: " << argv[1] << endl;
        res = 2;
    }

    return res;
}
