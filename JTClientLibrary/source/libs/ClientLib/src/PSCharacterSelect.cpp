#include "PSCharacterSelect.h"
#include "Game.h"
#include "IFButton.h"
#include "IFFade.h"
#include "IFStatic.h"
#include "IFTextBox.h"
#include "IFWnd.h"
#include "Keyframe.h"
#include "World.h"
#include <support/hook.h>
#include "GInterface.h"
#include "KillerAnimationPlayer.h"
#include "TextStringManager.h"
#include "CObjCharacter.h"
#include "ICPlayer.h"
#include <BSLib/Debug.h>

#include <BSLib/multibyte.h>
#include <ctime>
#include <cstdlib>
#include <CustomData/CustomDataManager.h>

#include <CustomData/CustomCICPlayer.h>
#include <CustomData/CustomSettingManager.h>
#include <CustomInterface/IFQuickLoginPanel.h>
#include <CustomInterface/IFOfflineStall.h>
#include <sstream>
#include <Hwid/HWIDGenerator.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace {
    bool IsMemoryDiagnosticsEnabled() {
        char value[8] = {0};
        const DWORD length = GetEnvironmentVariableA("KMTGUARD_MEMORY_DIAGNOSTICS", value, sizeof(value));
        return length > 0 && value[0] != '0';
    }

    SIZE_T GetCurrentWorkingSetBytes() {
        PROCESS_MEMORY_COUNTERS counters = {0};
        counters.cb = sizeof(counters);
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
            return 0;
        return counters.WorkingSetSize;
    }

    void LogCharacterSelectMemory(const char* phase, DWORD elapsedMs, SIZE_T beforeBytes,
                                  size_t characterCount) {
        if (!IsMemoryDiagnosticsEnabled())
            return;

        char line[256] = {0};
        _snprintf(line, sizeof(line) - 1,
                  "[KMTGuardMemory][CharacterSelect] %s elapsed_ms=%lu working_set_before=%lu working_set_after=%lu characters=%u\n",
                  phase,
                  static_cast<unsigned long>(elapsedMs),
                  static_cast<unsigned long>(beforeBytes),
                  static_cast<unsigned long>(GetCurrentWorkingSetBytes()),
                  static_cast<unsigned int>(characterCount));
        OutputDebugStringA(line);
    }
}


GlobalVar<bool, 0x00EED9B8> g_bIsUnityServer;


const Keyframe frames_intro[] = {
        Keyframe(144.0f, -927.0f, 583.0f, 0.1f, 3.15f, 0.0f, 30.0f),
        Keyframe(148.0f, -927.639f, 536.0f, 0.1f, 3.15f, 0.0f, 30.0f),
};

#define CHARSEL_CHARACTER_POSITION_YOFFSET 0x00DD7C98
#define CHARSEL_CHARACTER_POSITION_YOFFSETSITDOWN 0x00dd7ca0

#define LIZARD_X 0x0085F675

