#include "ItemLockPersistence.h"

#include <SettingMgr/NewSettings.h>
#include <SqlConnection/DbConnection.h>
#include <SqlConnection/sqlCon.h>
#include <deque>
#include <map>
#include <set>

namespace
{
    const size_t QUEUE_CAPACITY = 256;
    CRITICAL_SECTION s_lock;
    HANDLE s_stopEvent = NULL;
    HANDLE s_wakeEvent = NULL;
    HANDLE s_thread = NULL;
    volatile LONG s_nextOperationId = 0;
    // 0=stopped, 1=running, 2=terminal after a shutdown timeout.
    LONG s_state = 0;
    std::deque<ItemLockPersistence::Completion> s_requests;
    std::deque<ItemLockPersistence::Completion> s_completions;
    std::map<INT64, int> s_pendingItems;
    std::map<int, unsigned int> s_pendingCharacters;

    struct LockLifetime
    {
        LockLifetime() { InitializeCriticalSection(&s_lock); }
        ~LockLifetime() { DeleteCriticalSection(&s_lock); }
    } s_lockLifetime;

    class Guard
    {
    public:
        Guard() { EnterCriticalSection(&s_lock); }
        ~Guard() { LeaveCriticalSection(&s_lock); }
    private:
        Guard(const Guard&);
        Guard& operator=(const Guard&);
    };

    DWORD WINAPI Worker(LPVOID)
    {
        CDbConnection connection(CNewSettings::m_Settings->DatabaseConnectionString);
        const bool connected = connection.Connect();
        HANDLE waits[2] = { s_stopEvent, s_wakeEvent };
        for (;;)
        {
            const DWORD waitResult = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (waitResult == WAIT_OBJECT_0)
                break;

            for (;;)
            {
                ItemLockPersistence::Completion work;
                {
                    Guard guard;
                    if (s_requests.empty())
                        break;
                    work = s_requests.front();
                    s_requests.pop_front();
                }

                work.resultCode = connected
                    ? CSqlCon::SetItemLockState(&connection, work.itemId, work.lockItem)
                    : CSqlCon::ITEM_LOCK_STATE_FAILED;
                work.succeeded = work.resultCode == CSqlCon::ITEM_LOCK_STATE_CHANGED;
                {
                    Guard guard;
                    s_completions.push_back(work);
                }
            }
        }
        connection.Disconnect();
        return 0;
    }
}

bool ItemLockPersistence::Initialize()
{
    Guard guard;
    if (s_state != 0)
        return s_state == 1;
    s_stopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    s_wakeEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (s_stopEvent == NULL || s_wakeEvent == NULL)
    {
        if (s_wakeEvent != NULL) CloseHandle(s_wakeEvent);
        if (s_stopEvent != NULL) CloseHandle(s_stopEvent);
        s_wakeEvent = NULL; s_stopEvent = NULL;
        return false;
    }
    s_thread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
    if (s_thread == NULL)
    {
        CloseHandle(s_wakeEvent); CloseHandle(s_stopEvent);
        s_wakeEvent = NULL; s_stopEvent = NULL;
        return false;
    }
    s_state = 1;
    return true;
}

void ItemLockPersistence::Shutdown()
{
    HANDLE thread = NULL;
    {
        Guard guard;
        if (s_state != 1)
            return;
        s_state = 0;
        SetEvent(s_stopEvent);
        thread = s_thread;
    }
    if (thread != NULL && WaitForSingleObject(thread, 35000) == WAIT_OBJECT_0)
    {
        Guard guard;
        CloseHandle(s_thread); s_thread = NULL;
        CloseHandle(s_wakeEvent); s_wakeEvent = NULL;
        CloseHandle(s_stopEvent); s_stopEvent = NULL;
        s_requests.clear(); s_completions.clear(); s_pendingItems.clear();
        s_pendingCharacters.clear();
    }
    else
    {
        // Process-lifetime terminal state: keep every object used by the live
        // worker and prevent a duplicate writer from being initialized.
        Guard guard;
        s_state = 2;
    }
}

bool ItemLockPersistence::Enqueue(DWORD playerGameId, int characterId, INT64 itemId,
                                  INT64 scrollItemId, int itemSlot, int scrollSlot,
                                  bool lockItem)
{
    Completion request;
    request.operationId = static_cast<unsigned __int64>(
        static_cast<unsigned long>(InterlockedIncrement(&s_nextOperationId)));
    request.playerGameId = playerGameId;
    request.characterId = characterId;
    request.itemId = itemId;
    request.scrollItemId = scrollItemId;
    request.itemSlot = itemSlot;
    request.scrollSlot = scrollSlot;
    request.lockItem = lockItem;
    request.succeeded = false;
    request.resultCode = CSqlCon::ITEM_LOCK_STATE_FAILED;
    {
        Guard guard;
        if (s_state != 1 || s_pendingItems.size() >= QUEUE_CAPACITY ||
            s_pendingItems.find(itemId) != s_pendingItems.end())
            return false;
        if (s_pendingCharacters[characterId] >= 2)
            return false;
        s_pendingItems[itemId] = characterId;
        ++s_pendingCharacters[characterId];
        s_requests.push_back(request);
    }
    SetEvent(s_wakeEvent);
    return true;
}

bool ItemLockPersistence::TryTakeCompletion(Completion& completion)
{
    Guard guard;
    if (s_completions.empty())
        return false;
    completion = s_completions.front();
    s_completions.pop_front();
    return true;
}

void ItemLockPersistence::Release(INT64 itemId)
{
    Guard guard;
    std::map<INT64, int>::iterator item = s_pendingItems.find(itemId);
    if (item == s_pendingItems.end()) return;
    std::map<int, unsigned int>::iterator character = s_pendingCharacters.find(item->second);
    if (character != s_pendingCharacters.end())
    {
        if (character->second > 1) --character->second;
        else s_pendingCharacters.erase(character);
    }
    s_pendingItems.erase(item);
}
