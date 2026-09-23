#pragma once
#include <ctime>
#include "discord_game_sdk/discord.h"
#include <Windows.h>
#include <string>

struct DiscordApp {
    struct IDiscordCore* core;
    struct IDiscordUserManager* users;
    struct IDiscordAchievementManager* achievements;
    struct IDiscordActivityManager* activities;
    struct IDiscordRelationshipManager* relationships;
    struct IDiscordApplicationManager* application;
    struct IDiscordLobbyManager* lobbies;
    DiscordUser currentUser;
};

// Game status known
enum GAME_STATE : char
{
    LOADING = 0,
    FINISH = 1,
    SERVER_SELECTION = 2,
    CHARACTER_SELECTION = 3,
    IN_GAME = 4
};

// Discord wrapper class to use the new discord game sdk features
class DiscordManager {
public:
    DiscordManager();
    ~DiscordManager();
    void Start(DiscordClientId CLIENT_ID);
    void UpdateState(GAME_STATE State);
    void Stop();
    void RequestStop();

public:
    DiscordApp m_App;
    DiscordClientId m_CLIENT_ID;
    volatile LONG m_IsStarted;
    volatile LONG m_IsRunning;
    volatile LONG m_IsConnected;
    GAME_STATE m_GameState;
    std::time_t m_InGameTimestamp;
    static DWORD WINAPI DiscordThread();
    void UpdateState();
    void PublishActivity();

private:
    struct PresenceSnapshot {
        GAME_STATE state;
        std::string details;
        std::string largeText;
        std::time_t startedAt;
    };
    PresenceSnapshot GetSnapshot();
    HANDLE m_Thread;
    DWORD m_ThreadId;
    HANDLE m_StopEvent;
    CRITICAL_SECTION m_SnapshotLock;
    PresenceSnapshot m_Snapshot;
};
extern DiscordManager * m_dc;
