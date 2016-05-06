#include "general.h"
#include "StripWords.h"
#include "SolveManager.h"

#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
    int res = 0;

    // print banner
    cout << "KIV/BIT, Cryptanalysis - master" << endl;
    cout << "Author: Martin Ubl (A13B0453P)" << endl << endl;

    // secure argument count
    if (argc < 2)
    {
        cout << "Usage: ./" << argv[0] << " <command> [specific parameters]" << endl;
        cout << "Available commands: strip (parse dictionary and frequencies), solve (solving mode)" << endl << endl;
        return 1;
    }

    // word stripping
    if (strcmp(argv[1], "strip") == 0)
    {
        res = StripInputWords(argc, argv);
    }
    // solving
    else if (strcmp(argv[1], "solve") == 0)
    {
        res = sSolveManager->Run(argc, argv);
    }
    // no such command
    else
    {
        cerr << "Unknown command: " << argv[1] << endl;
        cout << "Available commands: strip (parse dictionary and frequencies), solve (solving mode)" << endl << endl;
        res = 2;
    }

    return res;
}
