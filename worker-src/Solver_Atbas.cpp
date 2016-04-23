#include "general.h"
#include "Solver.h"

bool Solver_Atbas::Initialize()
{
    return true;
}

void Solver_Atbas::Solve()
{
    const char* msgptr = m_message.c_str();
    size_t len = m_message.length();

    char* result = new char[len + 1];
    result[len] = '\0';

    for (size_t i = 0; i < len; i++)
    {
        if (msgptr[i] >= 'a' && msgptr[i] <= 'z')
            result[i] = 'a' + 26 - (msgptr[i] - 'a' + 1);
        else
            result[i] = msgptr[i];
    }

    ResultScore sc(result);

    AddResult(sc.GetFrequencyScore(), sc.GetDictionaryScore(), result);
}
