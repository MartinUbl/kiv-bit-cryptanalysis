#include "general.h"
#include "Opcodes.h"
#include "SmartPacket.h"
#include "SessionManager.h"
#include "ClientSolver.h"
#include "SolveManager.h"

ClientSolver::ClientSolver(SOCK socket)
{
    m_mySocket = socket;
}

void ClientSolver::SendPacket(SmartPacket &pkt)
{
    sSessionManager->SendPacket(pkt, m_mySocket);
}

void ClientSolver::HandlePacket(SmartPacket &pkt)
{
    if (pkt.GetOpcode() >= MAX_OPCODE)
    {
        cerr << "Received invalid opcode: " << pkt.GetOpcode() << endl;
        return;
    }

    try
    {
        (*this.*_packetHandlers[pkt.GetOpcode()].handler)(pkt);
    }
    catch (PacketReadException &ex)
    {
        cerr << "Error reading from packet at position " << ex.GetPosition() << ", attempted to read " << ex.GetAttemptSize() << "bytes" << endl;
    }
}

void ClientSolver::Handle_NULL(SmartPacket& pkt)
{
    //
}

void ClientSolver::HandleHello(SmartPacket& pkt)
{
    uint32_t val = pkt.ReadUInt32();
    cout << "Received hello value: " << val << endl;

    SmartPacket resp(SP_HELLO_RESPONSE);
    resp.WriteUInt32(val);
    resp.WriteUInt32(2);
    SendPacket(resp);
}

void ClientSolver::HandleGetFreqMap(SmartPacket& pkt)
{
    double* freqmap = sSolveManager->getDataLoader().GetFrequencyMap();

    SmartPacket resp(SP_FREQ_MAP);
    resp.WriteUInt32(ALPHABET_SIZE);
    for (int i = 0; i < ALPHABET_SIZE; i++)
        resp.WriteFloat((float)(freqmap[i]));
    SendPacket(resp);
}

void ClientSolver::HandleGetDictionary(SmartPacket& pkt)
{
    std::vector<string>* dict = sSolveManager->getDataLoader().GetDictionary();
    uint8_t ceiling;

    for (uint32_t i = 0; i < dict->size(); i += 20)
    {
        SmartPacket part(SP_DICTIONARY_WORDS);
        if (i + 20 < dict->size())
            ceiling = 20;
        else
            ceiling = (uint8_t)(dict->size() - i);

        part.WriteUInt8(ceiling);
        for (uint8_t j = 0; j < ceiling; j++)
            part.WriteString((*dict)[i + j].c_str());

        SendPacket(part);
    }

    SmartPacket endpkt(SP_END_OF_DICTIONARY);
    SendPacket(endpkt);
}

void ClientSolver::HandleGetEncryptedMessage(SmartPacket& pkt)
{
    SmartPacket resp(SP_ENC_MESSAGE);
    resp.WriteString(sSolveManager->getMessage().c_str());
    SendPacket(resp);
}

void ClientSolver::HandleGetWork(SmartPacket& pkt)
{
    CipherType ct = sSolveManager->getWork();

    SmartPacket wp(SP_GIVE_WORK);
    wp.WriteUInt16(ct);
    // generate (pseudo)random seed for client and distribute entrophy
    if (ct != CT_NONE)
        wp.WriteUInt32((uint32_t)rand());

    SendPacket(wp);
}

void ClientSolver::HandleSubmitResult(SmartPacket& pkt)
{
    float freqScore = pkt.ReadFloat();
    float dictScore = pkt.ReadFloat();
    std::string result = pkt.ReadString();

    cout << "Result, score: FREQ " << freqScore << ", DICT " << dictScore << endl;

    sSolveManager->AddClientResult(freqScore, dictScore, result.c_str());

    SmartPacket resp(SP_RESULT_ACCEPT);
    SendPacket(resp);
}
