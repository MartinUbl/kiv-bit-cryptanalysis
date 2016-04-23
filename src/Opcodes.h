#ifndef KIV_BIT_CRYPTOANALYSIS_OPCODES_H
#define KIV_BIT_CRYPTOANALYSIS_OPCODES_H

// CP = client packet (client to server)
// SP = server packet (server to client)
// GP = general packet (both ways, or none)

enum Opcodes
{
    GP_NONE             = 0x00,
    CP_HELLO            = 0x01,
    SP_HELLO_RESPONSE   = 0x02,
    CP_GET_FREQ_MAP     = 0x03,
    SP_FREQ_MAP         = 0x04,
    CP_GET_DICTIONARY   = 0x05,
    SP_DICTIONARY_WORDS = 0x06,
    SP_END_OF_DICTIONARY = 0x07,
    CP_GET_ENC_MESSAGE  = 0x08,
    SP_ENC_MESSAGE      = 0x09,
    CP_GET_WORK         = 0x0A,
    SP_GIVE_WORK        = 0x0B,
    CP_SUBMIT_RESULT    = 0x0C,
    SP_RESULT_ACCEPT    = 0x0D,

    MAX_OPCODE
};

#endif
