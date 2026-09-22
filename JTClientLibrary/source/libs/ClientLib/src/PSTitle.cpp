//
// Created by YUMBUL on 19.03.2024.
//

#include <BSLib/multibyte.h>
#include <CustomData/CustomSettingManager.h>
#include <SRIFLib/NIFUnderMenuBar.h>
#include <CustomData/CustomDataManager.h>
#include <SRIFLib/NInterfaceResource.h>
#include "PSTitle.h"
#include "Game.h"
#include "TextStringManager.h"
#include "PSCharacterSelect.h"
#include "ICPlayer.h"
#include "IFOListCtrl.h"
#include "GInterface.h"
#include "IFSimpleListCtrl.h"
#include "IFStateSlot.h"
#include "IFEdit.h"
#include "IDamageText.h"
#include "IFCOSCommand.h"
#include "IFEquipment.h"
#include "IFTargetWindowCommonEnemy.h"
#include "IFTargetWindowSpecialMob.h"
#include <support/hook.h>
#include <support/Process.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include <CustomData/CustomCICPlayer.h>
#include "SkillAutomationController.h"
#include "CustomData/MasteryLimitPatch.h"
#include <support/MemberFunctionHook.h>

#include <iostream>
#include <ctime>
#include <vector>

#include <stdexcept>
#include <iostream>
#include <time.h>  // time(NULL) için gerekli
#include <SecondPW/IFSecondaryPassword.h>
#include <Hwid/HWIDGenerator.h>
#include <DiscordRichPresence/DiscordManager.h>
#include <CustomInterface/IFLoginRegisterWnd.h>
#include <CustomInterface/IFQuickLoginPanel.h>
#include <CustomInterface/IFOfflineStall.h>
#include "../../../DevKit_DLL/src/Util.h"
#include "../../../DevKit_DLL/src/ClientStartupCompatibility.h"

struct CachedShardEntry {
    unsigned short id;
    std::n_string name;
    unsigned short current;
    unsigned short capacity;
    byte status;
    byte globalOperationId;
    byte nextFlag;
};

struct CachedShardList {
    bool valid;
    byte globalOperationFlag;
    byte globalOperationType;
    std::n_string globalOperationName;
    byte postGlobalOperationFlag;
    byte shardFlag;
    std::vector<CachedShardEntry> shards;
};

static CachedShardList g_LastShardList = {};
static bool g_ReplayCachedShardList = false;
static bool g_LastShardStatusTextureValid = false;
static std::string g_LastShardStatusTexture;

static void ApplyMasteryLimits(int chineseMasteryLimit, int europeanMasteryLimit)
{
    if (chineseMasteryLimit < 1 || chineseMasteryLimit > 10000 ||
        europeanMasteryLimit < 1 || europeanMasteryLimit > 10000)
        return;
    const DWORD addresses[] = {0x006AA4C3, 0x006A51BC, 0x006A5197, 0x006AA498};
    const DWORD lengths[] = {5, 5, 16, 16};
    BYTE original[4][16] = {}, patched[4][16] = {};
    int i;
    for (i = 0; i < 4; ++i) {
        SIZE_T count = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)addresses[i],
            original[i], lengths[i], &count) || count != lengths[i]) {
            WriteClientStartupDiagnostic("[Mastery] Cannot read native mastery instructions; no changes applied.");
            return;
        }
        memcpy(patched[i], original[i], lengths[i]);
        bool valid;
        if (i < 2) {
            valid = original[i][0] == (i == 0 ? 0xBE : 0xBF);
            const DWORD cap = static_cast<DWORD>(chineseMasteryLimit);
            memcpy(patched[i] + 1, &cap, sizeof(cap));
        } else {
            // Includes the cap loads at 0x006A51A2 and 0x006AA4A3.
            valid = BuildMasteryTotalSelection(original[i], patched[i],
                i == 2 ? 0xBF : 0xBE, europeanMasteryLimit);
        }
        if (!valid) {
            char diagnostic[256];
            _snprintf(diagnostic, sizeof(diagnostic),
                "[Mastery] Unsupported native layout at %08lX (CH=%d EU=%d): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X; no changes applied.",
                addresses[i], chineseMasteryLimit, europeanMasteryLimit,
                original[i][0],original[i][1],original[i][2],original[i][3],
                original[i][4],original[i][5],original[i][6],original[i][7],
                original[i][8],original[i][9],original[i][10],original[i][11],
                original[i][12],original[i][13],original[i][14],original[i][15]);
            diagnostic[sizeof(diagnostic)-1] = 0;
            WriteClientStartupDiagnostic(diagnostic);
            return;
        }
    }
    bool changed = false;
    for (i = 0; i < 4; ++i) {
        if (memcmp(original[i], patched[i], lengths[i]) == 0)
            continue;
        changed = true;
        BYTE verified[16];
        SIZE_T count = 0;
        if (!WriteProcessBytes(GetCurrentProcess(), addresses[i], patched[i], lengths[i]) ||
            !ReadProcessMemory(GetCurrentProcess(), (LPCVOID)addresses[i], verified, lengths[i], &count) ||
            count != lengths[i] || memcmp(verified, patched[i], lengths[i]) != 0) {
            bool restored = true;
            for (int j = i; j >= 0; --j)
                if (!WriteProcessBytes(GetCurrentProcess(), addresses[j], original[j], lengths[j])) restored = false;
            WriteClientStartupDiagnostic(restored
                ? "[Mastery] Write failed; original mastery instructions restored."
                : "[Mastery] Write and rollback failed; restart the client.");
            return;
        }
    }
    if (changed) {
        char diagnostic[128];
        _snprintf(diagnostic, sizeof(diagnostic), "[Mastery] Applied and verified total limits CH=%d EU=%d.",
            chineseMasteryLimit, europeanMasteryLimit);
        diagnostic[sizeof(diagnostic)-1] = 0;
        WriteClientStartupDiagnostic(diagnostic);
    }
}

void KmtApplyConfiguredMasteryLimits()
{
    if (!m_Settings)
        return;

    const int chineseMasteryLimit = m_Settings->ChineseMasteryLimit > 0
        ? m_Settings->ChineseMasteryLimit
        : m_Settings->MaxMasteryLevel;
    const int europeanMasteryLimit = m_Settings->EuropeanMasteryLimit > 0
        ? m_Settings->EuropeanMasteryLimit
        : m_Settings->MaxMasteryLevel;

    ApplyMasteryLimits(chineseMasteryLimit, europeanMasteryLimit);
}

static const char* SelectShardStatusTexture(byte shardStatus, unsigned short shardCurrent, unsigned short shardCapacity)
{
    if (shardStatus == 0) {
        return "clientlibrary\\charselect\\login_window_eu_located_black.ddj";
    }

    if (shardStatus == 1) {
        const double usagePercent = shardCapacity == 0
            ? 0
            : (static_cast<double>(shardCurrent) / shardCapacity) * 100;

        if (usagePercent < 50) {
            return "clientlibrary\\charselect\\login_window_eu_located_green.ddj";
        }
        if (usagePercent < 80) {
            return "clientlibrary\\charselect\\login_window_eu_located_yellow.ddj";
        }
        return "clientlibrary\\charselect\\login_window_eu_located_red.ddj";
    }

    return 0;
}

static void RememberShardStatusTexture(const char* texture)
{
    if (texture == 0) {
        return;
    }

    g_LastShardStatusTexture = texture;
    g_LastShardStatusTextureValid = true;
}

static void ApplyCachedShardStatusTextureToTitle(CPSTitle* title)
{
    if (!title) {
        return;
    }

    title->ApplyCachedShardStatusTexture();
}

static CIFStatic* EnsureLoginRegisterCaption(CPSTitle* title)
{
    if (!title) {
        return 0;
    }

    CIFStatic* caption = title->GetGuiFromList<CIFStatic>(LOGIN_REGISTER_CAPTION_ID);
    if (!caption) {
        RECT captionRect = {0, 0, 1, 1};
        caption = (CIFStatic*)CGWnd::CreateInstance(
            title,
            GFX_RUNTIME_CLASS(CIFStatic),
            captionRect,
            LOGIN_REGISTER_CAPTION_ID,
            0);
        if (caption) {
            caption->SetClickable(false);
            caption->ShowGWnd(false);
        }
    }

    return caption;
}

static void SyncLoginRegisterCaption(
    CPSTitle* title,
    CIFLoginRegisterButton* button,
    bool allowedVisible)
{
    CIFStatic* caption = EnsureLoginRegisterCaption(title);
    if (!caption) {
        return;
    }
    if (!button) {
        caption->ShowGWnd(false);
        return;
    }

    // The custom button is only the click surface.  Its visible wording is a
    // native CIFStatic sibling because that renderer survives title-scene
    // rebuilds when the player returns to change accounts.
    button->SetText(L"");

    CGWndBase::wnd_pos buttonPos = button->GetPos();
    CGWndBase::wnd_size buttonSize = button->GetSize();
    caption->SetGWndSize(buttonSize.width, buttonSize.height);
    caption->MoveGWnd(buttonPos.x, buttonPos.y);
    caption->SetClickable(false);

    void* font = theApp.GetFont(0);
    if (font) {
        caption->SetFont(font);
    }
    caption->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
    caption->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    caption->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    caption->SetText(KmtGetText(L"UIIT_KMT_REGISTER"));

    const bool visible = allowedVisible && button->IsVisible();
    caption->ShowGWnd(visible);
    if (visible) {
        caption->BringToFront();
    }
}

