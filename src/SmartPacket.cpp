#include "general.h"
#include "SmartPacket.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

SmartPacket::SmartPacket() : m_opcode(0), m_size(0), m_readPos(0), m_writePos(0)
{
    //
}

SmartPacket::SmartPacket(uint16_t opcode, uint16_t size) : m_opcode(opcode), m_size(size), m_readPos(0), m_writePos(0)
{
    m_data.reserve(size);
}

SmartPacket::~SmartPacket()
{
    m_data.clear();
}

void SmartPacket::SetReadPos(uint16_t pos)
{
    // do not allow setting cursor outside of data range
    if (pos > m_size)
    {
        //sLog->Error("Attempting to set read cursor of packet with opcode %u to position %u whilst having only size of %u", m_opcode, pos, m_size);
        return;
    }

    m_readPos = pos;
}

uint16_t SmartPacket::GetWritePos()
{
    return m_writePos;
}

std::string SmartPacket::ReadString()
{
    // we can detect only starting point being out of range at this time
    if (m_readPos >= m_size)
        throw new PacketReadException(m_readPos, 1);

    int i;

    // find zero, or end of packet
    for (i = m_readPos; i < m_size; i++)
    {
        if (m_data[i] == '\0')
            break;
    }

    // if we reached end without finding zero, that means, the string is not properly ended
    // or we just tried to read something, that's not string; by all means, this is errorneous state
    if (m_data[i] != '\0')
        throw new PacketReadException(m_readPos, m_size - m_readPos + 1);

    int oldReadPos = m_readPos;
    // set read position one character further to skip the zero termination
    m_readPos = i+1;

    // and return "substring"
    return std::string((const char*)&m_data[oldReadPos], i - oldReadPos);
}

void SmartPacket::_Read(void* dst, size_t size)
{
    // disallow reading more bytes than available
    if (m_readPos + size > m_size)
        throw new PacketReadException(m_readPos, m_size);

    memcpy(dst, &m_data[m_readPos], size);
    m_readPos += (uint16_t)size;
}

uint32_t SmartPacket::ReadUInt32()
{
    uint32_t toret;
    _Read(&toret, 4);
    return (uint32_t)ntohl(toret);
}

int32_t SmartPacket::ReadInt32()
{
    int32_t toret;
    _Read(&toret, 4);
    return (int32_t)ntohl(toret);
}

uint16_t SmartPacket::ReadUInt16()
{
    uint16_t toret;
    _Read(&toret, 2);
    return (uint16_t)ntohs(toret);
}

int16_t SmartPacket::ReadInt16()
{
    int16_t toret;
    _Read(&toret, 2);
    return (int16_t)ntohs(toret);
}

uint8_t SmartPacket::ReadUInt8()
{
    uint8_t toret;
    _Read(&toret, 1);
    return toret;
}

int8_t SmartPacket::ReadInt8()
{
    int8_t toret;
    _Read(&toret, 1);
    return toret;
}

float SmartPacket::ReadFloat()
{
    uint32_t toret;
    _Read(&toret, 4);
    toret = ntohl(toret);
    return *(float*)&toret;
}

void SmartPacket::_Write(void* data, size_t size)
{
    m_data.resize(m_writePos + 1 + size);
    memcpy(&m_data[m_writePos], data, size);
    m_writePos += (uint16_t)size;

    if (m_size < m_writePos + 1)
        m_size = m_writePos + 1;
}

void SmartPacket::_WriteAt(void* data, size_t size, uint16_t position)
{
    memcpy(&m_data[position], data, size);
}

void SmartPacket::WriteString(const char* str)
{
    _Write((void*)str, strlen(str) + 1);
}

void SmartPacket::WriteUInt32(uint32_t val)
{
    val = htonl(val);
    _Write(&val, sizeof(uint32_t));
}

void SmartPacket::WriteInt32(int32_t val)
{
    val = htonl(val);
    _Write(&val, sizeof(int32_t));
}

void SmartPacket::WriteUInt16(uint16_t val)
{
    val = htons(val);
    _Write(&val, sizeof(uint16_t));
}

void SmartPacket::WriteInt16(int16_t val)
{
    val = htons((uint16_t)val);
    _Write(&val, sizeof(int16_t));
}

void SmartPacket::WriteUInt8(uint8_t val)
{
    _Write(&val, sizeof(uint8_t));
}

void SmartPacket::WriteInt8(int8_t val)
{
    _Write(&val, sizeof(int8_t));
}

void SmartPacket::WriteFloat(float val)
{
    uint32_t cval = htonl(*(uint32_t*)&val);
    _Write(&cval, sizeof(float));
}

void SmartPacket::WriteUInt32At(uint32_t val, uint16_t position)
{
    val = htonl(val);
    _WriteAt(&val, sizeof(uint32_t), position);
}

void SmartPacket::WriteUInt16At(uint16_t val, uint16_t position)
{
    val = htons(val);
    _WriteAt(&val, sizeof(uint16_t), position);
}

void SmartPacket::WriteUInt8At(uint8_t val, uint16_t position)
{
    _WriteAt(&val, sizeof(uint8_t), position);
}

void SmartPacket::SetData(uint8_t* data, uint16_t size)
{
    m_data = std::vector<uint8_t>(data, data + size);
}

uint8_t* SmartPacket::GetData()
{
    return m_data.data();
}

uint16_t SmartPacket::GetOpcode()
{
    return m_opcode;
}

void SmartPacket::SetOpcode(uint16_t opcode)
{
    m_opcode = opcode;
}

uint16_t SmartPacket::GetSize()
{
    return m_size;
}
