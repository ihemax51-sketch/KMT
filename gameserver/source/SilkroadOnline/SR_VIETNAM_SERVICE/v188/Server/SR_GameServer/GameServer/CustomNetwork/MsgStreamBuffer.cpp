#include "MsgStreamBuffer.h"

#include <climits>
#include <stdexcept>

CMsgStreamBuffer::FN_CONSTRUCTOR CMsgStreamBuffer::s_pfnConstructor = NULL;
CMsgStreamBuffer::FN_READ CMsgStreamBuffer::s_pfnRead = NULL;
CMsgStreamBuffer::FN_WRITE CMsgStreamBuffer::s_pfnWrite = NULL;
CMsgStreamBuffer::FN_SEND CMsgStreamBuffer::s_pfnSend = NULL;

#define CONSTRUCTOR_OFFSET	0x0053CEA0
#define READ_OFFSET			0x004F7220
#define WRITE_OFFSET		0x00508FE0
#define SEND_OFFSET			0x008418D0

void CMsgStreamBuffer::Initialize()
{
    s_pfnConstructor = reinterpret_cast<FN_CONSTRUCTOR>(CONSTRUCTOR_OFFSET);
    s_pfnRead = reinterpret_cast<FN_READ>(READ_OFFSET);
    s_pfnWrite = reinterpret_cast<FN_WRITE>(WRITE_OFFSET);
    s_pfnSend = reinterpret_cast<FN_SEND>(SEND_OFFSET);
}

CMsgStreamBuffer::CMsgStreamBuffer(uint16_t id)
{
    if (s_pfnConstructor == NULL)
        throw std::runtime_error("Message stream is not initialized");
    s_pfnConstructor(this, id);
}

size_t CMsgStreamBuffer::GetOffset() const
{
    return m_offset;
}

size_t CMsgStreamBuffer::GetLength() const
{
    return m_length;
}

size_t CMsgStreamBuffer::GetRemainRead() const
{
    return GetOffset() <= GetLength() ? GetLength() - GetOffset() : 0;
}

uint16_t CMsgStreamBuffer::GetID() const
{
    return m_id;
}

SMsgStreamNode* CMsgStreamBuffer::GetFrontNode() const
{
    return m_pFrontNode;
}

SMsgStreamNode* CMsgStreamBuffer::GetCurrentNode() const
{
    return m_pCurNode;
}

size_t CMsgStreamBuffer::GetNodeCount() const
{
    int count = (GetLength() / MSG_NODE_BUFFER_SIZE);
    int tail = (GetLength() % MSG_NODE_BUFFER_SIZE);
    if (tail > 0)
        ++count;
    return count;
}

size_t CMsgStreamBuffer::Read(void* dest, size_t count)
{
    if (dest == NULL || s_pfnRead == NULL || count > GetRemainRead())
        throw std::runtime_error("Message stream read exceeds payload");
    return s_pfnRead(this, dest, count);
}

std::string CMsgStreamBuffer::ReadStringA()
{
    uint16_t len = Read<uint16_t>();

    std::string str;
    str.resize(len);
    if (len != 0)
        Read(&str[0], len);

    return str;
}

std::wstring CMsgStreamBuffer::ReadStringW()
{
    uint16_t len = Read<uint16_t>();

    std::wstring str;
    str.resize(len);
    if (len != 0)
        Read(&str[0], len * sizeof(wchar_t));

    return str;
}

void* CMsgStreamBuffer::Write(const void* src, size_t count)
{
    if (src == NULL || s_pfnWrite == NULL)
        return NULL;
    return s_pfnWrite(this, src, count);
}

void CMsgStreamBuffer::WriteStringA(const std::string& str)
{
    const size_t length = str.length() > USHRT_MAX ? USHRT_MAX : str.length();
    Write<uint16_t>(static_cast<uint16_t>(length));
    if (length != 0)
        Write(str.data(), length);
}

void CMsgStreamBuffer::WriteStringW(const std::wstring& str)
{
    const size_t length = str.length() > USHRT_MAX ? USHRT_MAX : str.length();
    Write<uint16_t>(static_cast<uint16_t>(length));
    if (length != 0)
        Write(str.data(), length * sizeof(wchar_t));
}

void CMsgStreamBuffer::Seek(size_t offset)
{
    if (offset > m_length)
        throw std::runtime_error("Invalid message stream seek");
    m_offset = offset;
}

void CMsgStreamBuffer::Skip(size_t count)
{
    if (count > GetRemainRead())
        throw std::runtime_error("Invalid message stream skip");
    m_offset += count;
}

void CMsgStreamBuffer::Send()
{
    if (s_pfnSend != NULL)
        s_pfnSend(this);
}