static void CaptureShardList(CMsgStreamBuffer* msg)
{
    CachedShardList snapshot = {};
    *msg >> snapshot.globalOperationFlag;
    snapshot.postGlobalOperationFlag = snapshot.globalOperationFlag;

    if (snapshot.globalOperationFlag == 1) {
        *msg >> snapshot.globalOperationType
             >> snapshot.globalOperationName
             >> snapshot.postGlobalOperationFlag;
    }

    *msg >> snapshot.shardFlag;
    byte shardFlag = snapshot.shardFlag;
    while (shardFlag == 1) {
        CachedShardEntry shard = {};
        *msg >> shard.id
             >> shard.name
             >> shard.current
             >> shard.capacity
             >> shard.status
             >> shard.globalOperationId
             >> shard.nextFlag;
        RememberShardStatusTexture(SelectShardStatusTexture(shard.status, shard.current, shard.capacity));
        snapshot.shards.push_back(shard);
        shardFlag = shard.nextFlag;
    }

    snapshot.valid = !snapshot.shards.empty();
    if (snapshot.valid)
        g_LastShardList = snapshot;
}

void CPSTitle::ReplayCachedShardList()
{
    if (!g_ReplayCachedShardList || !g_LastShardList.valid)
        return;

    CMsgStreamBuffer replay(0xA101);
    replay << g_LastShardList.globalOperationFlag;
    if (g_LastShardList.globalOperationFlag == 1) {
        replay << g_LastShardList.globalOperationType
               << g_LastShardList.globalOperationName
               << g_LastShardList.postGlobalOperationFlag;
    }

    replay << g_LastShardList.shardFlag;
    for (size_t i = 0; i < g_LastShardList.shards.size(); ++i) {
        const CachedShardEntry& shard = g_LastShardList.shards[i];
        replay << shard.id
               << shard.name
               << shard.current
               << shard.capacity
               << shard.status
               << shard.globalOperationId
               << shard.nextFlag;
    }

    // Replay through the game's real CPSTitle handler. This restores its
    // internal shard model, selection eligibility, and button state together.
    reinterpret_cast<int(__thiscall *)(CPSTitle *, CMsgStreamBuffer *)>(
        0x0086bfc0)(this, &replay);
    g_ReplayCachedShardList = false;
}

void CPSTitle::ApplyCachedShardStatusTexture()
{
    if (!m_Settings->NewIDPWScreen || !g_LastShardStatusTextureValid) {
        return;
    }

    m_IRM.GetResObj(1929, 1)->TB_Func_13(g_LastShardStatusTexture.c_str(), 1, 1);
    m_IRM.GetResObj(1927, 1)->TB_Func_13(g_LastShardStatusTexture.c_str(), 1, 1);
}

