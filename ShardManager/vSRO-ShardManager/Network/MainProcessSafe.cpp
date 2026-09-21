#include "MainProcess.h"
#include "ShardNetManager.h"
#include "../Runtime/UniqueLogQueue.h"
#include "../Utils/BSObj.h"

#include <Windows.h>
#include <KmtGuardProductVersion.h>
#include <cstdio>
#include <stdexcept>

namespace
{
    const WORD READ_LOCK_INFO = 0x5060;
    const WORD SEND_LOCK_INFO = 0x5061;
    const WORD READ_UNLOCK_INFO = 0x5062;
    const WORD SEND_UNLOCK_INFO = 0x5063;
    const WORD READ_TIMED_ITEM = 0x5065;
    const WORD SEND_TIMED_ITEM = 0x5066;
    const WORD READ_TIMED_ITEM_REMOVE = 0x5067;
    const WORD SEND_TIMED_ITEM_REMOVE = 0x5068;
    const WORD READ_LINKED_CHAT = 0x5068;
    const WORD SEND_LINKED_CHAT = 0x5069;

    const DWORD ORIGINAL_PROCESS_MESSAGE = 0x0040DDA0;
    const DWORD ORIGINAL_HANDLE_MESSAGE = 0x006990D0;
    const WORD SERVER_LOG_MESSAGE = 0x200A;
    const int LOG_TYPE_NOTIFY = 0;
    const char* SHARD_READY_MESSAGE = "SR_ShardManager is initialized successfully";

    typedef int(__thiscall* OriginalHandleMessage)(
        CMainProcess*, CMsgStreamBuffer*, DWORD, LPVOID, CMassiveMsg*);
    OriginalHandleMessage s_originalHandleMessage =
        reinterpret_cast<OriginalHandleMessage>(ORIGINAL_HANDLE_MESSAGE);
    volatile LONG s_readyStampWritten = 0;

    struct NativeLogSnapshot
    {
        WORD readPosition;
        WORD writePosition;
        int type;
        std::string text;

        NativeLogSnapshot()
            : readPosition(0), writePosition(0), type(0)
        {
        }
    };

    bool TryCaptureShardReadyLog(
        CMsgStreamBuffer* message,
        NativeLogSnapshot& snapshot)
    {
        if (message == NULL || message->GetID() != SERVER_LOG_MESSAGE)
            return false;

        snapshot.readPosition = message->GetReadPos();
        snapshot.writePosition = message->GetWritePos();
        if (snapshot.writePosition < 6)
            return false;

        message->SetReadPos(6);
        const bool valid =
            message->TryRead(snapshot.type) &&
            message->TryReadStringA(snapshot.text, 256) &&
            message->GetReadPos() == snapshot.writePosition;
        message->SetReadPos(snapshot.readPosition);
        return valid && snapshot.text == SHARD_READY_MESSAGE;
    }

    bool RewriteNativeLog(
        CMsgStreamBuffer* message,
        int type,
        const std::string& text)
    {
        if (message == NULL || text.size() > 256)
            return false;

        message->SetReadPos(6);
        message->SetWritePos(6);
        message->Write<int>(type);
        if (!message->TryWriteStringA(text, 256))
            return false;

        const size_t expectedSize = 6 + sizeof(type) + sizeof(WORD) + text.size();
        return message->GetWritePos() == expectedSize;
    }

    void RestoreNativeLog(
        CMsgStreamBuffer* message,
        const NativeLogSnapshot& snapshot)
    {
        RewriteNativeLog(message, snapshot.type, snapshot.text);
        message->SetReadPos(snapshot.readPosition);
    }

