#ifndef KIV_BIT_CRYPTOWORKER_SESSION_H
#define KIV_BIT_CRYPTOWORKER_SESSION_H

#include "Singleton.h"
#include "SmartPacket.h"
#include "Opcodes.h"

#include <condition_variable>

#ifdef _WIN32
#include <Windows.h>
#define SOCK SOCKET
#define ADDRLEN int

#define SOCKETWOULDBLOCK WSAEWOULDBLOCK
#define SOCKETCONNRESET  WSAECONNRESET
#define SOCKETCONNABORT  WSAECONNABORTED
#define LASTERROR() WSAGetLastError()
#define INET_PTON(fam,addrptr,buff) InetPton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) InetNtop(fam,addrptr,buff,socksize)
#else
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <fcntl.h>

#define SOCK int
#define ADDRLEN socklen_t

#define INVALID_SOCKET -1

#define SOCKETWOULDBLOCK EAGAIN
#define SOCKETCONNABORT ECONNABORTED
#define SOCKETCONNRESET ECONNRESET
#define LASTERROR() errno
#define INET_PTON(fam,addrptr,buff) inet_pton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) inet_ntop(fam,addrptr,buff,socksize)
#endif

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

#define WAITFOR SetWaitForResponse

class Session
{
    friend class Singleton<Session>;
    public:
        // connect to master
        bool Connect(const char* serverAddr, uint16_t port);
        // send packet to master
        void SendPacket(SmartPacket& pkt);

        // join this thread (i.e. to wait until it finishes properly)
        void JoinThread();

        // set the opcode, for which we will wait in next SendPacket call (after sending)
        Session* SetWaitForResponse(uint16_t opc);

        // send HELLO packet
        void SendHello();
        // request frequencies
        void SendGetFreqMap();
        // request dictionary
        void SendGetDictionary();
        // request encrypted message
        void SendGetEncMessage();
        // request work from master
        void SendGetWork();
        // submit solved result
        void SendSubmitResult(float freqScore, float dictScore, const char* result);

        void Handle_NULL(SmartPacket &pkt);
        void HandleHelloResponse(SmartPacket& pkt);
        void HandleFreqMapPacket(SmartPacket& pkt);
        void HandleDictionaryWordsPacket(SmartPacket& pkt);
        void HandleEndOfDictionaryPacket(SmartPacket& pkt);
        void HandleEncMessagePacket(SmartPacket& pkt);
        void HandleGiveWorkPacket(SmartPacket& pkt);
        void HandleResultAcceptPacket(SmartPacket& pkt);

    protected:
        // protected singleton instance
        Session();
        // run session manager
        void Run();
        // handle incoming packet
        void HandlePacket(SmartPacket &pkt);

    private:
        // master host
        string m_host;
        // master port
        uint16_t m_port;

        // are we waiting for response? if yes (>0), for what opcode?
        uint16_t m_waitForResponse;

        // our sockaddr
        sockaddr_in m_sockAddr;
        // our socket
        SOCK m_socket;
        // network thread
        std::thread* m_networkThread;
        // response waiting monitor
        std::condition_variable m_respCond;
        // mutex for synchronization
        std::mutex m_netMutex;
};

#define sSession Singleton<Session>::getInstance()

struct HandlerStruct
{
    // for now just method pointer

    void (Session::*handler)(SmartPacket&);
};

static HandlerStruct _packetHandlers[] = {
    { &Session::Handle_NULL }, // GP_NONE
    { &Session::Handle_NULL }, // CP_HELLO
    { &Session::HandleHelloResponse }, // SP_HELLO_RESPONSE
    { &Session::Handle_NULL }, // CP_GET_FREQ_MAP
    { &Session::HandleFreqMapPacket }, // SP_FREQ_MAP
    { &Session::Handle_NULL }, // CP_GET_DICTIONARY
    { &Session::HandleDictionaryWordsPacket }, // SP_DICTIONARY_WORDS
    { &Session::HandleEndOfDictionaryPacket }, // SP_END_OF_DICTIONARY
    { &Session::Handle_NULL }, // CP_GET_ENC_MESSAGE
    { &Session::HandleEncMessagePacket }, // SP_ENC_MESSAGE
    { &Session::Handle_NULL }, // CP_GET_WORK
    { &Session::HandleGiveWorkPacket }, // SP_GIVE_WORK
    { &Session::Handle_NULL }, // CP_SUBMIT_RESULT
    { &Session::HandleResultAcceptPacket }, // SP_RESULT_ACCEPT
};

#endif
