#include "general.h"
#include "Solver.h"

static int ClipChar(int inp)
{
    while (inp < 'a')
        inp += 26;
    while (inp > 'z')
        inp -= 26;
    return inp;
}

bool Solver_Caesar::Initialize()
{
    return true;
}

void Solver_Caesar::Solve()
{
    const char* msg = m_message.c_str();
    size_t len = m_message.length();
    char* tmp = new char[len+1];
    tmp[len] = '\0';

    // bruteforce all possible moves
    for (int i = 0; i < 25; i++)
    {
        for (size_t j = 0; j < len; j++)
        {
            if (msg[j] >= 'a' && msg[j] <= 'z')
                tmp[j] = ClipChar(msg[j] + i);
            else
                tmp[j] = msg[j];
        }

        ResultScore sc(tmp);

        AddResult(sc.GetFrequencyScore(), sc.GetDictionaryScore(), tmp);
    }

    delete tmp;
}