    void WriteShardManagerReadyStamp(
        CMainProcess* self,
        CMsgStreamBuffer* message,
        DWORD overlappedJobId,
        LPVOID parameter,
        CMassiveMsg* massiveMessage,
        const NativeLogSnapshot& snapshot)
    {
        char versionLine[96] = { 0 };
        _snprintf_s(
            versionLine,
            sizeof(versionLine),
            _TRUNCATE,
            "========== KMTGuard v%s ==========",
            KMTGUARD_VERSION_STRING);

        const char* lines[] =
        {
            versionLine,
            "Protection and command routing are ACTIVE",
            "ShardManager startup completed successfully"
        };

        for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i)
        {
            if (!RewriteNativeLog(message, LOG_TYPE_NOTIFY, lines[i]))
                break;
            s_originalHandleMessage(
                self, message, overlappedJobId, parameter, massiveMessage);
        }
        RestoreNativeLog(message, snapshot);
    }

    void CallOriginalProcessMessage(
        CMainProcess* self,
        CMsgStreamBuffer* message,
        DWORD overlappedJobId,
        LPVOID parameter,
        CMassiveMsg* massiveMessage)
    {
        reinterpret_cast<void(__thiscall*)(
            CMainProcess*, CMsgStreamBuffer*, DWORD, LPVOID, CMassiveMsg*)>(
                ORIGINAL_PROCESS_MESSAGE)(
                    self, message, overlappedJobId, parameter, massiveMessage);
    }

    bool TryForwardItemState(CMsgStreamBuffer* message, WORD outgoingId)
    {
        INT64 itemId = 0;
        if (!message->TryRead(itemId))
            return false;

        CMsgStreamBuffer* broadcast = CShardNetManager::AllocMsgForGS();
        broadcast->SetMsgID(outgoingId);
        *broadcast << itemId;
        CShardNetManager::BroadcastMsgToGameServers(broadcast);
        return true;
    }

    bool TryForwardTimedItem(CMsgStreamBuffer* message)
    {
        int characterId = 0;
        INT64 itemId = 0;
        int oldOptionLevel = 0;
        long endTime = 0;
        if (!message->TryRead(characterId) ||
            !message->TryRead(itemId) ||
            !message->TryRead(oldOptionLevel) ||
            !message->TryRead(endTime))
            return false;

        CMsgStreamBuffer* broadcast = CShardNetManager::AllocMsgForGS();
        broadcast->SetMsgID(SEND_TIMED_ITEM);
        *broadcast << characterId << itemId << oldOptionLevel << endTime;
        CShardNetManager::BroadcastMsgToGameServers(broadcast);
        return true;
    }

    bool TryForwardLinkedChat(CMsgStreamBuffer* message)
    {
        int senderId = 0;
        BYTE chatType = 0;
        BYTE chatIndex = 0;
        BYTE linkedItemSlot = 0;
        std::string receiverName;
        std::string text;

        if (!message->TryRead(senderId) ||
            !message->TryRead(chatType) ||
            !message->TryRead(chatIndex) ||
            chatType == 0 || chatType > 16)
            return false;

        if (chatType == 2 && !message->TryReadStringA(receiverName, 64))
            return false;
        if (!message->TryReadStringA(text, 1024) || !message->TryRead(linkedItemSlot))
            return false;

        CMsgStreamBuffer* broadcast = CShardNetManager::AllocMsgForGS();
        broadcast->SetMsgID(SEND_LINKED_CHAT);
        *broadcast << senderId << chatType << chatIndex;
        if (chatType == 2 && !broadcast->TryWriteStringA(receiverName, 64))
            return false;
        if (!broadcast->TryWriteStringA(text, 1024))
            return false;
        *broadcast << linkedItemSlot;
        CShardNetManager::BroadcastMsgToGameServers(broadcast);
        return true;
    }

    void InspectUniqueEvent(CMsgStreamBuffer* message)
    {
        if (message == NULL || message->GetID() != 0x7808)
            return;

        const WORD oldReadPosition = message->GetReadPos();
        message->SetReadPos(6);

        WORD containedOpcode = 0;
        WORD operation = 0;
        DWORD refObjectId = 0;
        bool valid = message->TryRead(containedOpcode) &&
            message->TryRead(operation) &&
            containedOpcode == 0x300C;

        if (valid && operation == 0x0C05 && message->TryRead(refObjectId))
        {
            if (!UniqueLogQueue::EnqueueSpawn(refObjectId))
                BS_WARNING("Unable to queue a unique spawn history event");
        }
        else if (valid && operation == 0x0C06 && message->TryRead(refObjectId))
        {
            std::string killerName;
            if (!message->TryReadStringA(killerName, 64) ||
                !UniqueLogQueue::EnqueueKill(refObjectId, killerName))
                BS_WARNING("Rejected an invalid unique kill history event");
        }

        message->SetReadPos(oldReadPosition);
    }

    int __fastcall SafeHandleMessage(
        CMainProcess* self,
        void*,
        CMsgStreamBuffer* message,
        DWORD overlappedJobId,
        LPVOID parameter,
        CMassiveMsg* massiveMessage)
    {
        NativeLogSnapshot readyLog;
        bool shardManagerReady = false;
        try
        {
            InspectUniqueEvent(message);
            shardManagerReady = TryCaptureShardReadyLog(message, readyLog);
        }
        catch (const std::exception& exception)
        {
            BS_ERROR("Unique event inspection failed: %s", exception.what());
        }
        catch (...)
        {
            BS_ERROR("Unique event inspection failed with an unknown error");
        }

        const int result = s_originalHandleMessage(
            self, message, overlappedJobId, parameter, massiveMessage);

        if (shardManagerReady &&
            InterlockedCompareExchange(&s_readyStampWritten, 1, 0) == 0)
        {
            try
            {
                WriteShardManagerReadyStamp(
                    self,
                    message,
                    overlappedJobId,
                    parameter,
                    massiveMessage,
                    readyLog);
            }
            catch (const std::exception& exception)
            {
                RestoreNativeLog(message, readyLog);
                BS_ERROR("ShardManager ready stamp failed: %s", exception.what());
            }
            catch (...)
            {
                RestoreNativeLog(message, readyLog);
                BS_ERROR("ShardManager ready stamp failed with an unknown error");
            }
        }

        return result;
    }
}