bool CPSTitle::OnCreateIMPL(long ln) {
    if (!WaitForClientInitialization(120000)) {
        WriteClientStartupDiagnostic(
            "Login interface creation stopped because KMTGuard initialization did not complete.");
        TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
        return false;
    }

    // Normal startup installs the runtime classes from InitGameAssets. This
    // main-thread fallback also covers supported loaders that attach after
    // that one-time client phase without ever exposing a null runtime class
    // to the interface resource manager.
    if (!EnsureRuntimeClassesInstalled()) {
        WriteClientStartupDiagnostic(
            "Login interface creation stopped because runtime-class registration was unavailable.");
        TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
        return false;
    }

    g_SkillAutomationController.ResetForCharacterChange();

    bool a = reinterpret_cast<bool(__thiscall *)(CPSTitle *, long)>(0x0086b190)(this, ln);
    g_ReplayCachedShardList = g_LastShardList.valid;
    QuickLogin_ResetPanel();
    if(m_Settings->NewIDPWScreen)
    {

        this->m_IRM.GetResObj(1926, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1927, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1928, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1929, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1930, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1931, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(1930, 1)->SetText(KmtGetText(L"UIIT_KMT_LOCATION"));
        this->m_IRM.GetResObj(104, 1)->SetText(KmtGetText(L"UIIT_KMT_CHANNEL"));

        wnd_pos servernamelist = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->GetPos();
        wnd_size servernamelistsize = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->GetSize();
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->MoveGWnd(servernamelist.x + 22, servernamelist.y);
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->SetGWndSize(servernamelistsize.width - 22, servernamelistsize.height - 14);

        wnd_pos servernamelistx = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->GetPos();
        wnd_size servernamelistsizex = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->GetSize();
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->MoveGWnd(servernamelistx.x + 38, servernamelistx.y);
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->SetGWndSize(servernamelistsizex.width + 4, servernamelistsizex.height);


        wnd_pos servernamelistxx = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10002)->GetPos();
        wnd_size servernamelistsizexx = this->GetSize();
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10002)->MoveGWnd(servernamelistxx.x + 60, servernamelistxx.y);



        this->m_IRM.GetResObj<CIFButton>(46, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFButton>(46, 1)->SetEnabledState(false);
        this->m_IRM.GetResObj<CIFButton>(46, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);
        this->m_IRM.GetResObj(45, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(104, 1)->ShowGWnd(true);
        //  this->m_IRM.GetResObj(15, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(43, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(45, 1)->SetText(KmtGetText(L"UIIT_KMT_WEST"));
        wnd_pos tt =  this->m_IRM.GetResObj(104, 1)->GetPos();
        this->m_IRM.GetResObj(104, 1)->MoveGWnd(tt.x - 22, tt.y - 2);

        wnd_pos t5 =  this->m_IRM.GetResObj(45, 1)->GetPos();
        this->m_IRM.GetResObj(45, 1)->MoveGWnd(t5.x - 17, t5.y - 2);
        this->m_IRM.GetResObj(45, 1)->SetGWndSize(198, 17);

        wnd_pos chlist =  this->m_IRM.GetResObj(46, 1)->GetPos();
        this->m_IRM.GetResObj(46, 1)->MoveGWnd(chlist.x + 94, chlist.y - 4);


        wnd_pos logo;
        logo = this->m_IRM.GetResObj(GDR_LOGO, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_LOGO, 1)->MoveGWnd(logo.x, logo.y - 60);


        wnd_pos x;
        x = this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->TB_Func_13("clientlibrary\\charselect\\newlogin.ddj", 1, 1);
        this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->SetGWndSize(400, 190);
        this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->MoveGWnd(x.x - 56, x.y - 47);




        wnd_pos connect;
        connect = this->m_IRM.GetResObj(GDR_CONNECTBTN, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_CONNECTBTN, 1)->MoveGWnd(connect.x - 10, connect.y);


        wnd_pos exit;
        exit = this->m_IRM.GetResObj(GDR_EXIT, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_EXIT, 1)->MoveGWnd(exit.x - 6, exit.y);

        wnd_pos listbtn;
        listbtn = this->m_IRM.GetResObj(GDR_LISTBTN, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_LISTBTN, 1)->MoveGWnd(listbtn.x + 94, listbtn.y - 2);


        wnd_pos id;
        id = this->m_IRM.GetResObj(GDR_ID, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_ID, 1)->SetGWndSize(138, 17);
        this->m_IRM.GetResObj(GDR_ID, 1)->MoveGWnd(id.x - 17, id.y - 2);


        wnd_pos pw;
        pw = this->m_IRM.GetResObj(GDR_PW, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_PW, 1)->SetGWndSize(198, 17);
        this->m_IRM.GetResObj(GDR_PW, 1)->MoveGWnd(pw.x - 17, pw.y - 2);


        wnd_pos sw;
        sw = this->m_IRM.GetResObj(GDR_SWNAME, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_SWNAME, 1)->SetGWndSize(198, 17);
        this->m_IRM.GetResObj(GDR_SWNAME, 1)->MoveGWnd(sw.x - 17, sw.y - 2);


        wnd_pos id1;
        id1 = this->m_IRM.GetResObj(GDR_101ID, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_101ID, 1)->SetGWndSize(53, 16);
        this->m_IRM.GetResObj(GDR_101ID, 1)->MoveGWnd(id1.x - 22, id1.y - 5);


        wnd_pos pw1;
        pw1 = this->m_IRM.GetResObj(GDR_102PW, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_102PW, 1)->SetGWndSize(53, 16);
        this->m_IRM.GetResObj(GDR_102PW, 1)->MoveGWnd(pw1.x - 22, pw1.y - 4);


        wnd_pos sw1;
        sw1 = this->m_IRM.GetResObj(GDR_103SW, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_103SW, 1)->SetGWndSize(53, 16);
        this->m_IRM.GetResObj(GDR_103SW, 1)->MoveGWnd(sw1.x - 22, sw1.y - 3);


        wnd_pos list = this->m_IRM.GetResObj(GDR_LIST, 1)->GetPos();
        this->m_IRM.GetResObj(GDR_LIST, 1)->TB_Func_13("clientlibrary\\charselect\\server_global_window.ddj", 1, 1);
        this->m_IRM.GetResObj(GDR_LIST, 1)->SetGWndSize(312, 340);
        this->m_IRM.GetResObj(GDR_LIST, 1)->MoveGWnd(list.x - 36, list.y);

        wnd_pos sliderctrl = this->m_IRM.GetResObj(54, 1)->GetPos();
        this->m_IRM.GetResObj(54, 1)->MoveGWnd(sliderctrl.x + 69, sliderctrl.y - 3);



        wnd_pos serverlist = this->m_IRM.GetResObj(51, 1)->GetPos();
        wnd_size serverlistsize = this->m_IRM.GetResObj(51, 1)->GetSize();
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->MoveGWnd(serverlist.x, serverlist.y +14);
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->SetGWndSize(serverlistsize.width+71, serverlistsize.height-14);
        this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->N00001C53 = serverlistsize.width + 71;


    }
    else
    {
        this->m_IRM.GetResObj(1926, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj(1927, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj(1928, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj(1929, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj(1930, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj(1931, 1)->ShowGWnd(false);


    }

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\pstitle.txt");
    m_IRM.CreateInterfaceSection("SecondaryPass", this);

    m_IRM.GetResObj<CIFSecondaryPassword>(5000,1)->ShowGWnd(false);
    m_IRM.GetResObj<CIFSecondaryPassword>(5000,1)->UpdateMenuSize();

    EnsureLoginRegisterButton();

    SyncLoginRegisterButton();
    QuickLogin_CreatePanel(this);
    ApplyCachedShardStatusTextureToTitle(this);
    OfflineStall_CreateLoginPrompt(this);
//    m_CustomDataManager->AvatarMallItemList.clear();
 //   m_CustomDataManager->CustomItemMallItemList.clear();
    m_CustomDataManager->_ActiveTags.clear();
    m_CustomDataManager->_ActiveTitleColors.clear();
    m_CustomDataManager->_ActiveNameColors.clear();

    m_CustomDataManager->m_IconsData.clear();
    m_CustomDataManager->MediaIcons.clear();
    m_CustomDataManager->m_LeftCharIcons.clear();
    m_CustomDataManager->m_RightCharIcons.clear();
    m_CustomDataManager->m_RefAchievement.clear();

    m_CustomDataManager->m_RefFellowPetSystem.clear();
    m_CustomDataManager->UniqueTargetHashmap.clear();
    m_CustomDataManager->UniqueTargetHashmapPlayer.clear();
    m_CustomDataManager->g_GroupSpawn_Type = 0;
    m_CustomDataManager->g_despawned_objects.clear();

    m_CustomDataManager->m_EventMapSettings.clear();
    m_CustomDataManager->HideEffects.clear();


    m_Player->m_AchievementsCondition.clear();
    m_Player->m_Achievements.clear();



    m_Player->m_MacroAutoSkillData.clear();
    m_Player->m_FellowSkillData.clear();

    m_Player->m_MagicPopTimerRunning = false;
    m_Player->m_MagicPopSettings = 700;


    m_Player->Enabled_AutoStr = false;
    m_Player->Enabled_AutoInt = false;
    m_Player->Enabled_AutoMastery = false;
    m_Player->Selected_AutoMasteryId = 0;
    m_Player->Enabled_AutoSkill = false;
    m_Player->Selected_AutoSkillMasteryId = 0;
    m_Player->Selected_AutoSkillId = 0;


    m_Player->PetSkillTimerRunning = false;


    m_Player->ReverseSlot = 9999;

    m_Player->m_MyNewAutoHunting_Enabled = false;


    m_Player->FocusedId = 0;
    m_Player->MailAddress = std::n_wstring();
   // this->ShowMessage(L"Welcome the New Server server protected by VFILTER V2.0", 0xFFFFF0);
    if(!m_Settings->PSTitleIsLoaded)
    {
        CMsgStreamBuffer buf(0xA150);
        SendMsg(buf);

        std::n_string IconPath = "interface\\worldmap\\wmap_sign_questnpc.ddj";
        m_CustomDataManager->MapIcon = Fun_CacheTexture_Create(std::n_string(IconPath));

        std::n_string PingIconPath = "clientlibrary\\common\\wmap_sign_location.ddj";
        m_CustomDataManager->PingIcon = Fun_CacheTexture_Create(std::n_string(PingIconPath));

          m_CustomDataManager->emojiList.insert(std::make_pair(L":01", "clientlibrary\\emoji\\emoji01.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":02", "clientlibrary\\emoji\\emoji02.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":03", "clientlibrary\\emoji\\emoji03.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":04", "clientlibrary\\emoji\\emoji04.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":05", "clientlibrary\\emoji\\emoji05.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":06", "clientlibrary\\emoji\\emoji06.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":07", "clientlibrary\\emoji\\emoji07.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":08", "clientlibrary\\emoji\\emoji08.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":09", "clientlibrary\\emoji\\emoji09.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":10", "clientlibrary\\emoji\\emoji10.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":11", "clientlibrary\\emoji\\emoji11.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":12", "clientlibrary\\emoji\\emoji12.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":13", "clientlibrary\\emoji\\emoji13.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":14", "clientlibrary\\emoji\\emoji14.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":15", "clientlibrary\\emoji\\emoji15.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":16", "clientlibrary\\emoji\\emoji16.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":17", "clientlibrary\\emoji\\emoji17.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":18", "clientlibrary\\emoji\\emoji18.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":19", "clientlibrary\\emoji\\emoji19.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":20", "clientlibrary\\emoji\\emoji20.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":21", "clientlibrary\\emoji\\emoji21.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":22", "clientlibrary\\emoji\\emoji22.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":23", "clientlibrary\\emoji\\emoji23.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":24", "clientlibrary\\emoji\\emoji24.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":25", "clientlibrary\\emoji\\emoji25.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":26", "clientlibrary\\emoji\\emoji26.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":27", "clientlibrary\\emoji\\emoji27.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":28", "clientlibrary\\emoji\\emoji28.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":29", "clientlibrary\\emoji\\emoji29.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":30", "clientlibrary\\emoji\\emoji30.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":31", "clientlibrary\\emoji\\emoji31.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":32", "clientlibrary\\emoji\\emoji32.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":33", "clientlibrary\\emoji\\emoji33.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":34", "clientlibrary\\emoji\\emoji34.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":35", "clientlibrary\\emoji\\emoji35.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":36", "clientlibrary\\emoji\\emoji36.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":37", "clientlibrary\\emoji\\emoji37.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":38", "clientlibrary\\emoji\\emoji38.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":39", "clientlibrary\\emoji\\emoji39.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":40", "clientlibrary\\emoji\\emoji40.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":41", "clientlibrary\\emoji\\emoji41.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L".1.", "clientlibrary\\emoji\\emoji42.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":43", "clientlibrary\\emoji\\emoji43.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":44", "clientlibrary\\emoji\\emoji44.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":45", "clientlibrary\\emoji\\emoji45.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":46", "clientlibrary\\emoji\\emoji46.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":47", "clientlibrary\\emoji\\emoji47.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":48", "clientlibrary\\emoji\\emoji48.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":49", "clientlibrary\\emoji\\emoji49.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":50", "clientlibrary\\emoji\\emoji50.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":51", "clientlibrary\\emoji\\emoji51.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":52", "clientlibrary\\emoji\\emoji52.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":53", "clientlibrary\\emoji\\emoji53.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":54", "clientlibrary\\emoji\\emoji54.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":55", "clientlibrary\\emoji\\emoji55.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":56", "clientlibrary\\emoji\\emoji56.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":57", "clientlibrary\\emoji\\emoji57.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":58", "clientlibrary\\emoji\\emoji58.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":59", "clientlibrary\\emoji\\emoji59.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":60", "clientlibrary\\emoji\\emoji60.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":61", "clientlibrary\\emoji\\emoji61.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":62:", "clientlibrary\\emoji\\emoji62.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":63", "clientlibrary\\emoji\\emoji63.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":64", "clientlibrary\\emoji\\emoji64.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":65", "clientlibrary\\emoji\\emoji65.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":66", "clientlibrary\\emoji\\emoji66.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^01", "clientlibrary\\emoji\\pepe11.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^02", "clientlibrary\\emoji\\pepe12.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^03", "clientlibrary\\emoji\\pepe13.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^04", "clientlibrary\\emoji\\pepe14.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^05", "clientlibrary\\emoji\\pepe15.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^06", "clientlibrary\\emoji\\pepe01.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^07", "clientlibrary\\emoji\\pepe02.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^08", "clientlibrary\\emoji\\pepe03.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^09", "clientlibrary\\emoji\\pepe04.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^10", "clientlibrary\\emoji\\pepe05.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^11", "clientlibrary\\emoji\\pepe06.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^12", "clientlibrary\\emoji\\pepe07.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^13", "clientlibrary\\emoji\\pepe08.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^14", "clientlibrary\\emoji\\pepe09.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"^15", "clientlibrary\\emoji\\pepe10.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":D", "clientlibrary\\emoji\\emoji01.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L"<3", "clientlibrary\\emoji\\emoji02.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":Kek", "clientlibrary\\emoji\\pepe11.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":S", "clientlibrary\\emoji\\emoji03.ddj"));
          m_CustomDataManager->emojiList.insert(std::make_pair(L":P", "clientlibrary\\emoji\\emoji04.ddj"));

       m_CustomDataManager->emojiListData[":01"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji01.ddj");
       m_CustomDataManager->emojiListData[":02"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji02.ddj");
       m_CustomDataManager->emojiListData[":03"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji03.ddj");
       m_CustomDataManager->emojiListData[":04"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji04.ddj");
       m_CustomDataManager->emojiListData[":05"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji05.ddj");
       m_CustomDataManager->emojiListData[":06"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji06.ddj");
       m_CustomDataManager->emojiListData[":07"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji07.ddj");
       m_CustomDataManager->emojiListData[":08"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji08.ddj");
       m_CustomDataManager->emojiListData[":09"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji09.ddj");
       m_CustomDataManager->emojiListData[":10"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji10.ddj");
       m_CustomDataManager->emojiListData[":11"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji11.ddj");
       m_CustomDataManager->emojiListData[":12"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji12.ddj");
       m_CustomDataManager->emojiListData[":13"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji13.ddj");
       m_CustomDataManager->emojiListData[":14"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji14.ddj");
       m_CustomDataManager->emojiListData[":15"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji15.ddj");
       m_CustomDataManager->emojiListData[":16"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji16.ddj");
       m_CustomDataManager->emojiListData[":17"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji17.ddj");
       m_CustomDataManager->emojiListData[":18"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji18.ddj");
       m_CustomDataManager->emojiListData[":19"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji19.ddj");
       m_CustomDataManager->emojiListData[":20"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji20.ddj");
       m_CustomDataManager->emojiListData[":21"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji21.ddj");
       m_CustomDataManager->emojiListData[":22"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji22.ddj");
       m_CustomDataManager->emojiListData[":23"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji23.ddj");
       m_CustomDataManager->emojiListData[":24"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji24.ddj");
       m_CustomDataManager->emojiListData[":25"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji25.ddj");
       m_CustomDataManager->emojiListData[":26"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji26.ddj");
       m_CustomDataManager->emojiListData[":27"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji27.ddj");
       m_CustomDataManager->emojiListData[":28"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji28.ddj");
       m_CustomDataManager->emojiListData[":29"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji29.ddj");
       m_CustomDataManager->emojiListData[":30"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji30.ddj");
       m_CustomDataManager->emojiListData[":31"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji31.ddj");
       m_CustomDataManager->emojiListData[":32"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji32.ddj");
       m_CustomDataManager->emojiListData[":33"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji33.ddj");
       m_CustomDataManager->emojiListData[":34"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji34.ddj");
       m_CustomDataManager->emojiListData[":35"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji35.ddj");
       m_CustomDataManager->emojiListData[":36"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji36.ddj");
       m_CustomDataManager->emojiListData[":37"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji37.ddj");
       m_CustomDataManager->emojiListData[":38"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji38.ddj");
       m_CustomDataManager->emojiListData[":39"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji39.ddj");
       m_CustomDataManager->emojiListData[":40"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji40.ddj");
       m_CustomDataManager->emojiListData[":41"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji41.ddj");
       m_CustomDataManager->emojiListData[".1."] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji42.ddj");
       m_CustomDataManager->emojiListData[":43"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji43.ddj");
       m_CustomDataManager->emojiListData[":44"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji44.ddj");
       m_CustomDataManager->emojiListData[":45"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji45.ddj");
       m_CustomDataManager->emojiListData[":46"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji46.ddj");
       m_CustomDataManager->emojiListData[":47"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji47.ddj");
       m_CustomDataManager->emojiListData[":48"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji48.ddj");
       m_CustomDataManager->emojiListData[":49"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji49.ddj");
       m_CustomDataManager->emojiListData[":50"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji50.ddj");
       m_CustomDataManager->emojiListData[":51"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji51.ddj");
       m_CustomDataManager->emojiListData[":52"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji52.ddj");
       m_CustomDataManager->emojiListData[":53"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji53.ddj");
       m_CustomDataManager->emojiListData[":54"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji54.ddj");
       m_CustomDataManager->emojiListData[":55"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji55.ddj");
       m_CustomDataManager->emojiListData[":56"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji56.ddj");
       m_CustomDataManager->emojiListData[":57"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji57.ddj");
       m_CustomDataManager->emojiListData[":58"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji58.ddj");
       m_CustomDataManager->emojiListData[":59"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji59.ddj");
       m_CustomDataManager->emojiListData[":60"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji60.ddj");
       m_CustomDataManager->emojiListData[":61"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji61.ddj");
       m_CustomDataManager->emojiListData[":62"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji62.ddj");
       m_CustomDataManager->emojiListData[":63"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji63.ddj");
       m_CustomDataManager->emojiListData[":64"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji64.ddj");
       m_CustomDataManager->emojiListData[":65"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji65.ddj");
       m_CustomDataManager->emojiListData[":66"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji66.ddj");
       m_CustomDataManager->emojiListData["^01"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe11.ddj");
       m_CustomDataManager->emojiListData["^02"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe12.ddj");
       m_CustomDataManager->emojiListData["^03"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe13.ddj");
       m_CustomDataManager->emojiListData["^04"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe14.ddj");
       m_CustomDataManager->emojiListData["^05"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe15.ddj");
       m_CustomDataManager->emojiListData["^06"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe01.ddj");
       m_CustomDataManager->emojiListData["^07"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe02.ddj");
       m_CustomDataManager->emojiListData["^08"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe03.ddj");
       m_CustomDataManager->emojiListData["^09"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe04.ddj");
       m_CustomDataManager->emojiListData["^10"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe05.ddj");
       m_CustomDataManager->emojiListData["^11"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe06.ddj");
       m_CustomDataManager->emojiListData["^12"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe07.ddj");
       m_CustomDataManager->emojiListData["^13"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe08.ddj");
       m_CustomDataManager->emojiListData["^14"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe09.ddj");
       m_CustomDataManager->emojiListData["^15"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe10.ddj");
       m_CustomDataManager->emojiListData[":D"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji01.ddj");
       m_CustomDataManager->emojiListData["<3"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji02.ddj");
       m_CustomDataManager->emojiListData[":Kek"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\pepe11.ddj");
       m_CustomDataManager->emojiListData[":S"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji03.ddj");
       m_CustomDataManager->emojiListData[":P"] = Fun_CacheTexture_Create("clientlibrary\\emoji\\emoji04.ddj");


    }
    return a;
}

void CPSTitle::PressConnectButton()
{
    if(this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->IsVisible())
        return;
    reinterpret_cast<void(__thiscall *)(CPSTitle *)>(0x00869460)(this);
    SyncLoginRegisterButton();
    QuickLogin_SyncPanel(this);
}
void CPSTitle::PressButtonServerList()
{
    if(this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->IsVisible())
        return;
    reinterpret_cast<void(__thiscall *)(CPSTitle *)>(0x00864c80)(this);

}

void CPSTitle::ShowLoginRegisterWindow()
{
    CIFLoginRegisterWnd* registerWnd = this->m_IRM.GetResObj<CIFLoginRegisterWnd>(LOGIN_REGISTER_WINDOW_ID, 1);
    if (!registerWnd) {
        RECT registerWndRect = {362, 250, 300, 215};
        registerWnd = (CIFLoginRegisterWnd*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFLoginRegisterWnd), registerWndRect, LOGIN_REGISTER_WINDOW_ID, 0);
    }

    if (!registerWnd) {
        return;
    }

    registerWnd->ShowGWnd(true);
}

CIFLoginRegisterButton* CPSTitle::GetLoginRegisterButton()
{
    // CPSTitle is reused when the player returns from character selection to
    // change accounts.  Do not trust a process-wide raw pointer from the
    // previous title scene; resolve the control owned by this title first.
    return GetGuiFromList<CIFLoginRegisterButton>(LOGIN_REGISTER_BUTTON_ID);
}

CIFLoginRegisterButton* CPSTitle::EnsureLoginRegisterButton()
{
    CIFLoginRegisterButton* registerButton = GetLoginRegisterButton();
    if (registerButton) {
        registerButton->SetText(L"");
        EnsureLoginRegisterCaption(this);
        return registerButton;
    }

    const int registerButtonWidth = 86;
    const int registerButtonHeight = 38;
    RECT registerButtonRect = {0, 0, registerButtonWidth, registerButtonHeight};

    registerButton = (CIFLoginRegisterButton*)CGWnd::CreateInstance(
        this,
        GFX_RUNTIME_CLASS(CIFLoginRegisterButton),
        registerButtonRect,
        LOGIN_REGISTER_BUTTON_ID,
        0);

    if (registerButton) {
        registerButton->TB_Func_13("interface\\outer\\button.ddj", 1, 1);
        registerButton->SetText(L"");
        registerButton->ShowGWnd(false);
        EnsureLoginRegisterCaption(this);
    }

    return registerButton;
}

void CPSTitle::SyncLoginRegisterButton()
{
    CIFLoginRegisterButton* registerButton = EnsureLoginRegisterButton();

    CIFButton* connectButton = this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1);
    CIFButton* exitButton = this->m_IRM.GetResObj<CIFButton>(GDR_EXIT, 1);

    if (!registerButton || !connectButton || !exitButton) {
        return;
    }

    CIFWnd* idPwFrame = this->m_IRM.GetResObj<CIFWnd>(GDR_IDPWFRAME, 1);
    if (!idPwFrame) {
        return;
    }

    wnd_pos framePos = idPwFrame->GetPos();
    wnd_size frameSize = idPwFrame->GetSize();

    // Control button row position from here.
    // Negative EXTRA_LEFT moves the whole row left.
    // Negative EXTRA_Y moves the whole row up.
    const int EXTRA_LEFT = -10;
    const int EXTRA_Y = 0;

    const int GAP = 8;
    const int BUTTON_HEIGHT = 38;

    // Keep the right edge aligned with the ID/PW frame even after moving the row left.
    const int totalWidth = frameSize.width - EXTRA_LEFT;
    const int BUTTON_WIDTH = (totalWidth - (GAP * 2)) / 3;

    int startX = framePos.x + EXTRA_LEFT;
    int buttonY = connectButton->GetPos().y + EXTRA_Y;

    connectButton->SetGWndSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    registerButton->SetGWndSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    exitButton->SetGWndSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    // Order: Connect | Register | Exit
    connectButton->MoveGWnd(startX, buttonY);
    registerButton->MoveGWnd(startX + BUTTON_WIDTH + GAP, buttonY);
    exitButton->MoveGWnd(startX + ((BUTTON_WIDTH + GAP) * 2), buttonY);

    const bool buttonRowVisible = connectButton->IsVisible() && exitButton->IsVisible();

    if (buttonRowVisible) {
        connectButton->BringToFront();
        registerButton->BringToFront();
        exitButton->BringToFront();
    }

    SetLoginRegisterButtonVisible(buttonRowVisible);
    SyncLoginRegisterCaption(this, registerButton, buttonRowVisible);
}

void CPSTitle::SetLoginRegisterButtonVisible(bool visible)
{
    CIFLoginRegisterButton* registerButton = GetLoginRegisterButton();

    if (!registerButton) {
        return;
    }

    if (registerButton->IsVisible() != visible) {
        registerButton->ShowGWnd(visible);
    }

    if (visible && registerButton->IsVisible()) {
        registerButton->BringToFront();
    }

    SyncLoginRegisterCaption(this, registerButton, visible);
}

std::n_string CPSTitle::GetLoginEditText(int id)
{
    CIFEdit* edit = this->m_IRM.GetResObj<CIFEdit>(id, 1);
    if (!edit) {
        return std::n_string();
    }

    return std::n_string(TO_STRING(edit->GetCurrentText()).c_str());
}

bool CPSTitle::GetLoginFrameRect(int& x, int& y, int& width, int& height)
{
    CIFStatic* frame = this->m_IRM.GetResObj<CIFStatic>(GDR_IDPWFRAME, 1);
    if (!frame) {
        return false;
    }

    wnd_pos pos = frame->GetPos();
    wnd_size size = frame->GetSize();
    x = pos.x;
    y = pos.y;
    width = size.width;
    height = size.height;
    return true;
}

void CPSTitle::OnUpdateIMPL()
{
    // Preserve CPSTitle's native update state machine. It advances the login
    // screen from its transient reset state back to the ready state required
    // by the native Connect handler after a rejected login.
    reinterpret_cast<void(__thiscall *)(CPSTitle *)>(0x008691B0)(this);
    ReplayCachedShardList();
    ApplyCachedShardStatusTextureToTitle(this);

    CIFLoginRegisterButton* registerButton = EnsureLoginRegisterButton();

    CIFButton* connectButton = this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1);
    CIFButton* exitButton = this->m_IRM.GetResObj<CIFButton>(GDR_EXIT, 1);
    CIFSecondaryPassword* secondaryPassword = this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1);
    bool registerCaptionVisible = false;

    if (registerButton && connectButton && exitButton) {
        const bool passwordVisible = secondaryPassword != 0 && secondaryPassword->IsVisible();
        const bool rowVisible = connectButton->IsVisible() && exitButton->IsVisible() && !passwordVisible;
        registerCaptionVisible = rowVisible;

        if (rowVisible) {
            if (!registerButton->IsVisible()) {
                SyncLoginRegisterButton();
                QuickLogin_SyncPanel(this);
            }
        } else if (registerButton->IsVisible()) {
            SetLoginRegisterButtonVisible(false);
        }
    }

    SyncLoginRegisterCaption(this, registerButton, registerCaptionVisible);

    // The server-list dialog hides Connect/Exit while its rows are visible.
    // These two presentation repairs must therefore run independently from
    // the login button row visibility.
    QuickLogin_EnsurePresentation(this);
}

enum eResultType
{
    ENTER_THE_PASSWORD = 0,
    CREATE_THE_PASSWORD = 1,
    CREATED_SUCCESS_ENTER_THE_PASSWORD = 2,
    WRONG_PASSWORD = 3,
    PW_TRUE = 4,
    PASSWORD_CHANGED_ENTER_THE_PASSWORD = 5,
};
bool CPSTitle::OnServerPacketRecv(CMsgStreamBuffer *msg) {
    if (msg->msgid() == 0xA101)
    {
        CaptureShardList(msg);
        msg->m_currentReadBytes = 0;
    }

    if (msg->msgid() == 0xB003)//notice
    {
        int Color;
        std::n_string notice;
        *msg >> notice >> Color;

        this->ShowMessage(TO_WSTRING(notice).c_str(), Color);
        //printf("showmesggg");
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x1215)
    {
        byte GlobalOperationFlag;
        *msg >> GlobalOperationFlag;

        if(GlobalOperationFlag == 1)
        {
            byte GlobalOperationType;
            std::n_string GlobalOperationName;
            *msg >> GlobalOperationType >> GlobalOperationName >> GlobalOperationFlag;
        }
        byte ShardFlag;
        *msg >> ShardFlag;

        while(ShardFlag == 1)
        {
            unsigned short ShardID;
            std::n_string ShardName;
            unsigned short ShardCurrent;

            unsigned short ShardCapacity;
            byte ShardStatus;
            byte GlobalOperationID;
            *msg >> ShardID >> ShardName >> ShardCurrent >> ShardCapacity >> ShardStatus >> GlobalOperationID;
            *msg >> ShardFlag;
            RememberShardStatusTexture(SelectShardStatusTexture(ShardStatus, ShardCurrent, ShardCapacity));

            if(ShardStatus == 0)
            {
                this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_black.ddj", 1, 1);
                this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_black.ddj", 1, 1);

            }
            else if(ShardStatus == 1)
            {
                double yuzde_degeri;
                if (ShardCapacity == 0) {
                    yuzde_degeri = 0;
                } else {
                    yuzde_degeri = (static_cast<double>(ShardCurrent) / ShardCapacity) * 100;

                }
                if(yuzde_degeri < 50)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_green.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_green.ddj", 1, 1);
                }
                else if(yuzde_degeri >= 50 && yuzde_degeri < 80)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_yellow.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_yellow.ddj", 1, 1);
                }
                else if(yuzde_degeri >= 80)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_red.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_red.ddj", 1, 1);

                }
            }

        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x1216) /// for duck
    {
        int sCount;
        *msg >> sCount;

        int i = 0;
        while (i < sCount && sCount > 0) {
            i++;
            byte ShardStatus;
            *msg >> ShardStatus;

            unsigned short ShardCapacity;
            unsigned short ShardCurrent;
            *msg >> ShardCapacity;
            *msg >> ShardCurrent;
            RememberShardStatusTexture(SelectShardStatusTexture(ShardStatus, ShardCurrent, ShardCapacity));

            if(ShardStatus == 0)
            {
                this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_black.ddj", 1, 1);
                this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_black.ddj", 1, 1);

            }
            else if(ShardStatus == 1)
            {
                double yuzde_degeri;
                if (ShardCapacity == 0) {
                    yuzde_degeri = 0;
                } else {
                    yuzde_degeri = (static_cast<double>(ShardCurrent) / ShardCapacity) * 100;

                }
                if(yuzde_degeri < 50)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_green.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_green.ddj", 1, 1);
                }
                else if(yuzde_degeri >= 50 && yuzde_degeri < 80)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_yellow.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_yellow.ddj", 1, 1);
                }
                else if(yuzde_degeri >= 80)
                {
                    this->m_IRM.GetResObj(1929, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_red.ddj", 1, 1);
                    this->m_IRM.GetResObj(1927, 1)->TB_Func_13("clientlibrary\\charselect\\login_window_eu_located_red.ddj", 1, 1);

                }
            }

        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x1210) {
        // This legacy configuration layout is variable-length. Reject packets
        // that cannot contain its fixed prefix before any settings, UI access,
        // or compatibility patch is touched. Full field migration remains
        // tracked in the repair report because native string reads need a
        // protocol-by-protocol conversion to SafePacketReader.
        if (msg->Remaining() < 48 || msg->Remaining() > 0x10000) {
            msg->FlushRemaining();
            return false;
        }
        //if (g_CGame->langId == 2) {
        //    const wchar_t *msg = L"Bu sunucu Lexa Shield tarafından korunmaktadır.";
        //    this->ShowMessage(msg, 0x87ceeb);
        //} else if (g_CGame->langId == 5) {
        //    this->ShowMessage(L"This server protected by Lexa Shield.", 0x87ceeb);
        //}

        if (theApp.GetCurrentProcess() != NULL) {
            std::n_wstring tag = L"KMTGuard";
            SetWindowText(theApp.GetHWnd(), TO_STRING(tag).c_str());
            SetWindowLong(theApp.GetHWnd(), GWL_STYLE,
                          GetWindowLong(theApp.GetHWnd(), GWL_STYLE) | WS_MINIMIZEBOX);
        }

        replaceOffset(0x00699AE8, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetObject));
        replaceOffset(0x00699AFA, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetName));
        replaceOffset(0x00699CA2, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetObject));
        replaceOffset(0x00699CBE, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetName));
        replaceOffset(0x0069A0F3, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetObject));
        replaceOffset(0x0069A105, addr_from_this(&CIFTargetWindowCommonEnemy::SetTargetName));
        replaceOffset(0x00699885, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetObject));
        replaceOffset(0x0069989D, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetName));
        replaceOffset(0x006998FA, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetObject));
        replaceOffset(0x00699912, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetName));
        replaceOffset(0x00699984, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetObject));
        replaceOffset(0x0069999C, addr_from_this(&CIFTargetWindowSpecialMob::SetTargetName));
        *msg >> m_Settings->OldLogin; //1
        *msg >> m_Settings->OldExpBar;//2
        *msg >> m_Settings->OldAlchemy;//3
        *msg >> m_Settings->GrantName;//4
        *msg >> m_Settings->IconManager;//5
        *msg >> m_Settings->IconManagerRight;//6
        *msg >> m_Settings->TitleManager;//7
        *msg >> m_Settings->TitleManagerColor;//8
        *msg >> m_Settings->RankingWnd;//9
        *msg >> m_Settings->UniqueHistoryWnd; //10
        *msg >> m_Settings->EventRegisterWnd; //11
        *msg >> m_Settings->EventScheduleWnd;//12
        *msg >> m_Settings->AchievementsWnd;//13
        *msg >> m_Settings->SecondarySlot;//14
        *msg >> m_Settings->MoveSkillBoard >>m_Settings->EnableServerInfoSkill >> m_Settings->EnableOldMainPopup >> m_Settings->HideTitleWhileTagActive;
        if(m_Settings->OldLogin)
        {

            replaceOffset(0x00860990, addr_from_this(&CPSCharacterSelect::aCPSCharacterSelect));
            replaceOffset(0x0085cb8b, addr_from_this(&CCameraWorking::AddKeyframeForSelectIdol));
        }
        if(m_Settings->OldExpBar)
        {
            /// old undermenu var
            // replaceAddr(0x00dd182c, addr_from_this(&CIFCOSManager::OnCreateIMPL));
            // replaceAddr(0x00dba97c, addr_from_this(&CIFCOSCommand::OnCreateIMPL));

            replaceOffset(0x007a2858, addr_from_this(&CIFCOSCommand::FUN_007a20a0));
            replaceOffset(0x0081145c, addr_from_this(&CIFCOSCommand::FUN_007a20a0));

            PatchMe(0x00BA2156 + 7, 0XB0);

            PatchMe(0x00BA2156 + 6, 0x36);
        }
        if(m_Settings->OldAlchemy)
        {
            /// old alchemy

            static const char *alchemy_str = "icon\\alchemy_slot_nothing.ddj";
            replaceAddr(0x00574389+1, (int)(alchemy_str));
            replaceAddr(0x00574CC8+1, (int)(alchemy_str));
            replaceAddr(0x0057510C+1, (int)(alchemy_str));
            replaceAddr(0x0057584C+1, (int)(alchemy_str));
            replaceAddr(0x00577159+1, (int)(alchemy_str));
            replaceAddr(0x00578005+1, (int)(alchemy_str));
            replaceAddr(0x0057871B+1, (int)(alchemy_str));
            replaceAddr(0x0057A89C+1, (int)(alchemy_str));
            replaceAddr(0x0059C98A+1, (int)(alchemy_str));
        }

        if (m_Settings->EnableServerInfoSkill) {
            replaceOffset(0x00803d96, addr_from_this(&CIFStateSlot::FUN_00806d50));
        }

        *msg >> m_Settings->ItemComparison;
        if(m_Settings->ItemComparison)
        {

           replaceOffset(0x00682AFC, addr_from_this(&CIFSlotWithHelp::AppendAdvancedInfo));
           replaceOffset(0x00682D6E, addr_from_this(&CIFSlotWithHelp::AppendAdvancedInfo));
           replaceOffset(0x00682FBE, addr_from_this(&CIFSlotWithHelp::AppendAdvancedInfo));
           replaceOffset(0x0068320E, addr_from_this(&CIFSlotWithHelp::AppendAdvancedInfo));
        }
        *msg >> m_Settings->AutoSortButton;

        *msg >> m_Settings->EnablePtMemberViewer;

        *msg >> m_Settings->EnableAutoSkill;
        if(m_Settings->EnableAutoSkill)
        {
            static const char *skillboard = "clientlibrary\\resinfo\\ifskillboard.txt";
            replaceAddr(0x0069c4f4+1, (int)(skillboard));
        }
        *msg >> m_Settings->MaxMasteryLevel;
        m_Settings->ChineseMasteryLimit = m_Settings->MaxMasteryLevel;
        m_Settings->EuropeanMasteryLimit = m_Settings->MaxMasteryLevel;

        *msg >> m_Settings->ServerMaxLevel;

        WriteMemoryValue<byte>(0x008A99A2 + 2, m_Settings->ServerMaxLevel);// Level up limit
        WriteMemoryValue<byte>(0x0069C7C8 + 1, m_Settings->ServerMaxLevel);// Mastery limit


        KmtApplyConfiguredMasteryLimits();

        WriteMemoryValue<byte>(0x0073AFAE + 1, m_Settings->ServerMaxLevel);// Party Match
        WriteMemoryValue<byte>(0x0073B013 + 1, m_Settings->ServerMaxLevel);// Party Match
        WriteMemoryValue<byte>(0x0073B030 + 1, m_Settings->ServerMaxLevel);// Party Match
        WriteMemoryValue<byte>(0x0073FA4C + 1, m_Settings->ServerMaxLevel);// Party Match
        WriteMemoryValue<byte>(0x0073FAAF + 1, m_Settings->ServerMaxLevel);// Party Match
        WriteMemoryValue<byte>(0x0073FACC + 1, m_Settings->ServerMaxLevel);// Party Match

        byte DamageText;
        *msg >> DamageText;
        if (DamageText == 1) {
            vftableHook(0x00de54ec, 9, addr_from_this(&CIDamageText::Func_9_IMPL));
        }

        *msg >> m_Settings->EnableAutoStrInt;

        *msg >> m_Settings->EnablePickSoxEffect;

        byte permanentalchemy;
        *msg >> permanentalchemy;
        if (permanentalchemy == 1) {
            PatchAlchemyPerm();

            JMPFunction(0x0059C95D, 0x0059CA04);
            RenderNop((void *) 0x0059C962, 1);
            RenderNop((void *) 0x0059C963, 1);
            RenderNop((void *) 0x0059C964, 1);
            RenderNop((void *) 0x0059CA0A, 1);
            RenderNop((void *) 0x0059CA0A, 1);
            RenderNop((void *) 0x0059CA24, 1);
            RenderNop((void *) 0x0059CA25, 1);
            RenderNop((void *) 0x0059CA26, 1);
            RenderNop((void *) 0x0059CA27, 1);
            RenderNop((void *) 0x0059CA28, 1);
            RenderNop((void *) 0x0059DF54, 1);
            RenderNop((void *) 0x0059DF55, 1);
            RenderNop((void *) 0x0059DF56, 1);
            RenderNop((void *) 0x0059DF57, 1);
            RenderNop((void *) 0x0059DF58, 1);
            RenderNop((void *) 0x0059DF59, 1);
            RenderNop((void *) 0x0059DF5A, 1);
            RenderNop((void *) 0x0059DF61, 1);
            RenderNop((void *) 0x0059DF62, 1);
            RenderNop((void *) 0x0059DF63, 1);
            RenderNop((void *) 0x0059DF64, 1);
            RenderNop((void *) 0x0059DF65, 1);
        }

        *msg >> m_Settings->GuildJobMode;

        *msg >> m_Settings->EnableUniqueTarget;
        if (m_Settings->EnableUniqueTarget == 1) {
            replaceOffset(0x00779aaa, addr_from_this(&CICPlayer::EffectHook));
        }
        *msg >> m_Settings->EnableMacro;
        byte secondpw;
        *msg >> secondpw >> m_Settings->NewCharInfoScreen >> m_Settings->NewIDPWScreen;
        *msg >> m_Settings->ServerName;
        if (m_Settings->ServerName.empty())
            m_Settings->ServerName = std::n_string("KMTGuard");

        if(!m_Settings->PSTitleIsLoaded)
        {
            if(m_Settings->NewIDPWScreen)
            {

                this->m_IRM.GetResObj(1926, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1927, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1928, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1929, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1930, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1931, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(1930, 1)->SetText(KmtGetText(L"UIIT_KMT_LOCATION"));
                this->m_IRM.GetResObj(104, 1)->SetText(KmtGetText(L"UIIT_KMT_CHANNEL"));

                wnd_pos servernamelist = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->GetPos();
                wnd_size servernamelistsize = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->GetSize();
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->MoveGWnd(servernamelist.x + 22, servernamelist.y);
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10000)->SetGWndSize(servernamelistsize.width - 22, servernamelistsize.height - 14);

                wnd_pos servernamelistx = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->GetPos();
                wnd_size servernamelistsizex = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->GetSize();
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->MoveGWnd(servernamelistx.x + 38, servernamelistx.y);
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10001)->SetGWndSize(servernamelistsizex.width + 4, servernamelistsizex.height);


                wnd_pos servernamelistxx = this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10002)->GetPos();
                wnd_size servernamelistsizexx = this->GetSize();
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->GetGuiFromList<CIFSimpleListCtrl>(10002)->MoveGWnd(servernamelistxx.x + 60, servernamelistxx.y);



                this->m_IRM.GetResObj<CIFButton>(46, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj<CIFButton>(46, 1)->SetEnabledState(false);
                this->m_IRM.GetResObj<CIFButton>(46, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);
                this->m_IRM.GetResObj(45, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(104, 1)->ShowGWnd(true);
                //  this->m_IRM.GetResObj(15, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(43, 1)->ShowGWnd(true);
                this->m_IRM.GetResObj(45, 1)->SetText(TO_WSTRING(m_Settings->ServerName).c_str());
                wnd_pos tt =  this->m_IRM.GetResObj(104, 1)->GetPos();
                this->m_IRM.GetResObj(104, 1)->MoveGWnd(tt.x - 22, tt.y - 2);

                wnd_pos t5 =  this->m_IRM.GetResObj(45, 1)->GetPos();
                this->m_IRM.GetResObj(45, 1)->MoveGWnd(t5.x - 17, t5.y - 2);
                this->m_IRM.GetResObj(45, 1)->SetGWndSize(198, 17);

                wnd_pos chlist =  this->m_IRM.GetResObj(46, 1)->GetPos();
                this->m_IRM.GetResObj(46, 1)->MoveGWnd(chlist.x + 94, chlist.y - 4);


                wnd_pos logo;
                logo = this->m_IRM.GetResObj(GDR_LOGO, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_LOGO, 1)->MoveGWnd(logo.x, logo.y - 60);


                wnd_pos x;
                x = this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->TB_Func_13("clientlibrary\\charselect\\newlogin.ddj", 1, 1);
                this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->SetGWndSize(400, 190);
                this->m_IRM.GetResObj(GDR_IDPWFRAME, 1)->MoveGWnd(x.x - 56, x.y - 47);




                wnd_pos connect;
                connect = this->m_IRM.GetResObj(GDR_CONNECTBTN, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_CONNECTBTN, 1)->MoveGWnd(connect.x - 10, connect.y);


                wnd_pos exit;
                exit = this->m_IRM.GetResObj(GDR_EXIT, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_EXIT, 1)->MoveGWnd(exit.x - 6, exit.y);

                wnd_pos listbtn;
                listbtn = this->m_IRM.GetResObj(GDR_LISTBTN, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_LISTBTN, 1)->MoveGWnd(listbtn.x + 94, listbtn.y - 2);


                wnd_pos id;
                id = this->m_IRM.GetResObj(GDR_ID, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_ID, 1)->SetGWndSize(138, 17);
                this->m_IRM.GetResObj(GDR_ID, 1)->MoveGWnd(id.x - 17, id.y - 2);


                wnd_pos pw;
                pw = this->m_IRM.GetResObj(GDR_PW, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_PW, 1)->SetGWndSize(198, 17);
                this->m_IRM.GetResObj(GDR_PW, 1)->MoveGWnd(pw.x - 17, pw.y - 2);


                wnd_pos sw;
                sw = this->m_IRM.GetResObj(GDR_SWNAME, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_SWNAME, 1)->SetGWndSize(198, 17);
                this->m_IRM.GetResObj(GDR_SWNAME, 1)->MoveGWnd(sw.x - 17, sw.y - 2);


                wnd_pos id1;
                id1 = this->m_IRM.GetResObj(GDR_101ID, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_101ID, 1)->SetGWndSize(53, 16);
                this->m_IRM.GetResObj(GDR_101ID, 1)->MoveGWnd(id1.x - 22, id1.y - 5);


                wnd_pos pw1;
                pw1 = this->m_IRM.GetResObj(GDR_102PW, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_102PW, 1)->SetGWndSize(53, 16);
                this->m_IRM.GetResObj(GDR_102PW, 1)->MoveGWnd(pw1.x - 22, pw1.y - 4);


                wnd_pos sw1;
                sw1 = this->m_IRM.GetResObj(GDR_103SW, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_103SW, 1)->SetGWndSize(53, 16);
                this->m_IRM.GetResObj(GDR_103SW, 1)->MoveGWnd(sw1.x - 22, sw1.y - 3);


                wnd_pos list = this->m_IRM.GetResObj(GDR_LIST, 1)->GetPos();
                this->m_IRM.GetResObj(GDR_LIST, 1)->TB_Func_13("clientlibrary\\charselect\\server_global_window.ddj", 1, 1);
                this->m_IRM.GetResObj(GDR_LIST, 1)->SetGWndSize(312, 340);
                this->m_IRM.GetResObj(GDR_LIST, 1)->MoveGWnd(list.x - 36, list.y);

                wnd_pos sliderctrl = this->m_IRM.GetResObj(54, 1)->GetPos();
                this->m_IRM.GetResObj(54, 1)->MoveGWnd(sliderctrl.x + 69, sliderctrl.y - 3);



                wnd_pos serverlist = this->m_IRM.GetResObj(51, 1)->GetPos();
                wnd_size serverlistsize = this->m_IRM.GetResObj(51, 1)->GetSize();
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->MoveGWnd(serverlist.x, serverlist.y +14);
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->SetGWndSize(serverlistsize.width+71, serverlistsize.height-14);
                this->m_IRM.GetResObj<CIFOListCtrl>(51, 1)->N00001C53 = serverlistsize.width + 71;


            }
            else
            {
                this->m_IRM.GetResObj(1926, 1)->ShowGWnd(false);
                this->m_IRM.GetResObj(1927, 1)->ShowGWnd(false);
                this->m_IRM.GetResObj(1928, 1)->ShowGWnd(false);
                this->m_IRM.GetResObj(1929, 1)->ShowGWnd(false);
                this->m_IRM.GetResObj(1930, 1)->ShowGWnd(false);
                this->m_IRM.GetResObj(1931, 1)->ShowGWnd(false);


            }
        }
        SyncLoginRegisterButton();
        QuickLogin_SyncPanel(this);
        *msg >> m_CustomDataManager->FacebookUrl >> m_CustomDataManager->DiscordUrl >> m_CustomDataManager->WebSiteUrl;
        *msg >> m_Settings->EnableChangeLog >> m_Settings->ShowChangeLogFirstLogin >> m_Settings->FixNewJobSuit;
        *msg >> m_Settings->EnableOldItemMall;
        *msg >> m_Settings->InsertCommaToPrices;
        *msg >> m_Settings->EnableCharacterBound;
        *msg >> m_Settings->EnableNewItemMall;
        if(m_Settings->EnableNewItemMall)
        {
            replaceOffset(0x0060cdb9, addr_from_this(&CGInterface::Wtf));
        }
        replaceOffset(0x006ec77b, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x004f8e51, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x00676cc3, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x0086252a, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x0086c5a8, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x0087acdd, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        replaceOffset(0x00883af9, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
        vftableHook(0x00d9b9cc, 3, addr_from_this(&CIFMessageBox::MessageMap));
        replaceOffset(0x006BA8AB, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
        replaceOffset(0x006BB133, addr_from_this(&CIFMessageBox::OnClickConfirm));

        if(m_Settings->EnableOldItemMall)
        {
            replaceAddr(0xDC8A48, addr_from_this(&CIFItemMall::OnCloseWnd_IMPL));
            vftableHook(0x0D9B9CC, 12, addr_from_this(&CIFMessageBox::OnUpdateIMPL));

            HOOK_ORIGINAL_MEMBER(0x007D0460, &CIFItemMallZzim::OnBuyAll_BtnClick);
            HOOK_ORIGINAL_MEMBER(0x007D0350, &CIFItemMallZzim::OnBuyAllCallBack);
            HOOK_ORIGINAL_MEMBER(0x0079B580, &CGInterface::OnItemMallSectionControl);

            /* replaceOffset(0x007bfe7b, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007c0646, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007cf114, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007d0318, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007d042c, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007d80e2, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007d83da, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007db0d5, addr_from_this(&CGInterface::OnReservedItemBuy));
             replaceOffset(0x007db3a0, addr_from_this(&CGInterface::OnReservedItemBuy));

             replaceOffset(0x00652c9f, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x007879b2, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x00798774, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x007d16c8, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x0087e00e, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x008a1474, addr_from_this(&CGInterface::OnItemMallSectionControl));
             replaceOffset(0x0087e31a, addr_from_this(&CGInterface::OnItemMallSectionControl));

             replaceOffset(0x007D0350, addr_from_this(&CIFItemMallZzim::OnBuyAllCallBack));
             replaceOffset(0x007d0659, addr_from_this(&CIFItemMallZzim::OnBuyAll_BtnClick));*/


        }

        *msg >> m_Settings->ENABLE_EMOJI_SYSTEM;
        if(m_Settings->ENABLE_EMOJI_SYSTEM)
        {
            replaceOffset(0x0063570B, addr_from_this(&CGFontTexture::Render_0));
            replaceOffset(0x00639FE0, addr_from_this(&CGFontTexture::Render_0));
            replaceOffset(0x008B4EA1, addr_from_this(&CGFontTexture::Render_1));

        }
        *msg >> m_Settings->ENABLE_NEW_PT_MATCH;


        *msg >> m_Settings->ENABLE_NEW_JOB_UI;

        // Consume the legacy settings field to preserve packet alignment, but do
        // not expose the retired custom New Alchemy interface.
        byte legacyNewAlchemySetting;
        *msg >> legacyNewAlchemySetting;
        m_Settings->ENABLE_NEW_ALCHEMY = false;

        *msg >> m_Settings->EnableItemTranslation;
        *msg >> m_Settings->ItemTranslationPayment;
        *msg >> m_Settings->ItemTranslationPrice;

        *msg >> m_Settings->NonClosePTForm;
        *msg >> m_Settings->EnableLuckySpin;
        *msg >> m_Settings->EnableLuckySpinSilk;
        *msg >> m_Settings->LuckySpinPrice;
        *msg >> m_Settings->ShowGuideMenu;
        *msg >> m_Settings->ShowGuideLuckySpin;
        *msg >> m_Settings->ShowGuideItemChest;
        *msg >> m_Settings->ShowGuideMacro;
        *msg >> m_Settings->ShowGuideDailyLogin;
        *msg >> m_Settings->ShowGuideDiscord;
        *msg >> m_Settings->ShowGuideWebsite;
        *msg >> m_Settings->ShowGuideFacebook;
        *msg >> m_Settings->ShowGuideAutoEquip;
        *msg >> m_Settings->ShowGuideWebViewer;
        *msg >> m_Settings->ShowGuideMapLocation;
        *msg >> m_Settings->EnableSpecialOffers;
        *msg >> m_Settings->ShowGuideSpecialOffers;
        *msg >> m_Settings->ShowGuideDropLogs;
        *msg >> m_Settings->EnableQuickLogin;
        *msg >> m_Settings->EnablePvpChallenge;
        *msg >> m_Settings->ShowGuidePvpChallenge;
        *msg >> m_Settings->ShowGuideKillerAnimation;
        if (msg->m_currentReadBytes < msg->m_availableBytesForReading) {
            *msg >> m_Settings->NewInventoryDesign;
        }
        if (msg->m_currentReadBytes < msg->m_availableBytesForReading) {
            *msg >> m_Settings->EnableOfflineStall;
        }
        if (msg->m_currentReadBytes < msg->m_availableBytesForReading) {
            *msg >> m_Settings->MenuLikeMaxi;
        }
        if (msg->m_currentReadBytes + sizeof(int) <= msg->m_availableBytesForReading) {
            *msg >> m_Settings->AutoEquipMaxLevel;
            if (m_Settings->AutoEquipMaxLevel < 0) {
                m_Settings->AutoEquipMaxLevel = 0;
            }
        }
        if (msg->m_currentReadBytes < msg->m_availableBytesForReading) {
            *msg >> m_Settings->MenuCasy;
        }
        if (msg->m_currentReadBytes <= msg->m_availableBytesForReading &&
            msg->m_availableBytesForReading - msg->m_currentReadBytes >= sizeof(int) * 2) {
            int chineseMasteryLimit = 0;
            int europeanMasteryLimit = 0;
            *msg >> chineseMasteryLimit >> europeanMasteryLimit;
            if (chineseMasteryLimit >= 1 && chineseMasteryLimit <= 10000 &&
                europeanMasteryLimit >= 1 && europeanMasteryLimit <= 10000) {
                m_Settings->ChineseMasteryLimit = chineseMasteryLimit;
                m_Settings->EuropeanMasteryLimit = europeanMasteryLimit;
                KmtApplyConfiguredMasteryLimits();
            }
        }
        // Publish the settings snapshot only after every optional tail field
        // has been consumed.
        m_Settings->PSTitleIsLoaded = true;
        ConfigureInventoryDesignPatches(m_Settings->NewInventoryDesign);
        QuickLogin_SetEnabled(this, m_Settings->EnableQuickLogin);
        if(m_Settings->NonClosePTForm)
        {
            PatchJZtoJMP((void*)0x0073884D);

        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0xA340)//notice
    {
        std::n_wstring notice;
        *msg >> notice;;

        this->ShowMessage(notice.c_str(), 0xFFFF671D);

        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x166B)
    {
        byte success;
        std::n_wstring notice;
        *msg >> success >> notice;

        CIFLoginRegisterWnd* registerWnd = CIFLoginRegisterWnd::GetActiveWindow();
        if (!registerWnd) {
            registerWnd = this->m_IRM.GetResObj<CIFLoginRegisterWnd>(LOGIN_REGISTER_WINDOW_ID, 1);
        }

        if (registerWnd) {
            registerWnd->HandleRegisterResponse(success != 0, notice.c_str());
        } else {
            this->ShowMessage(notice.c_str(), success ? 0xFF00FF00 : 0xFFFF671D);
        }

        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x1671)
    {
        byte success;
        std::n_string username;
        std::n_string token;
        std::n_wstring notice;
        *msg >> success >> username >> token >> notice;

        QuickLogin_HandleCreateTokenResult(this, success != 0, username, token, notice.c_str());
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x1673)
    {
        byte success;
        std::n_wstring notice;
        *msg >> success >> notice;

        QuickLogin_HandleQuickLoginResult(this, success != 0, notice.c_str());
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x1675)
    {
        byte success;
        std::n_wstring notice;
        *msg >> success >> notice;

        QuickLogin_HandleRevokeTokenResult(this, success != 0, notice.c_str());
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == OFFLINE_STALL_LOGIN_PROMPT_OPCODE)
    {
        const unsigned int remaining = msg->m_currentReadBytes <= msg->m_availableBytesForReading
            ? msg->m_availableBytesForReading - msg->m_currentReadBytes : 0;
        if (remaining >= sizeof(DWORD) + sizeof(BYTE) + sizeof(WORD)) {
            DWORD token = 0;
            BYTE timeoutSeconds = 5;
            std::n_string characterName;
            *msg >> token >> timeoutSeconds >> characterName;
            if (characterName.length() > 64) {
                characterName.resize(64);
            }
            OfflineStall_OpenLoginPrompt(this, token, timeoutSeconds, characterName, GDR_CONNECTBTN);
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x1212)
    {
        byte type;
        *msg >> type;
        byte locale;
        std::n_string UserName;
        std::n_string Password;
        unsigned __int16 wShardID;

        /**msg >> locale;
        *msg >> UserName;
        *msg >> Password;
        *msg >> wShardID;*/
        if(type == eResultType::CREATE_THE_PASSWORD)
        {
            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);

            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetMode(0);

            this->ShowMessage(KmtGetText(L"UIIT_KMT_YOU_WILL_CREATE_A_SECONDARY_PASSCODE_FOR_FURTHER_PROTECTION_OF_YOUR_ACCOUNT"), 0xFFFF671D);


            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == eResultType::ENTER_THE_PASSWORD) /// enter pass
        {
            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);


            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->UpdateMenuSize();
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetMode(1);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetRememberPC(false);

            CGEffSoundBody::get()->PlaySound(L"snd_error");

        }
        else if(type == eResultType::CREATED_SUCCESS_ENTER_THE_PASSWORD)
        {
            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);

            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->UpdateMenuSize();
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetMode(1);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetRememberPC(false);

            this->ShowMessage(
                KmtGetText(L"UIIT_KMT_PASSWORD_CREATED_SUCCESSFULLY_ENTER_THE_PASSCODE"),
                0xFFFF671D);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ClearAll();

            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == eResultType::WRONG_PASSWORD)
        {

            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);

            this->ShowMessage(KmtGetText(L"UIIT_KMT_INVALID_PASSCODE_IF_YOU_ENTER_THE_WRONG_PASSCODE_MORE_THAN_SIX_TIMES_YOU_WILL_04268C57"), 0xFFFF671D);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == eResultType::PW_TRUE)
        {
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(true);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(true);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("interface\\outer\\button.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("interface\\outer\\button.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(true);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("interface\\outer\\list_button.ddj",1,1);


            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(false);
            SyncLoginRegisterButton();
            QuickLogin_SyncPanel(this);
            this->ShowMessage(KmtGetText(L"UIIT_KMT_SECONDARY_PASSCODE_HAS_BEEN_ENTERED_SUCCESFULLY"), 0xFFFF671D);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == eResultType::PASSWORD_CHANGED_ENTER_THE_PASSWORD)
        {
            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->UpdateMenuSize();
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetMode(1);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetRememberPC(false);

            this->ShowMessage(KmtGetText(L"UIIT_KMT_PASSWORD_CHANGED_SUCCESFULLY_ENTER_THE_NEW_PASSCODE"), 0xFFFF671D);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ClearAll();

            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == eResultType::WRONG_PASSWORD)
        {
            SetLoginRegisterButtonVisible(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(GDR_CONNECTBTN, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);
            this->m_IRM.GetResObj<CIFButton>(12, 1)->TB_Func_13("clientlibrary\\common\\button_disable.ddj", 1, 1);

            this->m_IRM.GetResObj<CIFButton>(44, 1)->SetEnabledState(false);
            this->m_IRM.GetResObj<CIFButton>(44, 1)->TB_Func_13("clientlibrary\\common\\list_button_disable.ddj",1,1);


            this->ShowMessage(KmtGetText(L"UIIT_KMT_INVALID_PASSCODE_IF_YOU_ENTER_THE_WRONG_PASSCODE_MORE_THAN_SIX_TIMES_YOU_WILL_04268C57"), 0xFFFF671D);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
       /* else if(type == 2)
        {
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->UpdateMenuSize();
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->ShowGWnd(true);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetMode(type);

            this->m_IRM.GetResObj<CIFSecondaryPassword>(5000, 1)->SetRememberPC(false);


            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_CREATE_NUMBER"), 0xFFFFF0);


            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == 3)
        {

            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->SetMode(1);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->SetRememberPC(true);
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_ENTERED_SUCCESS"), 0xFFFFF0);


            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == 5)
        {
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_ENTER_ERROR"), 0xFFFFF0);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == 6)
        {
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_ENTERED_SUCCESS"), 0xFFFFF0);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->ClearAll();
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->ShowGWnd(false);
        }
        else if(type == 7)
        {
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->SetMode(1);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->ClearAll();
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_CREATE_SUCCESS"), 0xFFFFF0);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == 8)
        {
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->SetMode(0);
            this->m_IRM.GetResObj<CIFSecondaryPassword>(300, 1)->ClearAll();
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_SECOND_PASSWORD_REMOVED_SUCCESS"), 0xFFFFF0);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }
        else if(type == 9)
        {
            this->ShowMessage(TSM_GETTEXTPTR(L"VFILTER_HWID_ERROR"), 0xFFFFF0);
            CGEffSoundBody::get()->PlaySound(L"snd_error");
        }*/
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x165A)
    {
        std::n_string challenge;
        *msg >> challenge;
        HWIDGenerator hwidGenerator;
        std::string hwid = hwidGenerator.BuildChallengeResponse(challenge.c_str());

        CMsgStreamBuffer buf(0x165B);
        buf << std::n_string(hwid.c_str());
        SendMsg(buf);

        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x208F)
    {
        m_CustomDataManager->CustomItemMallItemList.clear();
        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;
            int ID;
            std::n_string CategoryName;
            byte Type;
            int ItemID;
            int ItemCount;
            int Silk;
            int ItemIndex;
            byte ShowInNewBest;
            int NewBestItemIndex;

            *msg >> ID;
            *msg >> CategoryName;
            *msg >> Type;
            *msg >> ItemID;
            *msg >> ItemCount;
            *msg >> Silk;

            *msg >> ShowInNewBest;

            *msg >> ItemIndex;

            *msg >> NewBestItemIndex;
            CustomDataManager::CustomItemMallItemStruct Data = CustomDataManager::CustomItemMallItemStruct();
            Data.ID = ID;
            Data.Category = TO_NWSTRING(CategoryName);
            Data.Type = Type;
            Data.ItemID = ItemID;
            Data.ItemCount = ItemCount;
            Data.SilkPrice = Silk;
            Data.ShowInNewBest= ShowInNewBest;
            Data.ItemIndex = ItemIndex;
            Data.NewBestItemIndex = NewBestItemIndex;
            m_CustomDataManager->CustomItemMallItemList.insert((std::make_pair(ID, Data)));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x209B)
    {
        m_CustomDataManager->AvatarMallItemList.clear();
        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;
            int ID;
            byte Type;
            int ItemID;
            int Silk;
            int ItemIndex;
            int PetObjID;
            *msg >> ID;
            *msg >> Type;
            *msg >> ItemID;
            *msg >> Silk;
            *msg >> ItemIndex;
            *msg >> PetObjID;

            CustomDataManager::AvatarMallStruct Data = CustomDataManager::AvatarMallStruct();
            Data.ID = ID;
            Data.CategoryID = Type;
            Data.ItemID = ItemID;
            Data.Silk = Silk;
            Data.ItemIndex = ItemIndex;
            Data.PetObjID = PetObjID;
            m_CustomDataManager->AvatarMallItemList.insert((std::make_pair(ID, Data)));
        }
        msg->m_currentReadBytes = 0;
    }
    return reinterpret_cast<int(__thiscall *)(CPSTitle *, CMsgStreamBuffer *)>(
        0x0086bfc0)(this, msg) != 0;
}
void CPSTitle::PatchAlchemyPerm() {
#pragma pack(push, 1)
    struct {
        BYTE opcode;
        BYTE puch;
    } Pushing;
#pragma pack(pop)
    Pushing.opcode = 0x6A;// push (constant)
    Pushing.puch = 1;
    CopyBytes(0x0059EB2F, &Pushing, sizeof(Pushing));
}
