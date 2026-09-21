#include <CustomData/CustomSettingManager.h>
#include "IFPlayerMiniInfo.h"
#include "GInterface.h"
#include "IFTargetWindow.h"
#include "ExtraUI/DesktopCharacterHud.h"
#include "Game.h"
#include "ICPlayer.h"
#include "IFStatic.h"
#include "Menu/IFMenu.h"
#include "GlobalHelpersThatHaveNoHomeYet.h"

#include <cstdio>

namespace
{
    bool LoadDesktopHudEnabledSetting()
    {
        char settingsPath[0x200];
        sprintf(settingsPath, "%s\\Setting\\Client_Extra.txt", theApp.GetWorkingDir());

        FILE* settingsFile = fopen(settingsPath, "r");
        if (settingsFile == NULL)
            return false;

        bool enabled = false;
        char line[512];
        while (fgets(line, sizeof(line), settingsFile) != NULL)
        {
            int value = 0;
            if (sscanf(line, "Enable HUD: %d", &value) == 1)
            {
                enabled = value != 0;
                break;
            }
        }

        fclose(settingsFile);
        return enabled;
    }

    void RefreshDesktopHudPortraitWhenReady()
    {
        static CICPlayer* trackedPlayer = NULL;
        static DWORD stableSince = 0;
        static DWORD lastAttempt = 0;
        static int trackedRefObjectId = 0;
        static int loadedRefObjectId = 0;
        static int failedAttempts = 0;

        CICPlayer* player = g_pMyPlayerObj;
        if (player == NULL || player->GetCommonData() == NULL)
        {
            trackedPlayer = NULL;
            stableSince = 0;
            lastAttempt = 0;
            trackedRefObjectId = 0;
            loadedRefObjectId = 0;
            failedAttempts = 0;
            return;
        }

        const DWORD now = GetTickCount();
        const int refObjectId = player->GetCommonData()->RefObjectId;
        if (player != trackedPlayer || refObjectId != trackedRefObjectId)
        {
            trackedPlayer = player;
            trackedRefObjectId = refObjectId;
            stableSince = now;
            lastAttempt = 0;
            loadedRefObjectId = 0;
            failedAttempts = 0;
            return;
        }

        if (refObjectId <= 0 || now - stableSince < 3000)
            return;

        // The HUD snapshot publisher can clear its portrait pixels after a
        // transient player-read failure.  Only suppress retries while the
        // pixels for this exact character are still present.
        if (loadedRefObjectId == refObjectId &&
            DesktopCharacterHud::HasPortraitForRefObjectId(refObjectId))
            return;

        const DWORD retryDelay = failedAttempts < 3 ? 2000 : 15000;
        if (lastAttempt != 0 && now - lastAttempt < retryDelay)
            return;

        lastAttempt = now;
        bool portraitLoaded = false;
        CIFMenu* menu = g_pCGInterface != NULL
            ? g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)
            : NULL;
        if (menu != NULL && menu->m_profileFace != NULL)
        {
            const std::n_string& portraitPath =
                menu->m_profileFace->GetBGFilename();
            if (!portraitPath.empty())
            {
                portraitLoaded =
                    DesktopCharacterHud::RefreshPortraitFromTexturePath(
                        refObjectId, portraitPath.c_str());
            }
        }

        if (!portraitLoaded)
            portraitLoaded =
                DesktopCharacterHud::RefreshPortraitForRefObjectId(refObjectId);

        if (portraitLoaded)
        {
            loadedRefObjectId = refObjectId;
            failedAttempts = 0;
        }
        else
        {
            ++failedAttempts;
        }
    }
}