void CPSCharacterSelect::aCPSCharacterSelect() {

    m_Characterselect.N00000058.position.X = 502.171f;
    m_Characterselect.N00000058.position.Y = 8.12451f;
    m_Characterselect.N00000058.position.Z = 241.01f;

    m_Characterselect.N00000058.rotation.X = 0.099325f;
    m_Characterselect.N00000058.rotation.Y = 0.752854f;
    m_Characterselect.N00000058.rotation.Z = 0;

    m_Characterselect.N00000058.distance = 80.0f;
    m_Characterselect.N00000058.time = 0.0f; // time is preset and ignored


    m_Characterselect.N00000059.position.X = 504.531f;
    m_Characterselect.N00000059.position.Y = 8.12451f;
    m_Characterselect.N00000059.position.Z = 235.514f;

    m_Characterselect.N00000059.rotation.X = 0.099325f;
    m_Characterselect.N00000059.rotation.Y = 1.16525f;
    m_Characterselect.N00000059.rotation.Z = 0;

    m_Characterselect.N00000059.distance = 80.0;
    m_Characterselect.N00000059.time = 6.0f; // time is preset and ignored

    m_Characterselect.N0000005A.position.X = 478.134f;
    m_Characterselect.N0000005A.position.Y = 10.7672f;
    m_Characterselect.N0000005A.position.Z = 231.172f;

    m_Characterselect.N0000005A.rotation.X = 0.099325f;
    m_Characterselect.N0000005A.rotation.Y = 1.33341f;
    m_Characterselect.N0000005A.rotation.Z = 0;

    m_Characterselect.N0000005A.distance = 80.0f;
    m_Characterselect.N0000005A.time = 10.0f; // time is preset and ignored


    m_Characterselect.BoxFrame.position.X = 376.72f;
    m_Characterselect.BoxFrame.position.Y = 7.73251f;
    m_Characterselect.BoxFrame.position.Z = 204.31f;

    m_Characterselect.BoxFrame.rotation.X = 0.8f;
    m_Characterselect.BoxFrame.rotation.Y = 0.8088f;
    m_Characterselect.BoxFrame.rotation.Z = 0;

    m_Characterselect.BoxFrame.distance = 6.7f;
    m_Characterselect.BoxFrame.time = 15.0f; // time is preset and ignored

    m_Characterselect.KeyframeStart.position.X = 486.626f;
    m_Characterselect.KeyframeStart.position.Y = 10.3903f;
    m_Characterselect.KeyframeStart.position.Z = 224.418f;

    m_Characterselect.KeyframeStart.rotation.X = 0.099325f;
    m_Characterselect.KeyframeStart.rotation.Y = 0.752854f;
    m_Characterselect.KeyframeStart.rotation.Z = 0;

    m_Characterselect.KeyframeStart.distance = 80.0f;
    m_Characterselect.KeyframeStart.time = 0.f;

    m_Characterselect.KeyframeEnd.position.X = 502.171f;
    m_Characterselect.KeyframeEnd.position.Y = 8.12451f;
    m_Characterselect.KeyframeEnd.position.Z = 241.01f;

    m_Characterselect.KeyframeEnd.rotation.X = 0.099325f;
    m_Characterselect.KeyframeEnd.rotation.Y = 0.752854f;
    m_Characterselect.KeyframeEnd.rotation.Z = 0;

    m_Characterselect.KeyframeEnd.distance = 80.0f;
    m_Characterselect.KeyframeEnd.time = 5.0f;



    m_Characterselect.Char3D1.Position.X = 460.835f;
    m_Characterselect.Char3D1.Position.Y = 0.626249f;
    m_Characterselect.Char3D1.Position.Z = 233.131f;
    m_Characterselect.Char3D1.rotation = -0.95493f;

    m_Characterselect.Char3D2.Position.X = 490.574f;
    m_Characterselect.Char3D2.Position.Y = 0.985927f;
    m_Characterselect.Char3D2.Position.Z = 194.903f;
    m_Characterselect.Char3D2.rotation = -0.827606f;



    MEMUTIL_WRITE_VALUE(double, CHARSEL_CHARACTER_POSITION_YOFFSET, 0.0f);
    MEMUTIL_WRITE_VALUE(double, CHARSEL_CHARACTER_POSITION_YOFFSETSITDOWN, 0.0f);
    //MEMUTIL_WRITE_VALUE(double, LIZARD_X, -584);


    PatchMe(0x00D8AD3E, (BYTE) 0x70417C87);

    reinterpret_cast<CPSCharacterSelect *(__thiscall *) (CPSCharacterSelect *)>(0x00860820)(this);

}

