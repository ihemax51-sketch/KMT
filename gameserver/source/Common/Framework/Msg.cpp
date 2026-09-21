//
// Created by Kurama on 12/25/2022.
//
#include <climits>
#include <stdexcept>
#include "Msg.h"

// todo : need to use the jmx exception

void CMsg::AddSizeToMsg(WORD wSize) {
    if (m_wpMsgSize == NULL || wSize > static_cast<WORD>(~MSG_ENC_MASK))
        throw std::runtime_error("Invalid message size");

    // Lets Enc it back
    *m_wpMsgSize = (wSize | (*m_wpMsgSize & MSG_ENC_MASK));
}
void CMsg::Read(void* dest, __int16 count)
{
    if (count < 0)
        throw std::runtime_error("Invalid message read length");

    if (m_dwReadMsgMode == MSG_READ_MODE_REVERSE)
        ReadBytesReverse(dest, static_cast<size_t>(count));
    else
        ReadBytes(dest, static_cast<size_t>(count));
}

void CMsg::SetReadPos(WORD wPos)
{
    if (m_pMsgBuffer == NULL ||
        wPos > m_wWriteDataArrayPos ||
        m_wWriteDataArrayPos > m_dwArrayDataSize)
        throw std::runtime_error("Invalid message read position");

    m_wReadDataArrayPos = wPos;
}

void CMsg::ReadBytes(void *pOut, size_t cbSize) {
    if (pOut == NULL ||
        m_pMsgBuffer == NULL ||
        m_wReadDataArrayPos > m_wWriteDataArrayPos ||
        cbSize > static_cast<size_t>(m_wWriteDataArrayPos - m_wReadDataArrayPos))
        throw std::runtime_error("Message read exceeds payload");

    memcpy(pOut, &m_pMsgBuffer[m_wReadDataArrayPos], cbSize);
    m_wReadDataArrayPos += cbSize;
}

void CMsg::ReadBytesReverse(void *pOut, size_t cbSize) {
    if (pOut == NULL ||
        m_pMsgBuffer == NULL ||
        m_wpMsgSize == NULL ||
        m_wWriteDataArrayPos < MSG_HEADER_SIZE ||
        cbSize > static_cast<size_t>(m_wWriteDataArrayPos - MSG_HEADER_SIZE))
        throw std::runtime_error("Reverse message read exceeds payload");

    m_wWriteDataArrayPos -= cbSize;
    memcpy(pOut, &m_pMsgBuffer[m_wWriteDataArrayPos], cbSize);

    AddSizeToMsg((m_wWriteDataArrayPos - MSG_HEADER_SIZE));
}

void CMsg::WriteBytes(const void *pIn, size_t cbSize) {
    Write(pIn, cbSize);
}

void CMsg::WriteString(const char *pChar) {
    size_t cbLength = 0;

    if (pChar != NULL)
        cbLength = ::lstrlenA(pChar);

    const size_t remaining = m_wWriteDataArrayPos >= MSG_HEADER_SIZE &&
                             m_wWriteDataArrayPos <= MSG_HEADER_SIZE + 0x7FFF
        ? static_cast<size_t>(MSG_HEADER_SIZE + 0x7FFF - m_wWriteDataArrayPos)
        : 0;
    if (remaining < sizeof(WORD)) { m_bWriteOverflow = 1; return; }
    const size_t payloadCapacity = remaining - sizeof(WORD);
    if (cbLength > payloadCapacity || cbLength > USHRT_MAX) { m_bWriteOverflow = 1; return; }

    (*this) << (WORD) cbLength;
    if (cbLength != 0)
        Write(pChar, cbLength);
}

void CMsg::WriteString(const std::string &str) {
    size_t cbLength = str.length();

    const size_t remaining = m_wWriteDataArrayPos >= MSG_HEADER_SIZE &&
                             m_wWriteDataArrayPos <= MSG_HEADER_SIZE + 0x7FFF
        ? static_cast<size_t>(MSG_HEADER_SIZE + 0x7FFF - m_wWriteDataArrayPos)
        : 0;
    if (remaining < sizeof(WORD)) { m_bWriteOverflow = 1; return; }
    const size_t payloadCapacity = remaining - sizeof(WORD);
    if (cbLength > payloadCapacity || cbLength > USHRT_MAX) { m_bWriteOverflow = 1; return; }

    (*this) << (WORD) cbLength;
    if (cbLength != 0)
        Write(str.c_str(), cbLength);
}
void CMsg::WriteStringW(const std::wstring& str)
{
    const size_t remaining = m_wWriteDataArrayPos >= MSG_HEADER_SIZE &&
                             m_wWriteDataArrayPos <= MSG_HEADER_SIZE + 0x7FFF
        ? static_cast<size_t>(MSG_HEADER_SIZE + 0x7FFF - m_wWriteDataArrayPos)
        : 0;
    if (remaining < sizeof(WORD)) { m_bWriteOverflow = 1; return; }

    size_t length = str.length();
    const size_t characterCapacity = (remaining - sizeof(WORD)) / sizeof(wchar_t);
    if (length > characterCapacity || length > USHRT_MAX) { m_bWriteOverflow = 1; return; }

    this->Write<WORD>(static_cast<WORD>(length));
    if (length > 0)
        this->Write(str.c_str(), length * sizeof(wchar_t));
}