void CMainProcess::_OnProcessMessageSafe(
    CMsgStreamBuffer* message,
    DWORD overlappedJobId,
    LPVOID parameter,
    CMassiveMsg* massiveMessage)
{
    if (message == NULL)
    {
        BS_WARNING("Rejected a null ShardManager message");
        return;
    }

    const WORD messageId = message->GetID();
    bool customMessage = true;
    bool forwarded = false;

    try
    {
        switch (messageId)
        {
        case READ_LOCK_INFO:
            forwarded = TryForwardItemState(message, SEND_LOCK_INFO);
            break;
        case READ_UNLOCK_INFO:
            forwarded = TryForwardItemState(message, SEND_UNLOCK_INFO);
            break;
        case READ_TIMED_ITEM:
            forwarded = TryForwardTimedItem(message);
            break;
        case READ_TIMED_ITEM_REMOVE:
            forwarded = TryForwardItemState(message, SEND_TIMED_ITEM_REMOVE);
            break;
        case READ_LINKED_CHAT:
            forwarded = TryForwardLinkedChat(message);
            break;
        default:
            customMessage = false;
            break;
        }
    }
    catch (const std::exception& exception)
    {
        BS_ERROR("ShardManager custom message 0x%04X failed: %s", messageId, exception.what());
        return;
    }
    catch (...)
    {
        BS_ERROR("ShardManager custom message 0x%04X failed with an unknown error", messageId);
        return;
    }

    if (customMessage)
    {
        if (!forwarded)
            BS_WARNING("Rejected malformed ShardManager message 0x%04X", messageId);
        return;
    }

    CallOriginalProcessMessage(
        this, message, overlappedJobId, parameter, massiveMessage);
}

bool CMainProcess::InitializeSafe()
{
    LONG result = DetourTransactionBegin();
    if (result != NO_ERROR)
        return false;
    result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR)
        result = DetourAttach(&(PVOID&)s_originalHandleMessage, SafeHandleMessage);
    if (result != NO_ERROR)
    {
        DetourTransactionAbort();
        return false;
    }
    result = DetourTransactionCommit();
    if (result != NO_ERROR)
        return false;

    BS_INFO("ShardManager message guards installed");
    return true;
}

void CMainProcess::ShutdownSafe()
{
    if (s_originalHandleMessage == NULL)
        return;
    if (DetourTransactionBegin() != NO_ERROR)
        return;
    if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR ||
        DetourDetach(&(PVOID&)s_originalHandleMessage, SafeHandleMessage) != NO_ERROR)
    {
        DetourTransactionAbort();
        return;
    }
    DetourTransactionCommit();
}