bool CPSCharacterSelect::OnCreateIMPL(long ln) {
    const DWORD diagnosticsStartTick = GetTickCount();
    const SIZE_T diagnosticsWorkingSetBefore = IsMemoryDiagnosticsEnabled() ? GetCurrentWorkingSetBytes() : 0;

    if (m_Player)
    {
        m_Player->FirstSpawn = false;
        m_Player->PetSkillTimerRunning = false;
    }

    if (g_pCGInterface)
        g_pCGInterface->SuspendMacroAutomationForWorldTransition();

    bool b = reinterpret_cast<bool(__thiscall *)(CPSCharacterSelect *, long)>(0x0085eb10)(this, ln);
    if(m_Settings->OldLogin)
    {
        GETMAP()->Func_1(24508);
        D3DXVECTOR3 loc(160, 0, 190);

        //m_cameraworking->sub_4E5BF0(-947);
        GETMAP()->Func_0(24508, loc);

        m_box.SetResource("res\\interface\\box.bsr", 1);

        m_box.SetLocation(D3DXVECTOR3(155.0f, 7.735f, 652.0f));


        m_Characterselect.BoxFrame.position.X = 376.72f;
        m_Characterselect.BoxFrame.position.Y = 7.73251f;
        m_Characterselect.BoxFrame.position.Z = 204.31f;

        const D3DVECTOR box_loc[] = {
                {378.0f, 7.74f, 207.0f},
                {380.5f, 7.74f, 208.0f},
                {380.0f, 7.74f, 208.0f},
        };

        m_box.SetRotation(5.0);


        const float box_rot[] = {
                5.0,
                5.0,
                5.0};

        const char *box_resources[] = {
                "res\\interface\\interface_idol_europe.bsr",
                "res\\interface\\interface_idol_china.bsr",
                "res\\interface\\interface_lizard.bsr"};


        for (int i = 0; i < 3; i++) {
            idols[i].SetResource(box_resources[i], 1);
            idols[i].SetLocation(box_loc[i]);
            idols[i].SetRotation(box_rot[i]);
        }


        field_0x220 = 0;

        // Map animation callback stuff
        idols[2].m_compound->Func_7(*this);
        // Something with animation speed
        idols[2].Func_3(0, 0, 0, 0, 1.0, 1.0);
    }
    if(m_Settings->NewCharInfoScreen)
    {
        m_IRM.GetResObj(1925, 1)->m_FontTexture.sub_8B4750(5);

        wnd_rect a2 = m_IRM.GetResObj(GDR_STA_SCREENUP, 1)->GetBounds();

        wnd_rect v109 = m_IRM.GetResObj(GDR_STA_SCREENDOWN, 1)->GetBounds();

        const ClientResolutonData &res = CGame::GetClientDimensionStuff();

        int v12 = res.width - 209;

        int buttons_y = v109.top() + 27;

        // State: All chars visible
        m_IRM.GetResObj(GDR_BTN_CREATE, 1)->MoveGWnd(v12, buttons_y);
        m_IRM.GetResObj(GDR_BTN_BACK, 1)->MoveGWnd(v12 + 104, buttons_y);


        // State: One char selected
        m_IRM.GetResObj(GDR_BTN_START, 1)->MoveGWnd(v12 - 104, buttons_y);
        m_IRM.GetResObj(GDR_BTN_DELETE, 1)->MoveGWnd(v12, buttons_y);
        m_IRM.GetResObj(GDR_BTN_RESTORE, 1)->MoveGWnd(v12, buttons_y);
        m_IRM.GetResObj(GDR_BTN_BACK2, 1)->MoveGWnd(v12 + 104, buttons_y);
        //  m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->MoveGWnd(res.width - 271, a2.bottom() + 41);
        m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->MoveGWnd(43, a2.bottom() + 1);
        m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->SetGWndSize(283, 170);
        m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->TB_Func_13("clientlibrary\\charselect\\info.ddj", 1, 0);

        m_IRM.GetResObj(GDR_GAU_HP, 1)->ShowGWnd(false);
        m_IRM.GetResObj(GDR_GAU_MP, 1)->ShowGWnd(false);
        m_IRM.GetResObj(GDR_STA_NAME, 1)->SetGWndSize(207, 13);

        m_IRM.GetResObj(GDR_STATIC3, 1)->MoveGWnd(m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->GetPos().x + 30, a2.bottom() + 55);
        m_IRM.GetResObj(GDR_STATIC3, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
        m_IRM.GetResObj(GDR_STATIC3, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);


        m_IRM.GetResObj(GDR_STA_LEVEL, 1)->MoveGWnd(124, a2.bottom() + 55);
        m_IRM.GetResObj(GDR_STA_LEVEL, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_LEFT);
        m_IRM.GetResObj(GDR_STA_LEVEL, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);



        m_IRM.GetResObj(GDR_STATIC1, 1)->MoveGWnd(m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->GetPos().x + 32, a2.bottom() + 80);
        m_IRM.GetResObj(GDR_STATIC1, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
        m_IRM.GetResObj(GDR_STATIC1, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        m_IRM.GetResObj(GDR_STATIC1, 1)->m_FontTexture.sub_8B4750(0);
        m_IRM.GetResObj(GDR_STATIC1, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 239, 153));


        m_IRM.GetResObj(GDR_STA_EXP, 1)->MoveGWnd(124, a2.bottom() + 80);
        m_IRM.GetResObj(GDR_STA_EXP, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_LEFT);
        m_IRM.GetResObj(GDR_STA_EXP, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);

        m_IRM.GetResObj(GDR_STA_EXP, 1)->m_FontTexture.sub_8B4750(0);


        m_IRM.GetResObj(GDR_STATIC2, 1)->MoveGWnd(m_IRM.GetResObj(GDR_STA_CHARINFO, 1)->GetPos().x + 180, a2.bottom() + 55);
        m_IRM.GetResObj(GDR_STATIC2, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_LEFT);
        m_IRM.GetResObj(GDR_STATIC2, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        m_IRM.GetResObj(GDR_STATIC2, 1)->m_FontTexture.sub_8B4750(0);
        m_IRM.GetResObj(GDR_STATIC2, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 239, 153));


        m_IRM.GetResObj(GDR_STA_SP, 1)->MoveGWnd(261, a2.bottom() + 55);

        m_IRM.GetResObj(GDR_STA_SP, 1)->JustifyHorizontal(CTextBoard::JUSTIFY_LEFT);
        m_IRM.GetResObj(GDR_STA_SP, 1)->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        m_IRM.GetResObj(GDR_STA_SP, 1)->m_FontTexture.sub_8B4750(0);
    }

    wnd_pos x = this->m_IRM.GetResObj(500, 1)->GetPos();
    this->m_IRM.GetResObj(500, 1)->MoveGWnd(x.x-10,x.y-5);

    OfflineStall_CreateLoginPrompt(this);

    LogCharacterSelectMemory("create", GetTickCount() - diagnosticsStartTick,
                             diagnosticsWorkingSetBefore, m_characters.size());

    return b;
}

void CPSCharacterSelect::FUN_0085b400() {
    reinterpret_cast<void(__thiscall *)(CPSCharacterSelect *)>(0x0085b400)(this);
}


//HOOK_ORIGINAL_MEMBER(0x0085b1f0, &CPSCharacterSelect::TriggerAnimation_Intro);
void CPSCharacterSelect::TriggerAnimation_Intro() {
    //  reinterpret_cast<void(__thiscall *)(CPSCharacterSelect *)>(0x0085b1f0)(this);
    FUN_00854860(m_IRM.GetResObj<CIFStatic>(GDR_STA_TITLE, 1), 255, 0.5f, 0.0f, 0);
    m_IRM.GetResObj<CIFStatic>(GDR_STA_REGIONTITLE, 1)->ShowGWnd(false);

    idol_label[0]->ShowGWnd(false);
    idol_label[1]->ShowGWnd(false);

    CIFButton *createBtn = m_IRM.GetResObj<CIFButton>(GDR_BTN_CREATE, 1);
    FUN_00854860(createBtn, 255, 0.5f, 0.0f, 0);
    wnd_pos createBtnPos = createBtn->GetPos();
    createBtn->MoveGWnd2(createBtnPos);

    CIFButton *backBtn = m_IRM.GetResObj<CIFButton>(GDR_BTN_CREATE, 1);
    FUN_00854860(backBtn, 255, 0.5f, 0.0f, 0);
    wnd_pos backBtnPos = backBtn->GetPos();
    createBtn->MoveGWnd2(backBtnPos);

    m_IRM.GetResObj<CIFButton>(GDR_BTN_CANCEL, 1)->ShowGWnd(false);

    m_cameraworking->sub_4E6630();

    for (int i = 0; i < size(frames_intro); i++) {
        m_cameraworking->AddKeyframe(static_cast<float>(i * i),
                                     frames_intro[i].position,
                                     frames_intro[i].rotation);
    }

    theApp.camera.origin = m_cameraworking->location;
    theApp.camera.rotation_to_world = m_cameraworking->rotation;

    m_cameraworking->float_F0 = 2.0;
    current_state = 0;
}

GFX_MSGMAP* CPSCharacterSelect::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    /* {GFX_WM_COMMAND, 0, 14, 14, BSSig_u12, 0,
                             (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFSkillBoard::OnBtnClick))},
 */
                    {GFX_WM_COMMAND, 0, 1111, 1111, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CPSCharacterSelect::CreatePIN))},
                    // Diğer özel mesaj girişleri buraya eklenebilir
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00dd76b8), skillBoardMessageEntries,
            };
    return &newmap;
}

