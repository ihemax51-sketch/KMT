#include "DiscordManager.h"
#include "ICharactor.h"
#include "ICPlayer.h"
#include "TextStringManager.h"
#include <sstream>
#include <BSLib/multibyte.h>
#include <signal.h>
#include <iostream>

DiscordManager* m_dc;

DiscordManager::DiscordManager() : m_IsStarted(false), m_IsRunning(false), m_IsConnected(false), m_GameState(LOADING), m_InGameTimestamp(0) {
}

void DiscordManager::Start(DiscordClientId CLIENT_ID) {
    try {
        m_CLIENT_ID = CLIENT_ID;
        if (!m_IsStarted && !m_IsRunning) {
            m_IsStarted = m_IsRunning = true;
            HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DiscordManager::DiscordThread, 0, 0, 0);
            // Set discord stuffs as background process (below normal)
            SetThreadPriority(hThread, -1);
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

        m_GameState = State;

        if (!m_IsConnected)
            return;

        struct DiscordActivity activity;
        memset(&activity, 0, sizeof(activity));

        switch (State) {
            default:
            case GAME_STATE::LOADING:
                sprintf(activity.state, "Loading");
                sprintf(activity.assets.large_image, "logo");
                sprintf(activity.assets.large_text, "Lexa Online");
                break;
            case GAME_STATE::SERVER_SELECTION:
                sprintf(activity.state, "Selecting Server");
                sprintf(activity.assets.large_image, "logo");
                sprintf(activity.assets.large_text, "Lexa Online");
                break;
            case GAME_STATE::CHARACTER_SELECTION:
                sprintf(activity.state, "Selecting Character");
                sprintf(activity.assets.large_image, "logo");
                sprintf(activity.assets.large_text, "Lexa Online");
                break;
            case GAME_STATE::IN_GAME:
                if(g_pMyPlayerObj != NULL) {
                    switch (g_pMyPlayerObj->GetJobType()) {
                        case 1:
                            sprintf(activity.state, "Job Mode (Trader)");
                            break;
                        case 2:
                            sprintf(activity.state, "Job Mode (Thief)");
                            break;
                        case 3:
                            sprintf(activity.state, "Job Mode (Hunter)");
                            break;
                        default:
                            sprintf(activity.state, "Playing VSRO");
                            break;
                    }

                    std::stringstream details;
                    std::n_wstring nwCharName = g_pMyPlayerObj->GetCharName();
                    std::string charName(nwCharName.begin(), nwCharName.end());
                    details << charName << "\nLv." << (int)g_pMyPlayerObj->m_btLevel;

                    std::string GuildName = TO_STRING(g_pMyPlayerObj->GetGuildName());
                    if (!GuildName.empty())
                        details << "\n[" << GuildName.c_str() << "]";

                    sprintf_s(activity.details, details.str().c_str());

                    if (g_pMyPlayerObj->GetJobType() == TRIJOB_TYPE::TRIJOB_NOJOB) {
                        std::wstringstream region;
                        region << g_pMyPlayerObj->GetRegion().r;
                        const std::n_wstring* nwRegionName = g_CTextStringManager->GetString2(region.str().c_str());
                        std::string regionName(nwRegionName->begin(), nwRegionName->end());
                        sprintf_s(activity.assets.large_text, regionName.c_str());
                    }

                    sprintf(activity.assets.small_image, "logo");
                    sprintf(activity.assets.small_text, "https://lexashield.online/");
                    activity.timestamps.start = m_InGameTimestamp;

                    // Butonları ekleyin
                    strcpy(activity.buttons[0].label, "Join Us");
                    strcpy(activity.buttons[0].url, "https://lexashield.online/join");
                    strcpy(activity.buttons[1].label, "Discord");
                    strcpy(activity.buttons[1].url, "https://discord.gg/yourdiscord");
                }
                break;
        }

        m_App.activities->update_activity(m_App.activities, &activity, m_App.application, UpdateActivityCallback);
    } catch (const std::exception& e) {
        std::cout << "Error updating state: " << e.what() << std::endl;
    }
}

void DiscordManager::Stop() {
    try {
        m_IsStarted = false;
    } catch (const std::exception& e) {
        std::cout << "Error stopping DiscordManager: " << e.what() << std::endl;
    }
}

void OnUserUpdated(void* data) {
    try {
        if (m_dc && m_dc->m_App.users) {
            m_dc->m_App.users->get_current_user(m_dc->m_App.users, &m_dc->m_App.currentUser);
            //printf("Connected user: %s#%s\r\n", m_dc->m_App.currentUser.username, m_dc->m_App.currentUser.discriminator);
            m_dc->UpdateState();
        } else {
            std::cout << "Error: m_dc or m_dc->m_App.users is null" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error in OnUserUpdated: " << e.what() << std::endl;
    }
}

void SignalInterrupt(int code) {
    try {
        m_dc->Stop();
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
            if (!m_dc->m_IsStarted) {
                m_dc->m_IsRunning = false;
                return 0;
            }
            if (result == DiscordResult_Ok) {
                m_dc->m_IsConnected = true;
                break;
            }
            Sleep(30000);
        } while (true);

        m_dc->m_App.users = m_dc->m_App.core->get_user_manager(m_dc->m_App.core);
        m_dc->m_App.achievements = m_dc->m_App.core->get_achievement_manager(m_dc->m_App.core);
        m_dc->m_App.activities = m_dc->m_App.core->get_activity_manager(m_dc->m_App.core);
        m_dc->m_App.application = m_dc->m_App.core->get_application_manager(m_dc->m_App.core);
        m_dc->m_App.lobbies = m_dc->m_App.core->get_lobby_manager(m_dc->m_App.core);

        signal(SIGINT, SignalInterrupt);

        while (m_dc->m_IsStarted) {
            m_dc->m_App.core->run_callbacks(m_dc->m_App.core);
            Sleep(2500);
        }

        m_dc->m_IsConnected = m_dc->m_IsRunning = false;
    } catch (const std::exception& e) {
        std::cout << "Error in DiscordThread: " << e.what() << std::endl;
        m_dc->m_IsRunning = false;
    }
    return 0;
}

void DiscordManager::UpdateState() {
    try {
        UpdateState(m_GameState);
        // Diğer güncelleme işlemleri...
    } catch (const std::exception& e) {
        std::cout << "Error in UpdateState: " << e.what() << std::endl;
    }
}
