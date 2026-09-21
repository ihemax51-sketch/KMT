#include "UniqueLogQueue.h"

#include <Windows.h>
#include <deque>
#include <sstream>
#include "../Database/SQLConnection.h"
#include "../Database/SQLCommand.h"
#include "../Utils/BSObj.h"

namespace
{
    const size_t MAX_PENDING_EVENTS = 4096;
    const DWORD WORKER_SHUTDOWN_TIMEOUT_MS = 35000;

    enum WorkerState
    {
        WORKER_STOPPED = 0,
        WORKER_RUNNING = 1,
        WORKER_STOPPING = 2
    };

    struct UniqueEvent
    {
        bool killed;
        unsigned long refObjectId;
        std::string killerName;
        unsigned retryCount;
    };

    CRITICAL_SECTION s_lock;
    volatile LONG s_lockInitialized = 0;
    volatile LONG s_workerState = WORKER_STOPPED;
    HANDLE s_event = NULL;
    HANDLE s_thread = NULL;
    SQLCommand* volatile s_activeCommand = NULL;
    std::wstring s_connectionString;
    std::deque<UniqueEvent> s_queue;

    void EnsureLock()
    {
        if (InterlockedCompareExchange(&s_lockInitialized, 1, 0) == 0)
            InitializeCriticalSection(&s_lock);
    }

    std::wstring EscapeSqlText(const std::string& text)
    {
        std::wstring result;
        result.reserve(text.size() + 8);
        for (std::string::const_iterator it = text.begin(); it != text.end(); ++it)
        {
            const unsigned char value = static_cast<unsigned char>(*it);
            if (value == '\'')
                result.append(L"''");
            else if (value >= 0x20 && value < 0x7F)
                result.push_back(static_cast<wchar_t>(value));
        }
        return result;
    }

    bool TryPop(UniqueEvent& value)
    {
        EnterCriticalSection(&s_lock);
        if (s_queue.empty())
        {
            LeaveCriticalSection(&s_lock);
            return false;
        }
        value = s_queue.front();
        s_queue.pop_front();
        LeaveCriticalSection(&s_lock);
        return true;
    }

    void RetryOrDrop(const UniqueEvent& value)
    {
        if (value.retryCount >= 2)
        {
            BS_ERROR("Dropped a unique history event after repeated database failures");
            return;
        }

        UniqueEvent retry = value;
        ++retry.retryCount;
        EnterCriticalSection(&s_lock);
        s_queue.push_front(retry);
        LeaveCriticalSection(&s_lock);
    }

    bool IsWorkerRunning()
    {
        return InterlockedCompareExchange(
            &s_workerState, WORKER_RUNNING, WORKER_RUNNING) == WORKER_RUNNING;
    }

    bool WaitForStopOrTimeout(DWORD timeoutMilliseconds)
    {
        return s_event != NULL &&
            WaitForSingleObject(s_event, timeoutMilliseconds) == WAIT_OBJECT_0;
    }

    DWORD WINAPI Worker(LPVOID)
    {
        SQLConnection connection;
        SQLCommand command;

        s_activeCommand = &command;
        while (IsWorkerRunning())
        {
            WaitForSingleObject(s_event, 5000);

            UniqueEvent value;
            while (IsWorkerRunning() && TryPop(value))
            {
                if (!connection.IsOpen() &&
                    (!connection.Open(const_cast<SQLWCHAR*>(s_connectionString.c_str())) ||
                     !command.Open(connection)))
                {
                    RetryOrDrop(value);
                    WaitForStopOrTimeout(2000);
                    break;
                }

                std::wstringstream query;
                if (value.killed)
                {
                    query << L"EXEC [KMTGuard].[dbo].[Hook_UniqueKill] "
                          << value.refObjectId << L", N'" << EscapeSqlText(value.killerName) << L"'";
                }
                else
                {
                    query << L"EXEC [KMTGuard].[dbo].[Hook_UniqueSpawn] " << value.refObjectId;
                }

                if (!command.ExecuteQuery(const_cast<SQLWCHAR*>(query.str().c_str())))
                {
                    command.Close();
                    connection.Close();
                    RetryOrDrop(value);
                    WaitForStopOrTimeout(1000);
                    break;
                }
                command.Clear();
            }
        }

        command.Close();
        connection.Close();
        s_activeCommand = NULL;
        InterlockedExchange(&s_workerState, WORKER_STOPPED);
        return 0;
    }