void CPSCharacterSelect::FUN_008560e0() {
    reinterpret_cast<void(__thiscall *)(CPSCharacterSelect *)>(0x008560e0)(this);
}

void CPSCharacterSelect::FUN_008548d0(bool a1) {
    reinterpret_cast<void(__thiscall *)(CPSCharacterSelect *, bool)>(0x008548d0)(this, a1);
}
void CPSCharacterSelect::FUN_00854860(CIFStatic *obj, unsigned char opacity, float time, float a4, char a5) {
    obj->sub_6526E0(obj->GetN00009BB9(), opacity, time, a4, a5);
}
#define DAT_00ec2d60 (*(void **) 0x00ec2d60)        /// charID ?
#define DAT_00ec2d64 (*(void **) 0x00ec2d64)        /// charID ?

#define keystate (*(wchar_t **) 0x00eedac0)/// charID ?
bool waitingForDoubleClick;
int lastClickTime;

namespace {
    bool PlayCharacterSelectAnimationSafe(void *animationObject, int animationId) {
        if(animationObject == 0)
            return false;

        return KillerAnimationPlayer::PlayOnAnimationObject(animationObject, animationId, 0, 750);
    }

    int GetRandomCharacterSelectAnimationId() {
        static bool seeded = false;
        if(!seeded) {
            srand((unsigned int)time(NULL));
            seeded = true;
        }

        static const int animations[] = {31, 32, 33, 54, 114, 115};
        const int count = sizeof(animations) / sizeof(animations[0]);
        return animations[rand() % count];
    }
}

