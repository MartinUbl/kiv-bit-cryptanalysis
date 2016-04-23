#ifndef KIV_BIT_CRYPTOANALYSIS_CLIENTSOLVER_H
#define KIV_BIT_CRYPTOANALYSIS_CLIENTSOLVER_H

class SmartPacket;

class ClientSolver
{
    public:
        ClientSolver(SOCK mysocket);

        void SendPacket(SmartPacket &pkt);
        void HandlePacket(SmartPacket &pkt);

        void Handle_NULL(SmartPacket& pkt);
        void HandleHello(SmartPacket& pkt);
        void HandleGetFreqMap(SmartPacket& pkt);
        void HandleGetDictionary(SmartPacket& pkt);
        void HandleGetEncryptedMessage(SmartPacket& pkt);
        void HandleGetWork(SmartPacket& pkt);
        void HandleSubmitResult(SmartPacket& pkt);

    protected:
        //

    private:
        SOCK m_mySocket;
};

struct HandlerStruct
{
    // for now just method pointer

    void (ClientSolver::*handler)(SmartPacket&);
};

static HandlerStruct _packetHandlers[] = {
    { &ClientSolver::Handle_NULL }, // GP_NONE
    { &ClientSolver::HandleHello }, // CP_HELLO
    { &ClientSolver::Handle_NULL }, // SP_HELLO_RESPONSE
    { &ClientSolver::HandleGetFreqMap }, // CP_GET_FREQ_MAP
    { &ClientSolver::Handle_NULL }, // SP_FREQ_MAP
    { &ClientSolver::HandleGetDictionary }, // CP_GET_DICTIONARY
    { &ClientSolver::Handle_NULL }, // SP_DICTIONARY_WORDS
    { &ClientSolver::Handle_NULL }, // SP_END_OF_DICTIONARY
    { &ClientSolver::HandleGetEncryptedMessage }, // CP_GET_ENC_MESSAGE
    { &ClientSolver::Handle_NULL }, // SP_ENC_MESSAGE
    { &ClientSolver::HandleGetWork }, // CP_GET_WORK
    { &ClientSolver::Handle_NULL }, // SP_GIVE_WORK
    { &ClientSolver::HandleSubmitResult }, // CP_SUBMIT_RESULT
    { &ClientSolver::Handle_NULL }, // SP_RESULT_ACCEPT
};

#endif