void CMsg::ReadStringW(std::wstring& str)
{
    ReadStringW(str, 0x7FFF);
}

void CMsg::ReadStringW(std::wstring& str, size_t maxLength)
{
    WORD sLength = 0;
    (*this) >> sLength;

    if (sLength == 0)
    {
        str.clear();
        return;
    }

    if (static_cast<size_t>(sLength) > maxLength)
        throw std::runtime_error("Wide string exceeds allowed length");

    const size_t byteCount = static_cast<size_t>(sLength) * sizeof(wchar_t);
    if (m_wReadDataArrayPos > m_wWriteDataArrayPos ||
        byteCount > static_cast<size_t>(m_wWriteDataArrayPos - m_wReadDataArrayPos))
        throw std::runtime_error("Wide string exceeds message payload");

    str.resize(sLength);
    ReadBytes(&str[0], byteCount);
}
std::n_wstring CMsg::ReadNStringW()
{
    WORD len = Read<WORD>();

    if (len == 0)
        return std::n_wstring();

    const size_t byteCount = static_cast<size_t>(len) * sizeof(wchar_t);
    if (m_wReadDataArrayPos > m_wWriteDataArrayPos ||
        byteCount > static_cast<size_t>(m_wWriteDataArrayPos - m_wReadDataArrayPos))
        throw std::runtime_error("Wide string exceeds message payload");

    wchar_t* buffer = new wchar_t[len + 1]; // +1 for null terminator
    ReadBytes(buffer, byteCount);
    buffer[len] = L'\0'; // Null terminate the string

    std::n_wstring result(buffer);

    delete[] buffer; // Don't forget to release memory

    return result;
}
void CMsg::ReadString(std::string &str) {
    ReadString(str, 0x7FFF);
}

void CMsg::ReadString(std::string &str, size_t maxLength) {
    WORD sLength = 0;
    (*this) >> sLength;

    if (sLength == 0)
    {
        str.clear();
        return;
    }

    if (static_cast<size_t>(sLength) > maxLength ||
        m_wReadDataArrayPos > m_wWriteDataArrayPos ||
        static_cast<size_t>(sLength) >
            static_cast<size_t>(m_wWriteDataArrayPos - m_wReadDataArrayPos))
        throw std::runtime_error("String exceeds message payload");

    str.resize(sLength);
    ReadBytes(&str[0], sLength);
}

void CMsg::Write(const void* src, size_t count)
{
    if (src == NULL ||
        m_pMsgBuffer == NULL ||
        m_wpMsgSize == NULL ||
        m_wWriteDataArrayPos < MSG_HEADER_SIZE ||
        m_wWriteDataArrayPos > m_dwArrayDataSize ||
        m_wWriteDataArrayPos > MSG_HEADER_SIZE + 0x7FFF ||
        count > static_cast<size_t>(MSG_HEADER_SIZE + 0x7FFF - m_wWriteDataArrayPos))
    {
        m_bWriteOverflow = 1;
        return;
    }

    __asm pushad;
    __asm pushfd;

    __asm push src;
    __asm mov ecx, this;
    __asm mov eax, count;
    __asm mov edx, 0x00404090;
    __asm call edx;

    __asm popfd;
    __asm popad;
}

bool CMsg::TryWrite(const void* src, size_t count)
{
    if (HasWriteOverflow()) return false;
    const WORD before = m_wWriteDataArrayPos;
    Write(src, count);
    return !HasWriteOverflow() &&
           m_wWriteDataArrayPos == static_cast<WORD>(before + count);
}

bool CMsg::TryWriteString(const std::string& str)
{
    if (HasWriteOverflow()) return false;
    const WORD before = m_wWriteDataArrayPos;
    WriteString(str);
    return !HasWriteOverflow() && m_wWriteDataArrayPos >= before;
}

bool CMsg::TryWriteStringW(const std::wstring& str)
{
    if (HasWriteOverflow()) return false;
    const WORD before = m_wWriteDataArrayPos;
    WriteStringW(str);
    return !HasWriteOverflow() && m_wWriteDataArrayPos >= before;
}
