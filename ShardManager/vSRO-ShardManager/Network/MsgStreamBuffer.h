#pragma once
#include <string>
#include <Rpc.h>


//NOTE: Currently only supports writing. 
//Reading is not needed at all, at least for now.
class CMsgStreamBuffer
{
public:

    WORD GetID() const;
    WORD GetReadPos() const;
    WORD GetWritePos() const;

    void SetMsgID(WORD wMsgID);
    void SetReadPos(WORD wPos);
    void SetWritePos(WORD wPos);


    void Read(void* dest, __int16 count);
    std::string ReadStringA();
	bool CanRead(size_t count) const;
	bool TryReadStringA(std::string& value, size_t maximumLength);

    void Write(const void* src, size_t count);
    void WriteStringA(const std::string& str);
	bool CanWrite(size_t count) const;
	bool TryWriteStringA(const std::string& str, size_t maximumLength);

    //============================================================================

    template<typename T>
    void Read(T& value)
    {
        return Read(&value, sizeof(T));
    }

    template<typename T>
    const T Read()
    {
        T value;
        Read<T>(value);
        return value;
    }

	template<typename T>
	bool TryRead(T& value)
	{
		if (!CanRead(sizeof(T)))
			return false;
		Read<T>(value);
		return true;
	}

    template<typename T>
    void Write(const T& value)
    {
        Write(&value, sizeof(T));
    }

    template<typename T>
    CMsgStreamBuffer& operator >> (T& value)
    {
        Read(&value, sizeof(T));
        return *this;
    }

    template<typename T>
    CMsgStreamBuffer& operator << (const T& value)
    {
        Write(&value, sizeof(T));
        return *this;
    }
};
