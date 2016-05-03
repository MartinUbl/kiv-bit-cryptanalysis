#ifndef KIV_BIT_CRYPTOANALYSIS_SOLVEENUMS_H
#define KIV_BIT_CRYPTOANALYSIS_SOLVEENUMS_H

enum CipherType
{
    CT_NONE = 0,
    CT_ATBAS = 1,                           // atbas cipher (a=z, b=y, ..)
    CT_CAESAR = 2,                          // caesar cipher (move alphabet by N)
    CT_VIGENERE = 3,                        // Vigenere cipher (polyalphabetic with key determining alphabet offset)
    CT_MONOALPHABETIC_SUB_GA = 4,           // monoalphabetic substitution (1:1 cipher alphabet) via genetic algorithm

    // not yet supported:
    CT_BIALPHABETIC_SUB = 5,                // 2 alphabets, one for even characters, one for odd

    MAX_CIPHER_TYPE,
    MIN_CIPHER_TYPE = CT_ATBAS
};

#define WORK_OK 0
#define WORK_NONE 1

#endif
