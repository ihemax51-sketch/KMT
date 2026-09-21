#include "MsgCustom.h"

#include <climits>
#include <cstring>
#include <stdexcept>

namespace
{
    const size_t MSG_HEADER_SIZE = 6;
    const WORD MSG_ENCRYPTED_MASK = 0x8000;

    char* GetBuffer(const CMsgStreamBufferCustom* message)
    {
        return message != NULL
            ? MEMUTIL_READ_BY_PTR_OFFSET(message, 0x1034, char*)
            : NULL;
    }

    DWORD GetCapacity(const CMsgStreamBufferCustom* message)
    {
        return message != NULL
            ? MEMUTIL_READ_BY_PTR_OFFSET(message, 0x104C, DWORD)
            : 0;
    }

    WORD* GetSizePointer(const CMsgStreamBufferCustom* message)
    {
        return message != NULL
            ? MEMUTIL_READ_BY_PTR_OFFSET(message, 0x1054, WORD*)
            : NULL;
    }
}

WORD CMsgStreamBufferCustom::GetID() const
{
    WORD* pAddr = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x1050, WORD*);
    return pAddr != NULL ? *pAddr : 0;
}

WORD CMsgStreamBufferCustom::GetReadPos() const
{
    return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x103C, WORD);
}

WORD CMsgStreamBufferCustom::GetWritePos() const
{
    return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x103E, WORD);
}

void CMsgStreamBufferCustom::SetMsgID(WORD wMsgID)
{
    WORD* pAddr = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x1050, WORD*);
    if (pAddr != NULL)
        *pAddr = wMsgID;
}

void CMsgStreamBufferCustom::SetReadPos(WORD wPos)
{
    if (!IsValid() || wPos > GetWritePos())
        throw std::runtime_error("Invalid log message read position");
    MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x103C, WORD, wPos);
}

void CMsgStreamBufferCustom::SetWritePos(WORD wPos)
{
    if (GetBuffer(this) == NULL || wPos > GetCapacity(this) || wPos < GetReadPos())
        throw std::runtime_error("Invalid log message write position");
    MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x103E, WORD, wPos);
}

bool CMsgStreamBufferCustom::IsValid() const
{
    const WORD readPosition = GetReadPos();
    const WORD writePosition = GetWritePos();
    const DWORD capacity = GetCapacity(this);
    return GetBuffer(this) != NULL &&
           GetSizePointer(this) != NULL &&
           writePosition >= MSG_HEADER_SIZE &&
           readPosition <= writePosition &&
           writePosition <= capacity;
}

size_t CMsgStreamBufferCustom::GetRemainingRead() const
{
    return IsValid()
        ? static_cast<size_t>(GetWritePos() - GetReadPos())
        : 0;
}

void CMsgStreamBufferCustom::Read(void* dest, size_t count)
{
    if (dest == NULL || count > GetRemainingRead())
        throw std::runtime_error("Log message read exceeds payload");

    const WORD readPosition = GetReadPos();
    std::memcpy(dest, GetBuffer(this) + readPosition, count);
    MEMUTIL_WRITE_BY_PTR_OFFSET(
        this,
        0x103C,
        WORD,
        static_cast<WORD>(readPosition + count));
}

std::string CMsgStreamBufferCustom::ReadStringA()
{
    WORD len = Read<WORD>();

    if (static_cast<size_t>(len) > GetRemainingRead())
        throw std::runtime_error("Log string exceeds message payload");

    std::string str(len, 0);
    if (len != 0)
        Read(&str[0], len);

    return str;
}

std::wstring CMsgStreamBufferCustom::ReadStringW()
{
    WORD len = Read<WORD>();

    const size_t byteCount = static_cast<size_t>(len) * sizeof(wchar_t);
    if (byteCount > GetRemainingRead())
        throw std::runtime_error("Wide log string exceeds message payload");

    std::wstring str(len, 0);
    if (len != 0)
        Read(&str[0], byteCount);

    return str;
}


void CMsgStreamBufferCustom::Write(const void* src, size_t count)
{
    const WORD writePosition = GetWritePos();
    const DWORD capacity = GetCapacity(this);
    WORD* sizePointer = GetSizePointer(this);
    if (src == NULL || GetBuffer(this) == NULL || sizePointer == NULL ||
        writePosition < MSG_HEADER_SIZE || writePosition > capacity ||
        count > static_cast<size_t>(capacity - writePosition))
        return;

    std::memcpy(GetBuffer(this) + writePosition, src, count);
    const WORD newWritePosition = static_cast<WORD>(writePosition + count);
    MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x103E, WORD, newWritePosition);
    const WORD payloadSize = static_cast<WORD>(newWritePosition - MSG_HEADER_SIZE);
    *sizePointer = static_cast<WORD>(payloadSize | (*sizePointer & MSG_ENCRYPTED_MASK));
}

void CMsgStreamBufferCustom::WriteStringA(const std::string& str)
{
    const size_t length = str.length() > USHRT_MAX ? USHRT_MAX : str.length();
    this->Write<WORD>(static_cast<WORD>(length));
    if (length != 0)
        this->Write(str.data(), length);
}

void CMsgStreamBufferCustom::WriteStringW(const std::wstring& str)
{
    const size_t length = str.length() > USHRT_MAX ? USHRT_MAX : str.length();
    this->Write<WORD>(static_cast<WORD>(length));
    if (length != 0)
        this->Write(str.c_str(), length * sizeof(wchar_t));
}