    bool Enqueue(const UniqueEvent& value)
    {
        if (!IsWorkerRunning())
            return false;

        EnterCriticalSection(&s_lock);
        if (s_queue.size() >= MAX_PENDING_EVENTS)
        {
            LeaveCriticalSection(&s_lock);
            BS_WARNING("Unique history queue is full; event rejected");
            return false;
        }
        s_queue.push_back(value);
        LeaveCriticalSection(&s_lock);
        SetEvent(s_event);
        return true;
    }
}

bool UniqueLogQueue::Initialize(const std::wstring& connectionString)
{
    if (connectionString.empty())
        return false;
    EnsureLock();
    if (s_thread != NULL || s_event != NULL)
        return false;
    const LONG previousState = InterlockedCompareExchange(
        &s_workerState, WORKER_RUNNING, WORKER_STOPPED);
    if (previousState != WORKER_STOPPED)
        return previousState == WORKER_RUNNING;

    s_connectionString = connectionString;
    s_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (s_event == NULL)
    {
        InterlockedExchange(&s_workerState, WORKER_STOPPED);
        return false;
    }

    s_thread = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
    if (s_thread == NULL)
    {
        CloseHandle(s_event);
        s_event = NULL;
        InterlockedExchange(&s_workerState, WORKER_STOPPED);
        return false;
    }

    BS_INFO("Unique history worker started");
    return true;
}

void UniqueLogQueue::Shutdown()
{
    const LONG previousState = InterlockedCompareExchange(
        &s_workerState, WORKER_STOPPING, WORKER_RUNNING);
    if (previousState == WORKER_STOPPED && s_thread == NULL)
        return;
    if (s_event != NULL)
        SetEvent(s_event);
    SQLCommand* activeCommand = s_activeCommand;
    if (activeCommand != NULL)
        activeCommand->Cancel();

    if (s_thread != NULL)
    {
        const DWORD waitResult = WaitForSingleObject(
            s_thread, WORKER_SHUTDOWN_TIMEOUT_MS);
        if (waitResult == WAIT_TIMEOUT)
        {
            BS_ERROR("Unique history worker did not stop before the shutdown deadline; runtime state was retained");
            return;
        }
        if (waitResult == WAIT_FAILED)
        {
            BS_ERROR("Unique history worker shutdown wait failed; runtime state was retained");
            return;
        }
        if (waitResult != WAIT_OBJECT_0)
        {
            BS_ERROR("Unique history worker returned an unexpected shutdown wait result; runtime state was retained");
            return;
        }
        CloseHandle(s_thread);
        s_thread = NULL;
    }
    if (s_event != NULL)
    {
        CloseHandle(s_event);
        s_event = NULL;
    }
    EnterCriticalSection(&s_lock);
    s_queue.clear();
    LeaveCriticalSection(&s_lock);
    s_connectionString.clear();
    InterlockedExchange(&s_workerState, WORKER_STOPPED);
}

bool UniqueLogQueue::EnqueueSpawn(unsigned long refObjectId)
{
    UniqueEvent value = { false, refObjectId, std::string(), 0 };
    return Enqueue(value);
}

bool UniqueLogQueue::EnqueueKill(unsigned long refObjectId, const std::string& killerName)
{
    if (killerName.empty() || killerName.size() > 64)
        return false;
    UniqueEvent value = { true, refObjectId, killerName, 0 };
    return Enqueue(value);
}