unsigned int CPSCharacterSelect::OnDisappear(bool a1) {
    const DWORD diagnosticsStartTick = GetTickCount();
    const SIZE_T diagnosticsWorkingSetBefore = IsMemoryDiagnosticsEnabled() ? GetCurrentWorkingSetBytes() : 0;
    const size_t characterCount = m_characters.size();
    OfflineStall_ResetLoginPrompt(this);
    // Returning to the title screen to change accounts recreates its native
    // child controls.  Drop the prior title-scene bindings before that handoff.
    QuickLogin_ResetPanel();
    const unsigned int result = reinterpret_cast<unsigned int(__thiscall *)(CPSOuterInterface *, bool)>(0x00862410)(this, a1);
    LogCharacterSelectMemory("disappear", GetTickCount() - diagnosticsStartTick,
                             diagnosticsWorkingSetBefore, characterCount);
    return result;
}
void CPSCharacterSelect::ClickIdol()
{
    reinterpret_cast<void*(__thiscall *)(CPSCharacterSelect *)>(0x0085ca10)(this);

}
int CPSCharacterSelect::FUN_0085ddb0(Event3D* pEventData) {
    int result = reinterpret_cast<int(__thiscall *)(CPSCharacterSelect*, Event3D*)>(0x0085ddb0)(this, pEventData);
    this->SetClickable(true);
    return result;
}
void CPSCharacterSelect::FUN_008548b0()
{
    reinterpret_cast<void(__thiscall *)(CPSCharacterSelect*)>(0x0085b560)(this);
}
int CPSCharacterSelect::FUN_0085B680(BYTE a2) {


    int result = reinterpret_cast<int(__thiscall *)(CPSCharacterSelect *, BYTE)>(0x0085B680)(this, a2);
    TriggerSelectedCharacterAnimation();
    byte test = (byte) DAT_00ec2d60;
    if (test != 255) {
        CPSCharacterSelect_SCharacterInfo *p = m_characters.at(test);
        //printf("%p \n", p);
    }

    for (std::vector<CustomDataManager::CharInfoStruct>::iterator it = m_CustomDataManager->CharInfo.begin(); it != m_CustomDataManager->CharInfo.end(); ++it) {
        if ((*it).CharName16 == m_IRM.GetResObj<CIFStatic>(GDR_STA_NAME, 1)->GetNText()) {
            this->m_IRM.GetResObj(1925, 1)->SetText(it->RegionName.c_str());
        }
    }

    //
    return result;
}

wchar_t *CPSCharacterSelect::Convert(std::n_string name) {
    wchar_t *szName;
    const char *p = name.c_str();
    int nChars = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
    szName = new WCHAR[nChars];
    MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR) szName, nChars);
    return szName;
}
void CPSCharacterSelect::CreatePIN()
{
}

void CPSCharacterSelect::TriggerCharacterAnimation(CPSCharacterSelect_SCharacterInfo *pInfo) {
    if(pInfo == 0)
        return;

    const int animationId = GetRandomCharacterSelectAnimationId();
    bool played = false;

    played = PlayCharacterSelectAnimationSafe(pInfo->CCompChar, animationId);
    played = PlayCharacterSelectAnimationSafe(pInfo, animationId) || played;

}

void CPSCharacterSelect::TriggerSelectedCharacterAnimation() {
    static byte lastSelectedIndex = 255;
    static DWORD lastAnimationTick = 0;

    byte selectedIndex = (byte) DAT_00ec2d60;
    if(selectedIndex == 255)
        return;
    if(selectedIndex >= m_characters.size())
        return;

    DWORD now = GetTickCount();
    if(selectedIndex == lastSelectedIndex && now - lastAnimationTick < 900)
        return;

    lastSelectedIndex = selectedIndex;
    lastAnimationTick = now;
    TriggerCharacterAnimation(m_characters.at(selectedIndex));
}

void CPSCharacterSelect::TriggerLoadedCharactersAnimation() {
    TriggerSelectedCharacterAnimation();
}

void CPSCharacterSelect::TryEnterPendingQuickLoginCharacter() {
    std::n_string pendingCharacter;
    if (!QuickLogin_GetPendingCharacterName(pendingCharacter))
        return;

    std::n_wstring targetName = TO_NWSTRING(pendingCharacter);
    bool found = false;

    if (m_characters.size() == 0)
        return;

    for (size_t i = 0; i < m_characters.size(); ++i) {
        CPSCharacterSelect_SCharacterInfo* info = m_characters.at(i);
        if (info == NULL)
            continue;

        if (_wcsicmp(info->CharName.c_str(), targetName.c_str()) != 0)
            continue;

        found = true;
        bool pendingDelete = false;
        for (std::vector<CustomDataManager::CharInfoStruct>::iterator it = m_CustomDataManager->CharInfo.begin();
             it != m_CustomDataManager->CharInfo.end(); ++it) {
            if (_wcsicmp(it->CharName16.c_str(), info->CharName.c_str()) == 0) {
                pendingDelete = it->DeleteStatus != 0;
                break;
            }
        }

        if (pendingDelete) {
            ShowMessage(KmtGetText(L"UIIT_KMT_SAVED_QUICK_LOGIN_CHARACTER_IS_PENDING_DELETION"), 0xFFFF671D);
            QuickLogin_ClearPendingCharacterName();
            return;
        }

        CMsgStreamBuffer buf(0x7001);
        buf << TO_STRING(info->CharName);
        SendMsg(buf);
        QuickLogin_ClearPendingCharacterName();
        return;
    }

    if (!found) {
        ShowMessage(KmtGetText(L"UIIT_KMT_SAVED_QUICK_LOGIN_CHARACTER_WAS_NOT_FOUND_ON_THIS_ACCOUNT"), 0xFFFF671D);
        QuickLogin_ClearPendingCharacterName();
    }
}


