#include "DiscordManager.h"
#include "ICharactor.h"
#include "ICPlayer.h"
#include "TextStringManager.h"
#include <sstream>
#include <BSLib/multibyte.h>
#include <signal.h>
#include <iostream>

DiscordManager* m_dc;

DiscordManager::DiscordManager() : m_IsStarted(0), m_IsRunning(0), m_IsConnected(0), m_GameState(LOADING), m_InGameTimestamp(0), m_Thread(NULL), m_ThreadId(0), m_StopEvent(NULL) {
    InitializeCriticalSection(&m_SnapshotLock);
    m_Snapshot.state = LOADING;
    m_Snapshot.startedAt = 0;
}

DiscordManager::~DiscordManager() {
    Stop();
    DeleteCriticalSection(&m_SnapshotLock);
}

void DiscordManager::Start(DiscordClientId CLIENT_ID) {
    try {
        m_CLIENT_ID = CLIENT_ID;
        if (InterlockedCompareExchange(&m_IsStarted, 1, 0) == 0) {
            m_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!m_StopEvent) {
                InterlockedExchange(&m_IsStarted, 0);
                return;
            }
            InterlockedExchange(&m_IsRunning, 1);
            m_Thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DiscordManager::DiscordThread, 0, 0, &m_ThreadId);
            if (!m_Thread) {
                CloseHandle(m_StopEvent);
                m_StopEvent = NULL;
                InterlockedExchange(&m_IsRunning, 0);
                InterlockedExchange(&m_IsStarted, 0);
                return;
            }
            // Set discord stuffs as background process (below normal)
            SetThreadPriority(m_Thread, THREAD_PRIORITY_BELOW_NORMAL);
        }
    } catch (const std::exception& e) {
        std::cout << "Error starting DiscordManager: " << e.what() << std::endl;
    }
}

void UpdateActivityCallback(void* data, enum EDiscordResult result) {
    if (result != DiscordResult_Ok) {
        std::cout << "Discord Activity (ErrCode " << result << ")" << std::endl;
    }
}

void DiscordManager::UpdateState(GAME_STATE State) {
    try {
        if (m_GameState != GAME_STATE::IN_GAME && State == GAME_STATE::IN_GAME)
            m_InGameTimestamp = std::time(0);

        PresenceSnapshot snapshot;
        snapshot.state = State;
        snapshot.startedAt = m_InGameTimestamp;
        snapshot.largeText = "KMTGuard";
        if (State == IN_GAME && g_pMyPlayerObj != NULL) {
            std::stringstream details;
            const std::n_wstring character = g_pMyPlayerObj->GetCharName();
            details << std::string(character.begin(), character.end()) << " Lv."
                    << static_cast<int>(g_pMyPlayerObj->m_btLevel);
            snapshot.details = details.str();
        }
        EnterCriticalSection(&m_SnapshotLock);
        m_GameState = State;
        m_Snapshot = snapshot;
        LeaveCriticalSection(&m_SnapshotLock);
    } catch (const std::exception& e) {
        std::cout << "Error updating state: " << e.what() << std::endl;
    }
}

