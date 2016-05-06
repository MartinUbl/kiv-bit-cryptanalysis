#ifdef _WIN32
#include <WS2tcpip.h>
#endif
#include "general.h"
#include "Session.h"
#include "Solver.h"

#define RECV_BUFFER_LEN 2048

Session::Session()
{
    m_waitForResponse = GP_NONE;
}

bool Session::Connect(const char* serverAddr, uint16_t port)
{
    // init winsock when on windows
#ifdef _WIN32
    WORD version = MAKEWORD(1, 1);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
    {
        cerr << "Unable to start winsock service" << endl;
        return false;
    }
#endif

    // init socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons(port);
    m_host = serverAddr;
    m_port = port;

    // resolve remote address
    if (INET_PTON(AF_INET, serverAddr, &m_sockAddr.sin_addr.s_addr) != 1)
    {
        cerr << "Unable to resolve remote address" << endl;
        return false;
    }

    // connect!
    if (connect(m_socket, (sockaddr*)&m_sockAddr, sizeof(sockaddr_in)) < 0)
    {
        cerr << "Unable to connect to server" << endl;
        return false;
    }

    // wheeeeeeeee!
    m_networkThread = new std::thread(&Session::Run, this);

    return true;
}

void Session::JoinThread()
{
    m_networkThread->join();
}

Session* Session::SetWaitForResponse(uint16_t opc)
{
    m_waitForResponse = opc;
    return this; // fluent interface
}

void Session::SendPacket(SmartPacket& pkt)
{
    uint16_t op, sz;
    uint8_t* tosend = new uint8_t[SMARTPACKET_HEADER_SIZE + pkt.GetSize()];

    op = htons(pkt.GetOpcode());
    sz = htons(pkt.GetSize());

    // write opcode
    memcpy(tosend, &op, 2);
    // write contents size
    memcpy(tosend + 2, &sz, 2);
    // write contents
    memcpy(tosend + 4, pkt.GetData(), pkt.GetSize());

    std::unique_lock<std::mutex> lck(m_netMutex);

    // send response
    send(m_socket, (const char*)tosend, pkt.GetSize() + SMARTPACKET_HEADER_SIZE, MSG_NOSIGNAL);

    // when waiting for response is set, wait on condition variable
    if (m_waitForResponse)
    {
        m_respCond.wait(lck);
        m_waitForResponse = GP_NONE;
    }
}

void Session::Run()
{
    fd_set rdset;
    int res;

    // 1 second timeout for select
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 1;
    uint8_t recvbuff[RECV_BUFFER_LEN];

    struct
    {
        uint16_t opcode;
        uint16_t size;
    } recvHeader;

    while (1)
    {
        FD_ZERO(&rdset);
        FD_SET(m_socket, &rdset);

        tv.tv_sec = 1;
        tv.tv_usec = 1;

        // select - to be properly blocked before socket read is available
        res = select(m_socket + 1, &rdset, nullptr, nullptr, &tv);
        // error
        if (res < 0)
        {
            cerr << "select(): error " << LASTERROR() << endl;
        }
        // something's on input
        else if (res > 0)
        {
            if (FD_ISSET(m_socket, &rdset))
            {
                // read initial bytes (header)
                res = recv(m_socket, (char*)&recvHeader, SMARTPACKET_HEADER_SIZE, 0);
                // sanitize length
                if (res < SMARTPACKET_HEADER_SIZE)
                {
                    cerr << "recv(): error, received malformed packet" << endl;
                    exit(2);
                }
                else
                {
                    // retrieve opcode and size
                    recvHeader.opcode = ntohs(recvHeader.opcode);
                    recvHeader.size = ntohs(recvHeader.size);

                    if (recvHeader.size > 0 && recvHeader.size < RECV_BUFFER_LEN)
                    {
                        int recbytes = 0;

                        // while there's something to read..
                        while (recbytes != recvHeader.size)
                        {
                            // retrieve next bunch
                            res = recv(m_socket, (char*)(recvbuff + recbytes), recvHeader.size - recbytes, 0);
                            if (res <= 0)
                            {
                                if (LASTERROR() == SOCKETWOULDBLOCK)
                                    continue;
                                cerr << "recv(): error, received packet with different size than expected" << endl;
                                recbytes = -1;
                                break;
                            }
                            recbytes += res;
                        }

                        if (recbytes == -1)
                            continue;
                    }
                    else if (recvHeader.size >= RECV_BUFFER_LEN)
                    {
                        cerr << "recv(): error, received bigger packet than expected, exiting to avoid overflow" << endl;
                        exit(2);
                    }

                    // build packet
                    SmartPacket pkt(recvHeader.opcode, recvHeader.size);
                    pkt.SetData(recvbuff, recvHeader.size);

                    // handle it
                    HandlePacket(pkt);
                }
            }
        }
    }
}

