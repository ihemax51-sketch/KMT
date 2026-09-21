#pragma once
#include <ctime>
#include "discord_game_sdk/discord.h"
#include <Windows.h>

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
    void Start(DiscordClientId CLIENT_ID);
    void UpdateState(GAME_STATE State);
    void Stop();

public:
    DiscordApp m_App;
    DiscordClientId m_CLIENT_ID;
    bool m_IsStarted;
    bool m_IsRunning;
    bool m_IsConnected;
    GAME_STATE m_GameState;
    std::time_t m_InGameTimestamp;
    static DWORD WINAPI DiscordThread();
    void UpdateState();
};
extern DiscordManager * m_dc;