void DiscordManager::Stop() {
    try {
        RequestStop();
        if (m_Thread && GetCurrentThreadId() != m_ThreadId) {
            if (WaitForSingleObject(m_Thread, 10000) == WAIT_OBJECT_0) {
                CloseHandle(m_Thread);
                m_Thread = NULL;
                m_ThreadId = 0;
                if (m_StopEvent) {
                    CloseHandle(m_StopEvent);
                    m_StopEvent = NULL;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error stopping DiscordManager: " << e.what() << std::endl;
    }
}

void DiscordManager::RequestStop() {
    InterlockedExchange(&m_IsStarted, 0);
    if (m_StopEvent)
        SetEvent(m_StopEvent);
}

void OnUserUpdated(void* data) {
    try {
        if (m_dc && m_dc->m_App.users) {
            m_dc->m_App.users->get_current_user(m_dc->m_App.users, &m_dc->m_App.currentUser);
            //printf("Connected user: %s#%s\r\n", m_dc->m_App.currentUser.username, m_dc->m_App.currentUser.discriminator);
            m_dc->PublishActivity();
        } else {
            std::cout << "Error: m_dc or m_dc->m_App.users is null" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error in OnUserUpdated: " << e.what() << std::endl;
    }
}

void SignalInterrupt(int code) {
    try {
        if (m_dc)
            m_dc->RequestStop();
    } catch (const std::exception& e) {
        std::cout << "Error in SignalInterrupt: " << e.what() << std::endl;
    }
}

DWORD WINAPI DiscordManager::DiscordThread() {
    try {
        memset(&m_dc->m_App, 0, sizeof(m_dc->m_App));

        struct IDiscordUserEvents users_events;
        memset(&users_events, 0, sizeof(users_events));
        users_events.on_current_user_update = OnUserUpdated;

        struct DiscordCreateParams params;
        DiscordCreateParamsSetDefault(&params);
        params.client_id = m_dc->m_CLIENT_ID;
        params.flags = DiscordCreateFlags_NoRequireDiscord;
        params.event_data = &m_dc->m_App;
        params.user_events = &users_events;

        signal(SIGINT, SignalInterrupt);

        do {
            EDiscordResult result = DiscordCreate(DISCORD_VERSION, &params, &m_dc->m_App.core);
            if (InterlockedCompareExchange(&m_dc->m_IsStarted, 0, 0) == 0) {
                InterlockedExchange(&m_dc->m_IsRunning, 0);
                return 0;
            }
            if (result == DiscordResult_Ok) {
                InterlockedExchange(&m_dc->m_IsConnected, 1);
                break;
            }
            if (WaitForSingleObject(m_dc->m_StopEvent, 30000) == WAIT_OBJECT_0)
                return 0;
        } while (true);

        m_dc->m_App.users = m_dc->m_App.core->get_user_manager(m_dc->m_App.core);
        m_dc->m_App.achievements = m_dc->m_App.core->get_achievement_manager(m_dc->m_App.core);
        m_dc->m_App.activities = m_dc->m_App.core->get_activity_manager(m_dc->m_App.core);
        m_dc->m_App.application = m_dc->m_App.core->get_application_manager(m_dc->m_App.core);
        m_dc->m_App.lobbies = m_dc->m_App.core->get_lobby_manager(m_dc->m_App.core);

        signal(SIGINT, SignalInterrupt);

        while (InterlockedCompareExchange(&m_dc->m_IsStarted, 0, 0) != 0) {
            m_dc->m_App.core->run_callbacks(m_dc->m_App.core);
            m_dc->PublishActivity();
            if (WaitForSingleObject(m_dc->m_StopEvent, 2500) == WAIT_OBJECT_0)
                break;
        }

        if (m_dc->m_App.core)
            m_dc->m_App.core->destroy(m_dc->m_App.core);
        memset(&m_dc->m_App, 0, sizeof(m_dc->m_App));
        InterlockedExchange(&m_dc->m_IsConnected, 0);
        InterlockedExchange(&m_dc->m_IsRunning, 0);
    } catch (const std::exception& e) {
        std::cout << "Error in DiscordThread: " << e.what() << std::endl;
        InterlockedExchange(&m_dc->m_IsRunning, 0);
    }
    return 0;
}

void DiscordManager::UpdateState() {
    try {
        PublishActivity();
    } catch (const std::exception& e) {
        std::cout << "Error in UpdateState: " << e.what() << std::endl;
    }
}

DiscordManager::PresenceSnapshot DiscordManager::GetSnapshot() {
    EnterCriticalSection(&m_SnapshotLock);
    PresenceSnapshot snapshot = m_Snapshot;
    LeaveCriticalSection(&m_SnapshotLock);
    return snapshot;
}

void DiscordManager::PublishActivity() {
    if (InterlockedCompareExchange(&m_IsConnected, 0, 0) == 0 || !m_App.activities)
        return;
    const PresenceSnapshot snapshot = GetSnapshot();
    DiscordActivity activity;
    memset(&activity, 0, sizeof(activity));
    const char* state = "Loading";
    if (snapshot.state == SERVER_SELECTION) state = "Selecting Server";
    else if (snapshot.state == CHARACTER_SELECTION) state = "Selecting Character";
    else if (snapshot.state == IN_GAME) state = "Playing VSRO";
    strncpy_s(activity.state, sizeof(activity.state), state, _TRUNCATE);
    strncpy_s(activity.details, sizeof(activity.details), snapshot.details.c_str(), _TRUNCATE);
    strncpy_s(activity.assets.large_image, sizeof(activity.assets.large_image), "logo", _TRUNCATE);
    strncpy_s(activity.assets.large_text, sizeof(activity.assets.large_text), snapshot.largeText.c_str(), _TRUNCATE);
    activity.timestamps.start = snapshot.startedAt;
    m_App.activities->update_activity(m_App.activities, &activity, m_App.application, UpdateActivityCallback);
}