void Session::HandlePacket(SmartPacket &pkt)
{
    std::unique_lock<std::mutex> lck(m_netMutex);

    try
    {
        (*this.*_packetHandlers[pkt.GetOpcode()].handler)(pkt);
    }
    catch (PacketReadException &ex)
    {
        cerr << "Failed attempt to read " << ex.GetAttemptSize() << " from packet at position " << ex.GetPosition() << endl;
    }

    if (m_waitForResponse != GP_NONE && m_waitForResponse == pkt.GetOpcode())
        m_respCond.notify_all();
}

void Session::SendHello()
{
    cout << "Sending HELLO packet" << endl;

    SmartPacket pkt(CP_HELLO);
    pkt.WriteUInt32(1);
    SendPacket(pkt);
}

void Session::SendGetFreqMap()
{
    cout << "Requesting frequency map..." << endl;

    SmartPacket pkt(CP_GET_FREQ_MAP);
    SendPacket(pkt);
}

void Session::SendGetDictionary()
{
    cout << "Requesting dictionary..." << endl;

    SmartPacket pkt(CP_GET_DICTIONARY);
    SendPacket(pkt);
}

void Session::SendGetEncMessage()
{
    cout << "Requesting encrypted message..." << endl;

    SmartPacket pkt(CP_GET_ENC_MESSAGE);
    SendPacket(pkt);
}

void Session::SendGetWork()
{
    cout << "Requesting work..." << endl;

    SmartPacket pkt(CP_GET_WORK);
    SendPacket(pkt);
}

void Session::SendSubmitResult(float freqScore, float dictScore, const char* result)
{
    cout << "Submitting result (FS " << freqScore << ", DS " << dictScore << "): " << std::string(result, 40).c_str() << " .." << endl;

    SmartPacket pkt(CP_SUBMIT_RESULT);
    pkt.WriteFloat(freqScore);
    pkt.WriteFloat(dictScore);
    pkt.WriteString(result);
    SendPacket(pkt);
}

void Session::Handle_NULL(SmartPacket& pkt)
{
    // this should never happen
}

void Session::HandleHelloResponse(SmartPacket& pkt)
{
    uint32_t res = pkt.ReadUInt32();
    uint32_t res2 = pkt.ReadUInt32();

    cout << "Received HELLO response" << endl;
}

void Session::HandleFreqMapPacket(SmartPacket& pkt)
{
    // read alphabet size
    uint32_t alphabetSize = pkt.ReadUInt32();
    float* freqmap = new float[alphabetSize];
    // read frequencies
    for (uint32_t i = 0; i < alphabetSize; i++)
        freqmap[i] = pkt.ReadFloat();

    // store frequencies
    sDataHolder->SetFrequencies(alphabetSize, freqmap);

    // retrieve bigram count
    uint32_t bigramsSize = pkt.ReadUInt32();
    // read and store bigram frequencies
    for (uint32_t i = 0; i < bigramsSize; i++)
        sDataHolder->AddBigram(pkt.ReadString().c_str(), pkt.ReadFloat());

    cout << "Received frequency map of " << alphabetSize << " letters and " << bigramsSize << " bigrams" << endl;
}

void Session::HandleDictionaryWordsPacket(SmartPacket& pkt)
{
    // retrieve and store all words in this packet
    uint8_t cnt = pkt.ReadUInt8();
    for (uint8_t i = 0; i < cnt; i++)
        sDataHolder->AddWord(pkt.ReadString());

    cout << "Added " << (int)cnt << " words to dictionary" << endl;
}

void Session::HandleEndOfDictionaryPacket(SmartPacket& pkt)
{
    // do nothing, just marker packet (the main solver thread is blocked until this packet is retrieved)

    cout << "End of dictionary stream" << endl;
}

void Session::HandleEncMessagePacket(SmartPacket& pkt)
{
    sDataHolder->SetEncryptedMessage(pkt.ReadString());

    cout << "Obtained encrypted message: " << sDataHolder->GetEncryptedMessage() << endl;
}

void Session::HandleGiveWorkPacket(SmartPacket& pkt)
{
    uint16_t work = pkt.ReadUInt16();
    if (work >= CT_NONE && work < MAX_CIPHER_TYPE)
        sSolverManager->SetObtainedWork((CipherType)work);

    if (work != CT_NONE)
    {
        uint32_t seed = pkt.ReadUInt32() % 100;

        cout << "Obtained work: " << work << ", received seed: " << seed << endl;

        // set new random seed; generate N numbers mod 100 by every generator
        for (uint32_t i = 0; i < seed; i++)
        {
            standardChance();
            generalChance();
        }
    }
    else
        cout << "No work, exiting" << endl;
}

void Session::HandleResultAcceptPacket(SmartPacket& pkt)
{
    // do nothing

    cout << "Result accepted" << endl;
}