bool CPSCharacterSelect::OnServerPacketRecv(CMsgStreamBuffer *msg) {
    const bool isCharacterListPacket = msg->msgid() == 0xB007;
    const bool isCustomCharacterInfoPacket = msg->msgid() == 0x1199;
    const DWORD diagnosticsStartTick = GetTickCount();
    const SIZE_T diagnosticsWorkingSetBefore =
            isCharacterListPacket && IsMemoryDiagnosticsEnabled() ? GetCurrentWorkingSetBytes() : 0;

    if (msg->msgid() == 0xB003)//notice
    {
        int Color;
        std::n_string notice;
        *msg >> notice >> Color;

        wchar_t* convertedNotice = Convert(notice);
        this->ShowMessage(convertedNotice, Color);
        delete[] convertedNotice;

        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0xA340)//notice
    {
        std::n_wstring notice;
        *msg >> notice;;

        this->ShowMessage(notice.c_str(), 0xFFFF671D);

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
            OfflineStall_OpenLoginPrompt(this, token, timeoutSeconds, characterName, GDR_BTN_START);
        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x1199) {
        m_CustomDataManager->CharInfo.clear();
        byte count;
        *msg >> count;
        m_CustomDataManager->CharInfo.reserve(count);
        int i = 0;
        while (i < count && count > 0) {
            i++;
            std::n_string CharName16;
            int RegionID;
            byte ID;
            *msg >> CharName16 >> RegionID >> ID;
            std::wstringstream region;
            region << RegionID;
            const std::n_wstring *nwRegionName = g_CTextStringManager->GetString2(region.str().c_str());

            std::n_wstring TitleNames = nwRegionName->c_str();
            if (TitleNames.empty())
                TitleNames = L"</>";

            CustomDataManager::CharInfoStruct Mgr = CustomDataManager::CharInfoStruct();
            Mgr.CharName16 = TO_NWSTRING(CharName16);
            Mgr.RegionName = TitleNames;
            Mgr.DeleteStatus = ID;
            m_CustomDataManager->CharInfo.push_back(Mgr);
        }

        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x1214)
    {
        *msg >> m_Player->MailAddress;
        msg->m_currentReadBytes = 0;
    }

    else if(msg->msgid() == 0x207E)
    {
        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;
            int RegionID;
            bool EventSuit;
            bool HideBuffViewer;
            bool DisablePetSpawn;
            bool DisableParty;
            bool AutoCape;
            bool HideMiniMap;
            byte RegType;
            *msg >> RegionID;
            *msg >> EventSuit;
            *msg >> HideBuffViewer;
            *msg >> DisablePetSpawn;
            *msg >> DisableParty;
            *msg >> AutoCape;
            *msg >> HideMiniMap;
            *msg >> RegType;
            CustomDataManager::SEventMapSettings str = CustomDataManager::SEventMapSettings();
            str.RegionID= RegionID;
            str.EventSuit= EventSuit;
            str.HideBuffViewer= HideBuffViewer;
            str.DisablePetSpawn= DisablePetSpawn;
            str.DisableParty= DisableParty;
            str.AutoCape= AutoCape;
            str.HideMiniMap = HideMiniMap;
            str.RegionType = RegType;
            m_CustomDataManager->m_EventMapSettings[RegionID] = str;
        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x204C)/// New Titles
    {
        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;

            std::n_string CharName167;
            *msg >> CharName167;

            std::n_string TitleName7;
            *msg >> TitleName7;

            std::n_wstring CharName16 = TO_NWSTRING(CharName167);
            std::n_wstring TitleName = TO_NWSTRING(TitleName7);

            m_CustomDataManager->_ActiveTags.insert(std::make_pair(CharName16,TitleName));
        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x204E)/// Title Colors
    {

        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;

            std::n_string CharName16;
            *msg >> CharName16;

            unsigned int Color;
            *msg >> Color;
            m_CustomDataManager->_ActiveNameColors.insert((std::make_pair(TO_NWSTRING(CharName16), Color)));
        }
        msg->m_currentReadBytes = 0;
    }
    else if (msg->msgid() == 0x204D)/// Title Colors
    {

        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;

            std::n_string CharName16;
            *msg >> CharName16;

            unsigned int Color;
            *msg >> Color;
            m_CustomDataManager->_ActiveTitleColors.insert((std::make_pair(TO_NWSTRING(CharName16), Color)));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x204F) /// media path
    {
        int Count;
        *msg >> Count;
        int i = 0;
     //   void* Resource = Fun_CacheTexture_Create("clientlibrary\\emoji\\laugh.ddj");
       // m_CustomDataManager->emojiListData.insert(std::make_pair(":D", Resource));
        while (i < Count && Count > 0) {
            i++;

            int IconID;
            *msg >> IconID;

            std::n_string IconPath;
            *msg >> IconPath;
            void* Resource = Fun_CacheTexture_Create(IconPath);
            if (Resource != NULL)
            {
                m_CustomDataManager->m_IconsData.insert(std::make_pair(IconID, Resource));
                m_CustomDataManager->MediaIcons.insert(std::make_pair(IconID, IconPath));
            }
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x206F) /// Lucky Spin rewards
    {
        int count;
        *msg >> count;
        m_CustomDataManager->LuckySpinRewards.clear();
        if (count > 0)
            m_CustomDataManager->LuckySpinRewards.reserve(count);

        for (int i = 0; i < count; ++i) {
            CustomDataManager::LuckySpinReward reward;
            *msg >> reward.ItemID;
            *msg >> reward.Amount;
            if (reward.ItemID > 0 && reward.Amount > 0) {
                m_CustomDataManager->LuckySpinRewards.push_back(reward);
            }
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x205A) /// Left Char Icon
    {

        int count;
        *msg >> count;
        int i = 0;
        while (i < count && count > 0) {
            i++;

            std::n_string Charname;
            *msg >> Charname;

            int IconID;
            *msg >> IconID;


            m_CustomDataManager->m_LeftCharIcons.insert(std::make_pair(TO_NWSTRING(Charname), IconID));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x205B) /// RightCharIcon
    {

        int count;
        *msg >> count;
        int i = 0;
        while (i < count && count > 0) {
            i++;

            std::n_string Charname;
            *msg >> Charname;

            int IconID;
            *msg >> IconID;


            m_CustomDataManager->m_RightCharIcons.insert(std::make_pair(TO_NWSTRING(Charname), IconID));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x205D) /// ach
    {

        int count;
        *msg >> count;
        int i = 0;
        while (i < count && count > 0) {
            i++;
            int ID;
            int RefAchievementID;
            byte State;
            byte ConditionType;
            __int64 ProgressCount;

            *msg >> ID >> RefAchievementID >> State >> ConditionType >> ProgressCount;


        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x205F)
    {

        byte Count;
        *msg >> Count;
        byte i = 0;
        while (i < Count && Count > 0) {
            i++;

            byte SlotType;
            int Data, SlotSeq;
            *msg >> SlotSeq >> SlotType >> Data;
            CustomCICPlayer::Macro_AutoSkillData Datas = CustomCICPlayer::Macro_AutoSkillData();
            Datas.SlotSeq = SlotSeq;
            Datas.SlotType = SlotType;
            Datas.Data = Data;
            m_Player->m_MacroAutoSkillData.insert((std::make_pair(SlotSeq, Datas)));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x206A)
    {

        byte Count;
        *msg >> Count;
        byte i = 0;
        while (i < Count && Count > 0) {
            i++;
            std::n_string NameStrIDx;
            *msg >> NameStrIDx;

            std::n_wstring NameStrID = TO_NWSTRING(NameStrIDx);


            int SkillID_1, SkillID_2, SkillID_3, SkillID_4,SkillID_5;


            byte Active_Level_1, Active_Level_2, Active_Level_3, Active_Level_4, Active_Level_5, SkillType_1, SkillType_2, SkillType_3,SkillType_4,SkillType_5;

            int SelfSkill_1, SelfSkill_2;

            byte SelfSkill_Active_Level_1, SelfSkill_Active_Level_2;

            *msg >> SkillID_1;
            *msg >> Active_Level_1;
            *msg >> SkillType_1;
            *msg >> SkillID_2;
            *msg >> Active_Level_2;
            *msg >> SkillType_2;
            *msg >> SkillID_3;
            *msg >> Active_Level_3;
            *msg >> SkillType_3;
            *msg >> SkillID_4;
            *msg >> Active_Level_4;
            *msg >> SkillType_4;
            *msg >> SkillID_5;
            *msg >> Active_Level_5;
            *msg >> SkillType_5;
            *msg >> SelfSkill_1;
            *msg >> SelfSkill_Active_Level_1;
            *msg >> SelfSkill_2;
            *msg >> SelfSkill_Active_Level_2;

            CustomDataManager::FellowPetStruct Data = CustomDataManager::FellowPetStruct();
            Data.NameStrID = NameStrID;
            Data.SkillID_1 = SkillID_1;
            Data.SkillID_2 = SkillID_2;
            Data.SkillID_3 = SkillID_3;
            Data.SkillID_4 = SkillID_4;
            Data.SkillID_5 = SkillID_5;

            Data.Active_Level_1 = Active_Level_1;
            Data.Active_Level_2 = Active_Level_2;
            Data.Active_Level_3 = Active_Level_3;
            Data.Active_Level_4 = Active_Level_4;
            Data.Active_Level_5 = Active_Level_5;

            Data.SkillType_1 = SkillType_1;
            Data.SkillType_2 = SkillType_2;
            Data.SkillType_3 = SkillType_3;
            Data.SkillType_4 = SkillType_4;
            Data.SkillType_5 = SkillType_5;
            Data.SelfSkill_1 = SelfSkill_1;
            Data.SelfSkill_2 = SelfSkill_2;
            Data.SelfSkill_Active_Level_1 = SelfSkill_Active_Level_1;
            Data.SelfSkill_Active_Level_2 = SelfSkill_Active_Level_2;

            m_CustomDataManager->m_RefFellowPetSystem.insert((std::make_pair(NameStrID, Data)));
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x205C)
    {
        int Count;
        *msg >> Count;
        m_CustomDataManager->m_RefAchievement.clear();
        if (Count < 0 || Count > 2048) {
            msg->m_currentReadBytes = 0;
            return 0;
        }
        int i = 0;
        while (i < Count) {
            i++;
            int ID;
            byte Category;
            std::n_string Name;
            byte RewardType;
            std::n_string RewardTagName;
            int RewardSkillPoint;
            __int64 RewardGold;
            *msg >> ID >> Category >> Name >> RewardType >> RewardTagName >> RewardSkillPoint >> RewardGold;

            CustomDataManager::Achievements Data = CustomDataManager::Achievements();
            Data.ID = ID;
            Data.Category = Category;
            Data.Name = TO_WSTRING(Name);
            Data.RewardType = RewardType;
            Data.RewardTagName = TO_WSTRING(RewardTagName);
            Data.RewardSkillPoint = RewardSkillPoint;
            Data.RewardGold = RewardGold;
            m_CustomDataManager->m_RefAchievement[ID] = Data;
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x175A)
    {
        int Count;
        *msg >> Count;
        m_CustomDataManager->m_RefAchievementCondition.clear();
        if (Count > 0)
            m_CustomDataManager->m_RefAchievementCondition.reserve(Count);
        if (Count < 0 || Count > 4096) {
            msg->m_currentReadBytes = 0;
            return 0;
        }
        int i = 0;
        while (i < Count) {
            i++;
            int ID;
            std::n_string Name;
            int RefAchievementID;
            __int64 CompleteCount;
            byte Type;
            *msg >> ID >> Name >> RefAchievementID >> CompleteCount >> Type;

            CustomDataManager::SRefAchievementCondition Data = CustomDataManager::SRefAchievementCondition();
            Data.ID = ID;
            Data.Name = TO_WSTRING(Name);
            Data.RefAchievementID = RefAchievementID;
            Data.CompleteCount = CompleteCount;
            Data.Type = Type;
            m_CustomDataManager->m_RefAchievementCondition.push_back(Data);
        }
        msg->m_currentReadBytes = 0;
    }
    else if(msg->msgid() == 0x209E)
    {
        int Count;
        *msg >> Count;
        int i = 0;
        while (i < Count && Count > 0) {
            i++;

            int SkillID;
            *msg >> SkillID;
            bool JobMode;
            bool MapSettings;
            *msg >> JobMode >> MapSettings;
            CustomDataManager::SHideEffect data  = CustomDataManager::SHideEffect();
            data.SkillID = SkillID;
            data.JobMode = JobMode;
            data.MapSettings = MapSettings;
            m_CustomDataManager->HideEffects.insert((std::make_pair(SkillID, data)));
        }
        msg->m_currentReadBytes = 0;
    }

    int result = reinterpret_cast<int(__thiscall *)(CPSCharacterSelect *, CMsgStreamBuffer *)>(0x0085fc60)(this, msg);
    if(isCharacterListPacket) {
        TriggerLoadedCharactersAnimation();
        TryEnterPendingQuickLoginCharacter();
        LogCharacterSelectMemory("character_list", GetTickCount() - diagnosticsStartTick,
                                 diagnosticsWorkingSetBefore, m_characters.size());
    }
    else if(isCustomCharacterInfoPacket) {
        TryEnterPendingQuickLoginCharacter();
    }

    return result;
}
