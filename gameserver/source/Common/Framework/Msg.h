//
// Created by Kurama on 12/15/2022.
//
#pragma once

#include <string>
#include "Windows.h"

#include "Test/Test.h"

enum MSG_READ_MODE : DWORD {
    MSG_READ_MODE_REVERSE,
    MSG_READ_MODE_NORMAL,
};

// Msg Info
#define MSG_BUFFER_SIZE 4096

#define MSG_BUFFER_OFFSET 0

#define MSG_SIZE_OFFSET MSG_BUFFER_OFFSET
#define MSG_ID_OFFSET (MSG_SIZE_OFFSET + sizeof(WORD)) // 2
#define MSG_SEQ_OFFSET (MSG_ID_OFFSET + sizeof(WORD))
#define MSG_CRC_OFFSET (MSG_SEQ_OFFSET + sizeof(BYTE))

#define MSG_DATA_OFFSET (MSG_CRC_OFFSET + sizeof(BYTE)) // 6
#define MSG_ENC_DATA_OFFSET (MSG_BUFFER_OFFSET + sizeof(WORD))

#define MSG_HEADER_SIZE MSG_DATA_OFFSET
#define MSG_DATA_SIZE MSG_BUFFER_SIZE - MSG_HEADER_SIZE;

#define MSG_ENC_MASK (WORD)0x8000

class CMsg {
    // non vtable
public:
    template<typename T>
    CMsg &operator>>(T &pOut) {
        ReadBytes(pOut);
        return *this;
    }

    CMsg &operator>>(std::string &str) {
        ReadString(str);
        return *this;
    }

    template<typename T>
    CMsg &operator<<(T In) {
        Write(&In, sizeof(T));
        return *this;
    }

    CMsg &operator<<(const std::string &str) {
        WriteString(str);
        return *this;
    }

    template<typename T>
    void Write(const T& value)
    {
        Write(&value, sizeof(T));
    }
    template<typename T>
    bool TryWrite(const T& value)
    {
        return TryWrite(&value, sizeof(T));
    }
    template<typename T>
    const T Read()
    {
        T value;
        Read<T>(value);
        return value;
    }
    template<typename T>
    void Read(T& value)
    {
        ReadBytes(value);
    }
private:
    template<typename T>
    void ReadBytesEx(T &pOut) {
        ReadBytes(&pOut, sizeof(T));
    }

    template<typename T>
    void ReadBytesReverseEx(T &pOut) {
        ReadBytesReverse(&pOut, sizeof(T));
    }

    template<typename T>
    void ReadBytes(T &pOut) {
        if (m_dwReadMsgMode == MSG_READ_MODE_REVERSE)
            ReadBytesReverseEx(pOut);
        else
            ReadBytesEx(pOut);
    }

public:
    void SetMsgID(WORD wMsgID) {
        if (m_wpMsgId != NULL)
            *m_wpMsgId = wMsgID;
    }

    WORD GetMsgId() const { return m_wpMsgId != NULL ? *m_wpMsgId : 0; }
  
    DWORD GetSessionId() const { return m_dwSessionID; }

    void SetReadPos(WORD wPos);

    WORD GetReadPos() const { return m_wReadDataArrayPos; }

    void FlushRemainingBytes() {
        if (m_wWriteDataArrayPos <= m_dwArrayDataSize)
            m_wReadDataArrayPos = m_wWriteDataArrayPos;
    }
    std::n_wstring ReadNStringW();
    void ReadBytes(void *pOut, size_t cbSize);

    void ReadBytesReverse(void *pOut, size_t cbSize);

    void WriteBytes(const void *pIn, size_t cbSize);

    void WriteString(const char *pChar);

    void WriteString(const std::string &str);

    void WriteStringW(const std::wstring& str);

    void WriteString(const std::wstring& str);
    bool TryWriteString(const std::string& str);
    bool TryWriteStringW(const std::wstring& str);

    void ReadString(std::string &str);
    void ReadString(std::string &str, size_t maxLength);
    void ReadStringW(std::wstring& str);
    void ReadStringW(std::wstring& str, size_t maxLength);

    void AddSizeToMsg(WORD wSize);
    void Read(void* dest, __int16 count);
    void Write(const void* src, size_t count);
    bool TryWrite(const void* src, size_t count);
    bool HasWriteOverflow() const { return m_bWriteOverflow != 0; }
    void ResetWriteState() { m_bWriteOverflow = 0; }
public:
    char pad_0000[0x28]; //0x0000
    char *m_pOldMsgBuffer; //0x0028
    char pad_002c[8]; //0x002c
    char m_MsgBuffer[MSG_BUFFER_SIZE]; //0x0034
    char *m_pMsgBuffer; //0x1034
    char *m_pMsgBufferEx; //0x1038 if the m_pMsgBuffer array size is bigger than m_dwArrayDataSize this mem get allocate
    WORD m_wReadDataArrayPos; //0x103c
    WORD m_wWriteDataArrayPos; //0x103e
    char pad_1040[4]; //0x1040
    int m_cMsgUsage; //0x1044
    MSG_READ_MODE m_dwReadMsgMode; //0x1048
    DWORD m_dwArrayDataSize; //0x104c
    WORD *m_wpMsgId; //0x1050
    WORD *m_wpMsgSize; //0x1054
    BYTE *m_bpSequence; //0x1058
    BYTE *m_bpCRC; //0x105C
    DWORD m_dwSessionID; //0x1060
    char pad_1064[4]; //0x1064
    LONG m_lIsAllocated; //0x1068 0 mean freed 1 mean still can use
    BYTE m_bWriteOverflow; //0x106c - owned by the add-on
    char pad_106d[13]; //0x106d
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x107C)
        ENSURE_OFFSET(m_pMsgBuffer, 0x1034)
        ENSURE_OFFSET(m_wReadDataArrayPos, 0x103C)
        ENSURE_OFFSET(m_wWriteDataArrayPos, 0x103E)
        ENSURE_OFFSET(m_dwArrayDataSize, 0x104C)
        ENSURE_OFFSET(m_wpMsgId, 0x1050)
        ENSURE_OFFSET(m_wpMsgSize, 0x1054)
        ENSURE_OFFSET(m_bpSequence, 0x1058)
        ENSURE_OFFSET(m_bpCRC, 0x105C)
        ENSURE_OFFSET(m_dwSessionID, 0x1060)
        ENSURE_OFFSET(m_lIsAllocated, 0x1068)
        ENSURE_OFFSET(m_bWriteOverflow, 0x106C)
    END_FIXTURE()

    RUN_FIXTURE(CMsg)
};
