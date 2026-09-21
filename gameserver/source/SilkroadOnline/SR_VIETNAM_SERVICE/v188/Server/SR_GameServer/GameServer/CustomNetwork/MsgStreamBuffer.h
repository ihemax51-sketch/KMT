#pragma once

#include <memory/hook.h>
#include <memory/RemodelUtility.h>
#include <SettingMgr/NewSettings.h>
#include <memory/MemoryUtility.h>

#define MSG_NODE_BUFFER_SIZE 4096

//TODO: Check if the opcode is supported?
#define NEWMSG(id) \
	new CMsgStreamBuffer(id);

#define SENDMSG(msg) \
	msg->Send();

struct SMsgStreamNode;


class CMsgStreamBuffer
{
private:
    typedef const CMsgStreamBuffer* (*FN_CONSTRUCTOR)(CMsgStreamBuffer*, uint16_t id);
    typedef size_t (*FN_READ)(CMsgStreamBuffer*, void* dest, size_t count);
    typedef void* (*FN_WRITE)(CMsgStreamBuffer*, const void* src, size_t count);
    typedef void* (*FN_SEND)(CMsgStreamBuffer*);

    static FN_CONSTRUCTOR s_pfnConstructor;
    static FN_READ s_pfnRead;
    static FN_WRITE s_pfnWrite;
    static FN_SEND s_pfnSend;

#pragma pack(push, 1)
    union
    {
        struct
        {
            size_t m_offset;
            size_t m_length;
            DWORD m_unk;
            SMsgStreamNode* m_pFrontNode;
            SMsgStreamNode* m_pCurNode;
            uint16_t m_id;
        };
    };
#pragma pack(pop)

public:
    static void Initialize();
    CMsgStreamBuffer(uint16_t id);
    REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(CMsgStreamBuffer);

    size_t GetOffset() const;
    size_t GetLength() const;
    size_t GetRemainRead() const;
    uint16_t GetID() const;
    SMsgStreamNode* GetFrontNode() const;
    SMsgStreamNode* GetCurrentNode() const;
    size_t GetNodeCount() const;

    size_t Read(void* dest, size_t count);
    std::string ReadStringA();
    std::wstring ReadStringW();
    void* Write(const void* src, size_t count);
    void WriteStringA(const std::string& str);
    void WriteStringW(const std::wstring& str);

    void Seek(size_t offset);
    void Skip(size_t count);
    void Send();

    //============================================================================
    template<typename T>
    size_t Read(T& value)
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
    void* Write(const T& value)
    {
        return Write(&value, sizeof(T));
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
    //============================================================================

}; //sizeof = 28


