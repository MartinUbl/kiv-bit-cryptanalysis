#include "general.h"
#include "Opcodes.h"
#include "SmartPacket.h"
#include "SessionManager.h"
#include "ClientSolver.h"
#include "SolveManager.h"

ClientSolver::ClientSolver(SOCK socket)
{
    m_mySocket = socket;
    m_myWork = CT_NONE;
}

void ClientSolver::SendPacket(SmartPacket &pkt)
{
    sSessionManager->SendPacket(pkt, m_mySocket);
}

void ClientSolver::HandlePacket(SmartPacket &pkt)
{
    // secure range
    if (pkt.GetOpcode() >= MAX_OPCODE)
    {
        cerr << "NET: Received invalid opcode: " << pkt.GetOpcode() << endl;
        return;
    }

    try
    {
        // call appropriate handler
        (*this.*_packetHandlers[pkt.GetOpcode()].handler)(pkt);
    }
    catch (PacketReadException &ex)
    {
        cerr << "NET: Error reading from packet at position " << ex.GetPosition() << ", attempted to read " << ex.GetAttemptSize() << "bytes" << endl;
    }
}

CipherType ClientSolver::GetMyWork()
{
    return m_myWork;
}

void ClientSolver::Handle_NULL(SmartPacket& pkt)
{
    //
}

void ClientSolver::HandleHello(SmartPacket& pkt)
{
    // just random handshake stuff
    uint32_t val = pkt.ReadUInt32();

    SmartPacket resp(SP_HELLO_RESPONSE);
    resp.WriteUInt32(val);
    resp.WriteUInt32(2);
    SendPacket(resp);
}

void ClientSolver::HandleGetFreqMap(SmartPacket& pkt)
{
    double* freqmap = sSolveManager->getDataLoader().GetFrequencyMap();
    std::map<string, float>* bifreqmap = sSolveManager->getDataLoader().GetBigramFrequencyMap();

    SmartPacket resp(SP_FREQ_MAP);

    // write unigram frequencies
    resp.WriteUInt32(ALPHABET_SIZE);
    for (int i = 0; i < ALPHABET_SIZE; i++)
        resp.WriteFloat((float)(freqmap[i]));

    // write bigram frequencies
    resp.WriteUInt32(bifreqmap->size());
    for (auto &bif : *bifreqmap)
    {
        resp.WriteString(bif.first.c_str());
        resp.WriteFloat(bif.second);
    }

    SendPacket(resp);
}

void ClientSolver::HandleGetDictionary(SmartPacket& pkt)
{
    std::vector<string>* dict = sSolveManager->getDataLoader().GetDictionary();
    uint8_t ceiling;

    // send dictionary in smaller packets
    for (uint32_t i = 0; i < dict->size(); i += 20)
    {
        SmartPacket part(SP_DICTIONARY_WORDS);
        // determine maximum word count
        if (i + 20 < dict->size())
            ceiling = 20;
        else
            ceiling = (uint8_t)(dict->size() - i);

        // write words
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

    // if there was some previous work, mark as finished
    if (m_myWork != CT_NONE)
        sSolveManager->finishWork(m_myWork);

    m_myWork = ct;

    SmartPacket wp(SP_GIVE_WORK);
    wp.WriteUInt16(ct);
    // generate (pseudo)random seed for client and distribute "entrophy"
    if (ct != CT_NONE)
        wp.WriteUInt32((uint32_t)(rand()*rand()));

    SendPacket(wp);
}

void ClientSolver::HandleSubmitResult(SmartPacket& pkt)
{
    // read scores and decrypted message
    float freqScore = pkt.ReadFloat();
    float dictScore = pkt.ReadFloat();
    std::string result = pkt.ReadString();

    cout << "Obtained result, score: FREQ " << freqScore << ", DICT " << dictScore << endl;
    cout << "First 60 chars: " << result.substr(0, 80-16-4).c_str() << " .." << endl;

    // log to file
    FILE* log_m = fopen("output.txt", "a");
    if (log_m)
    {
        fprintf(log_m, "Score: frequency %f, dictionary %f\nMessage: %s\n", freqScore, dictScore, result.c_str());
        fclose(log_m);
    }

    // finally submit result
    sSolveManager->AddClientResult(freqScore, dictScore, result.c_str());

    // signal that result was accepted
    SmartPacket resp(SP_RESULT_ACCEPT);
    SendPacket(resp);
}
