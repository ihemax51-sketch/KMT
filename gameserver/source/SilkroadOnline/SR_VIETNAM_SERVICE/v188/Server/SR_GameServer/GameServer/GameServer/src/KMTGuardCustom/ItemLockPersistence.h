#pragma once

#include <Windows.h>

namespace ItemLockPersistence
{
    struct Completion
    {
        unsigned __int64 operationId;
        DWORD playerGameId;
        int characterId;
        INT64 itemId;
        INT64 scrollItemId;
        int itemSlot;
        int scrollSlot;
        bool lockItem;
        bool succeeded;
        int resultCode;
    };

    bool Initialize();
    void Shutdown();
    bool Enqueue(DWORD playerGameId, int characterId, INT64 itemId,
                 INT64 scrollItemId, int itemSlot, int scrollSlot, bool lockItem);
    bool TryTakeCompletion(Completion& completion);
    void Release(INT64 itemId);
}