#define GDR_PMI_GAUGE_HP 0
#define GDR_PMI_GAUGE_MP 1
#define GDR_PMI_TXT_ID 3
#define GDR_PMI_TXT_LEVEL 4
#define GDR_PMI_TXT_HP 5
#define GDR_PMI_TXT_MP 6
#define GDR_PMI_CIRCLE0 7
#define GDR_PMI_CIRCLE1 8
#define GDR_PMI_CIRCLE2 9
#define GDR_PMI_CIRCLE3 10
#define GDR_PMI_CIRCLE4 11
#define GDR_PMI_PICTURE 12
#define GDR_PMI_BTN_JAHWAN 13
#define GDR_PMI_PARTY_LEADER 14
#define GDR_PMI_RACE_MARK 15
#define GDR_PMI_BTN_CINFO 20
#define GDR_PMI_STA_CINFOBG 21
#define GDR_PMI_TXT_PHYATT 22
#define GDR_PMI_TXT_PHYDEF 23
#define GDR_PMI_TXT_MAGATT 24
#define GDR_PMI_TXT_MAGDEF 25
#define GDR_PMI_TXT_HIT 26
#define GDR_PMI_TXT_PARRY 27
#define GDR_PMI_TXT_PHYATTDAT 28
#define GDR_PMI_TXT_PHYDEFDAT 29
#define GDR_PMI_TXT_MAGATTDAT 30
#define GDR_PMI_TXT_MAGDEFDAT 31
#define GDR_PMI_TXT_HITDAT 32
#define GDR_PMI_TXT_PARRYDAT 33
#define GDR_PMI_BTN_STATUP 34
#define GDR_PMI_TXT_STR 35
#define GDR_PMI_TXT_INT 36
#define GDR_PMI_TXT_STRDAT 37
#define GDR_PMI_TXT_INTDAT 38
#define GDR_PMI_DECO_HWAN_SLOT0 40
#define GDR_PMI_DECO_HWAN_SLOT1 41
#define GDR_PMI_DECO_HWAN_SLOT2 42
#define GDR_PMI_DECO_HWAN_SLOT3 43
#define GDR_PMI_DECO_HWAN_SLOT4 44
#define GDR_PMI_HWAN_EFF_FACE 45
#define GDR_PMI_HWAN_EFF_GLOW 46
#define GDR_PMI_PK 50
#define GDR_PMI_FORFEIT 51
#define GDR_PMI_TXT_PHYBAL 52
#define GDR_PMI_TXT_MAGBAL 53
#define GDR_PMI_TXT_PHYBALDAT 54
#define GDR_PMI_TXT_MAGBALDAT 55
#define GDR_PMI_BATTLE_GRADE 60
#define GDR_PMI_FORTRESS_INFO 70
#define GDR_PMI_EFFECT_HP 77
#define GDR_PMI_EFFECT_MP 78
#define GDR_PMI_SELECT 200

void CIFPlayerMiniInfo::Render()
{
	reinterpret_cast<void(__thiscall*)(CIFPlayerMiniInfo*)>(0x007B7C60)(this);
}

bool CIFPlayerMiniInfo::GetZerkButtonState()
{
    return m_IRM.GetResObj<CIFButton>(13, 1)->IsVisible();
}

void CIFPlayerMiniInfo::OnUpdateIMPL() {

    static bool hudSettingApplied = false;
    if (!hudSettingApplied)
    {
        hudSettingApplied = true;
        DesktopCharacterHud::SetEnabled(LoadDesktopHudEnabledSetting());
    }

    if(m_Settings->ENABLE_NEW_JOB_UI)
    {
        g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->MoveGWnd(this->GetPos().x + 205, this->GetPos().y + 25);
        g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->BringToFront();

    }
/*
    CIFWnd *wind = g_pCGInterface->m_IRM.GetResObj(201, 1);
    if(wind != NULL)
    {
        wind->MoveGWnd(this->GetPos().x + this->GetSize().width - 38, this->GetPos().y + 70);
    }
*/
    reinterpret_cast<void (__thiscall *)(CIFPlayerMiniInfo *)>(0x007b7c60)(this);

    UpdateTargetPossibleDropsButtonOverlay();
    DesktopCharacterHud::PublishFromPlayerMiniInfo();
    RefreshDesktopHudPortraitWhenReady();

}
