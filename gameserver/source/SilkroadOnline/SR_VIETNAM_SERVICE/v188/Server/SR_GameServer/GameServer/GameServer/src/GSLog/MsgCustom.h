#pragma once
#include <string>
#include <memory/MemoryUtility.h>
typedef unsigned short WORD;


class CMsgStreamBufferCustom
{
public:

    WORD GetID() const;
    WORD GetReadPos() const;
    WORD GetWritePos() const;
    void SetMsgID(WORD wMsgID);
    void SetReadPos(WORD wPos);
    void SetWritePos(WORD wPos);
    bool IsValid() const;
    size_t GetRemainingRead() const;


    void Read(void* dest, size_t count);
    std::string ReadStringA();
    std::wstring ReadStringW();

    void Write(const void* src, size_t count);
    void WriteStringA(const std::string& str);
    void WriteStringW(const std::wstring& str);
    //============================================================================

    template<typename T>
    void Read(T& value)
    {
        return Read(&value, sizeof(T));
    }

    template<typename T>
    const T Read()
    {
        T value = T();
        Read<T>(value);
        return value;
    }

    template<typename T>
    void Write(const T& value)
    {
        Write(&value, sizeof(T));
    }

    template<typename T>
    CMsgStreamBufferCustom& operator >> (T& value)
    {
        Read(&value, sizeof(T));
        return *this;
    }

    template<typename T>
    CMsgStreamBufferCustom& operator << (const T& value)
    {
        Write(&value, sizeof(T));
        return *this;
    }
};
