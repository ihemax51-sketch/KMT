#include "IFMenu.h"
#include "IFGrantName.h"
#include "IFTitleManager.h"
#include "IFIconManager.h"
#include "IFDynamicRanking.h"
#include "IFUniqueHistory.h"
#include "IFEventRegister.h"
#include "IFEventSchedule.h"
#include "IFChangelog.h"
#include "IFAchievements.h"
#include <ctime>
#include <BSLib/Debug.h>
#include <Game.h>
#include <ICPlayer.h>
#include <GInterface.h>
#include <CustomData/CustomSettingManager.h>
#include <MacroAlchemy/IFAlchemyMacro.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include <CharacterDependentData.h>
#include <ExtraUI/IFSettings.h>
#include <Web/IFWeb.h>
#include <IFStatic.h>
#include <IFButton.h>


#define GDR_MENU_BTN_GRANTNAME 12
#define GDR_MENU_BTN_TITLEMGR 13
#define GDR_MENU_BTN_ICONMGR 14
#define GDR_MENU_BTN_DYNAMICRANKING 15
#define GDR_MENU_BTN_UNIQUEHISTORY 16
#define GDR_MENU_BTN_EVENT_REGISTER 17
#define GDR_MENU_BTN_EVENT_SCHEDULE 18
#define GDR_MENU_BTN_ACHIEVEMENTS 19
#define GDR_MENU_BTN_CHANGELOG 20
#define GDR_MENU_BTN_ALCHEMY_MACRO 21
#define GDR_DISCORD 24
#define GDR_WEBSITE 25
#define GDR_SETTINGS 22
#define GDR_LUCKY_SPIN 23
#define GDR_PLAY_TIME 24

namespace {

const int ID_MENU_BG = 500;

const char* MENU_BG_TEXTURE = "interface\\mall\\mall_pre_characterview.ddj";
const char* MENU_BUTTON_TEXTURE = "juicer\\menu\\button_title.ddj";
const char* MENU_BUTTON_PRESSED_TEXTURE = "juicer\\menu\\button_title_press.ddj";
const char* MENU_BUTTON_DISABLED_TEXTURE = "juicer\\menu\\button_title_focus.ddj";

struct MenuButtonLayout {
    int id;
    int x;
    int y;
    int w;
    int h;
    const wchar_t* text;
};

struct MenuButtonSkin {
    int id;
    const char* normal;
    const char* pressed;
    const char* disabled;
};

const int MENU_WIDTH = 243;
const int MENU_MAX_HEIGHT = 625;
const int MENU_MIN_HEIGHT = 130;
const int MENU_BOTTOM_PADDING = 19;
const int MENU_BUTTON_X = 2;
const int MENU_BUTTON_TOP = 105;
const int MENU_BUTTON_WIDTH = 238;
const int MENU_BUTTON_HEIGHT = 40;
const int MENU_BUTTON_STEP = 46;
const int MENU_CLOSE_X = MENU_WIDTH - 20;
const int MENU_CLOSE_Y = 3;
const int MENU_CLOSE_SIZE = 16;
const int MAXI_MENU_WIDTH = 208;
const int MAXI_CLOSE_X = MAXI_MENU_WIDTH - 26;
const int MAXI_CLOSE_Y = 9;
const int CASY_MENU_WIDTH = 292;
const int CASY_MENU_HEIGHT = 460;
const int CASY_BUTTON_X = 99;
const int CASY_BUTTON_WIDTH = 163;
const int CASY_BUTTON_HEIGHT = 31;
const int CASY_BUTTON_TOP = 118;
const int CASY_BUTTON_STEP = 39;
const int CASY_CLOSE_X = 258;
const int CASY_CLOSE_Y = 54;
const int CASY_CLOSE_SIZE = 16;
const int ID_CASY_FRAME = 501;
const int ID_CASY_HEADER = 502;

struct CasyButtonLayout {
    int id;
    const char* asset;
    const wchar_t* text;
    bool enabled;
};

int GetReferenceButtonY(int row)
{
    // The source layout uses 105, 152, 198, 244, 290, and 336 for its six
    // button rows. Preserve those exact values before extending the list for
    // this project's additional menu actions.
    return row == 0 ? MENU_BUTTON_TOP : MENU_BUTTON_TOP + 47 + ((row - 1) * MENU_BUTTON_STEP);
}

const MenuButtonSkin* GetButtonSkin(int id)
{
    static const MenuButtonSkin skins[] = {
        { GDR_MENU_BTN_GRANTNAME, "juicer\\menu\\button_title.ddj", "juicer\\menu\\button_title_press.ddj", "juicer\\menu\\button_title_focus.ddj" },
        { GDR_MENU_BTN_TITLEMGR, "juicer\\menu\\button_title_storage.ddj", "juicer\\menu\\button_title_storage_press.ddj", "juicer\\menu\\button_title_storage_focus.ddj" },
        { GDR_MENU_BTN_ICONMGR, "juicer\\menu\\button_title.ddj", "juicer\\menu\\button_title_press.ddj", "juicer\\menu\\button_title_focus.ddj" },
        { GDR_MENU_BTN_DYNAMICRANKING, "juicer\\menu\\button_ranking.ddj", "juicer\\menu\\button_ranking_press.ddj", "juicer\\menu\\button_ranking_focus.ddj" },
        { GDR_MENU_BTN_UNIQUEHISTORY, "juicer\\menu\\button_unique.ddj", "juicer\\menu\\button_unique_press.ddj", "juicer\\menu\\button_unique_focus.ddj" },
        { GDR_MENU_BTN_EVENT_REGISTER, "juicer\\menu\\button_schedule.ddj", "juicer\\menu\\button_schedule_press.ddj", "juicer\\menu\\button_schedule_focus.ddj" },
        { GDR_MENU_BTN_EVENT_SCHEDULE, "juicer\\menu\\button_schedule.ddj", "juicer\\menu\\button_schedule_press.ddj", "juicer\\menu\\button_schedule_focus.ddj" },
        { GDR_MENU_BTN_ACHIEVEMENTS, "juicer\\menu\\button_daily.ddj", "juicer\\menu\\button_daily_press.ddj", "juicer\\menu\\button_daily_focus.ddj" },
        { GDR_MENU_BTN_CHANGELOG, "juicer\\menu\\button_daily.ddj", "juicer\\menu\\button_daily_press.ddj", "juicer\\menu\\button_daily_focus.ddj" },
        { GDR_MENU_BTN_ALCHEMY_MACRO, "juicer\\menu\\button_title.ddj", "juicer\\menu\\button_title_press.ddj", "juicer\\menu\\button_title_focus.ddj" },
        { GDR_SETTINGS, "juicer\\menu\\button_title.ddj", "juicer\\menu\\button_title_press.ddj", "juicer\\menu\\button_title_focus.ddj" }
    };

    for (int i = 0; i < sizeof(skins) / sizeof(skins[0]); ++i) {
        if (skins[i].id == id)
            return &skins[i];
    }

    return NULL;
}

void ConfigureStaticText(CIFStatic* control, const wchar_t* text, DWORD color, CTextBoard::eJustifyHorizontal hAlign)
{
    if (control == NULL)
        return;

    control->SetText(text);
    control->SetFont(theApp.GetFont(0));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(hAlign);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->SetClickable(false);
    control->ShowGWnd(true);
    control->BringToFront();
}

CIFStatic* CreateStatic(CIFMenu* menu, int id, int x, int y, int w, int h, const wchar_t* text, DWORD color, CTextBoard::eJustifyHorizontal hAlign)
{
    RECT rect = {x, y, w, h};
    CIFStatic* control = (CIFStatic*)CGWnd::CreateInstance(menu, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
    ConfigureStaticText(control, text, color, hAlign);
    return control;
}

void HideResource(CIFMenu* menu, int id)
{
    CIFWnd* control = menu->GetMenuResource(id);
    if (control != NULL)
        control->ShowGWnd(false);
}

CIFStatic* CreateImage(CIFMenu* menu, int id, int x, int y, int w, int h, const char* texture)
{
    RECT rect = {x, y, w, h};
    CIFStatic* control = (CIFStatic*)CGWnd::CreateInstance(menu, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
    if (control != NULL) {
        control->TB_Func_13(texture, 0, 0);
        control->SetClickable(false);
        control->ShowGWnd(true);
    }
    return control;
}

void ConfigureMenuButton(CIFMenu* menu, const MenuButtonLayout& layout)
{
    HideResource(menu, layout.id);

    RECT rect = {layout.x, layout.y, layout.w, layout.h};
    CIFButton* button = (CIFButton*)CGWnd::CreateInstance(menu, GFX_RUNTIME_CLASS(CIFButton), rect, layout.id, 0);
    if (button == NULL)
        return;

    const MenuButtonSkin* skin = GetButtonSkin(layout.id);
    button->TB_Func_13(skin != NULL ? skin->normal : MENU_BUTTON_TEXTURE, 1, 1);
    button->FUN_00656590(std::n_string(skin != NULL ? skin->pressed : MENU_BUTTON_PRESSED_TEXTURE));
    button->FUN_00656640(std::n_string(skin != NULL ? skin->disabled : MENU_BUTTON_DISABLED_TEXTURE));
    button->SetText(layout.text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(0xFFFFA500);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->SetEnabledState(true);
    button->ShowGWnd(true);
    button->BringToFront();
}

void AddGridButton(CIFMenu* menu, int& row, int& column, bool enabled, int id, const wchar_t* text)
{
    if (!enabled)
        return;

    MenuButtonLayout layout = {
        id,
        MENU_BUTTON_X,
        GetReferenceButtonY(row),
        MENU_BUTTON_WIDTH,
        MENU_BUTTON_HEIGHT,
        text
    };

    ConfigureMenuButton(menu, layout);

    column = 0;
    ++row;
}

bool IsSettingReadyOrEnabled(bool enabled);

struct SystemMenuButton
{
    bool enabled;
    int id;
    const wchar_t* text;
};

void AddSystemButtons(CIFMenu* menu, int& row)
{
    SystemMenuButton buttons[3] = {
        { IsSettingReadyOrEnabled(m_Settings->EnableChangeLog), GDR_MENU_BTN_CHANGELOG, KmtGetText(L"UIIT_KMT_CHANGELOG") },
        { true, GDR_MENU_BTN_ALCHEMY_MACRO, KmtGetText(L"UIIT_KMT_ALCHEMY_MACRO") },
        { true, GDR_SETTINGS, KmtGetText(L"UIIT_KMT_SETTINGS") }
    };

    int visibleCount = 0;
    for (int i = 0; i < 3; ++i) {
        if (buttons[i].enabled)
            ++visibleCount;
    }

    if (visibleCount == 0)
        return;

    for (int i = 0; i < 3; ++i) {
        if (!buttons[i].enabled)
            continue;

        MenuButtonLayout layout = {
            buttons[i].id,
            MENU_BUTTON_X,
            GetReferenceButtonY(row),
            MENU_BUTTON_WIDTH,
            MENU_BUTTON_HEIGHT,
            buttons[i].text
        };

        ConfigureMenuButton(menu, layout);
        ++row;
    }
}

bool IsSettingReadyOrEnabled(bool enabled)
{
    if (m_Settings == NULL || !m_Settings->PSTitleIsLoaded)
        return true;

    return enabled;
}

bool IsMaxiMenuEnabled()
{
    return m_Settings != NULL && m_Settings->MenuLikeMaxi;
}

bool IsCasyMenuEnabled()
{
    return m_Settings != NULL && m_Settings->MenuCasy;
}

bool IsSettingsSnapshotReady()
{
    return m_Settings != NULL && m_Settings->PSTitleIsLoaded;
}

void ConfigureLegacyButton(CIFMenu* menu, int id, const wchar_t* text, bool enabled)
{
    CIFWnd* resource = menu->GetMenuResource(id);
    if (resource == NULL)
        return;

    CIFButton* button = static_cast<CIFButton*>(resource);
    button->SetText(text);
    button->SetEnabledState(IsSettingReadyOrEnabled(enabled));
    button->ShowGWnd(true);
}

void PositionMaxiCloseButton(CIFMenu* menu)
{
    if (menu == NULL || menu->m_pCloseBtn == NULL)
        return;

    menu->m_pCloseBtn->MoveGWnd(menu->GetPos().x + MAXI_CLOSE_X,
                                menu->GetPos().y + MAXI_CLOSE_Y);
    menu->m_pCloseBtn->BringToFront();
}

void ConfigureMaxiMenu(CIFMenu* menu)
{
    menu->SetText(KmtGetText(L"UIIT_KMT_MENU"));

    CIFWnd* frame = menu->GetMenuResource(2);
    if (frame != NULL)
        frame->ShowGWnd(true);
    CIFWnd* background = menu->GetMenuResource(1);
    if (background != NULL)
        background->ShowGWnd(true);

    ConfigureLegacyButton(menu, GDR_MENU_BTN_GRANTNAME, KmtGetText(L"UIIT_KMT_GRANT_NAME"),
                          m_Settings == NULL || m_Settings->GrantName != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_TITLEMGR, KmtGetText(L"UIIT_KMT_TITLE_MANAGER"),
                          m_Settings == NULL || m_Settings->TitleManager != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_ICONMGR, KmtGetText(L"UIIT_KMT_ICON_MANAGER"),
                          m_Settings == NULL || m_Settings->IconManager != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_DYNAMICRANKING, KmtGetText(L"UIIT_KMT_CHARACTER_RANKING"),
                          m_Settings == NULL || m_Settings->RankingWnd != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_UNIQUEHISTORY, KmtGetText(L"UIIT_KMT_UNIQUE_LOGS"),
                          m_Settings == NULL || m_Settings->UniqueHistoryWnd != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_EVENT_REGISTER, KmtGetText(L"UIIT_KMT_EVENT_REGISTER"),
                          m_Settings == NULL || m_Settings->EventRegisterWnd != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_EVENT_SCHEDULE, KmtGetText(L"UIIT_KMT_EVENT_SCHEDULING"),
                          m_Settings == NULL || m_Settings->EventScheduleWnd != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_ACHIEVEMENTS, KmtGetText(L"UIIT_KMT_ACHIEVEMENTS"),
                          m_Settings == NULL || m_Settings->AchievementsWnd != 0);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_CHANGELOG, KmtGetText(L"UIIT_KMT_CHANGELOG"),
                          m_Settings == NULL || m_Settings->EnableChangeLog);
    ConfigureLegacyButton(menu, GDR_MENU_BTN_ALCHEMY_MACRO, KmtGetText(L"UIIT_KMT_ALCHEMY_MACRO"), true);
    ConfigureLegacyButton(menu, GDR_SETTINGS, KmtGetText(L"UIIT_KMT_SETTINGS"), true);

    // The imported Clean menu also contains Lucky Spin and Play Time rows.
    // Those actions are not owned by this window in KMTGuard, so do not leave
    // two empty resource buttons visible.
    HideResource(menu, GDR_LUCKY_SPIN);
    HideResource(menu, GDR_PLAY_TIME);

    // Keep the imported resinfo layout, trimmed after the Settings row.
    menu->SetGWndSize(MAXI_MENU_WIDTH, 430);
    if (frame != NULL)
        frame->SetGWndSize(174, 377);
    if (background != NULL)
        background->SetGWndSize(163, 367);
    PositionMaxiCloseButton(menu);
}

void PositionMenuCloseButton(CIFMenu* menu)
{
    if (menu == NULL || menu->m_pCloseBtn == NULL)
        return;

    menu->m_pCloseBtn->MoveGWnd(menu->GetPos().x + MENU_CLOSE_X,
                                menu->GetPos().y + MENU_CLOSE_Y);
    menu->m_pCloseBtn->BringToFront();
}

void PositionCasyCloseButton(CIFMenu* menu)
{
    if (menu == NULL || menu->m_pCloseBtn == NULL)
        return;

    menu->m_pCloseBtn->MoveGWnd(menu->GetPos().x + CASY_CLOSE_X,
                                menu->GetPos().y + CASY_CLOSE_Y);
    menu->m_pCloseBtn->BringToFront();
}

void ConfigureCasyButton(CIFMenu* menu, const CasyButtonLayout& layout, int y)
{
    // Preserve the legacy feature gate: disabled features do not expose a
    // clickable menu action. Their disabled media still exists so every
    // native button state remains a complete asset family.
    if (!layout.enabled)
        return;

    RECT rect = { CASY_BUTTON_X, y, CASY_BUTTON_WIDTH, CASY_BUTTON_HEIGHT };
    CIFButton* button = (CIFButton*)CGWnd::CreateInstance(menu, GFX_RUNTIME_CLASS(CIFButton), rect, layout.id, 0);
    if (button == NULL)
        return;

    char normalTexture[128] = {0};
    char pressedTexture[128] = {0};
    char disabledTexture[128] = {0};
    sprintf(normalTexture, "clientlibrary\\menu_casy\\%s.ddj", layout.asset);
    sprintf(pressedTexture, "clientlibrary\\menu_casy\\%s_press.ddj", layout.asset);
    sprintf(disabledTexture, "clientlibrary\\menu_casy\\%s_disabled.ddj", layout.asset);

    // The native texture loader derives the _focus companion from the normal
    // surface. Press and disabled states are assigned explicitly.
    button->TB_Func_13(normalTexture, 1, 1);
    button->FUN_00656590(std::n_string(pressedTexture));
    button->FUN_00656640(std::n_string(disabledTexture));
    button->SetText(L"");
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(0xFFFFE7A0);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->SetEnabledState(true);
    button->ShowGWnd(true);
    button->BringToFront();
}

void ConfigureCasySideButton(CIFMenu* menu, int id, int x, int y, int width, int height, const char* asset, bool enabled)
{
    if (!enabled)
        return;

    RECT rect = { x, y, width, height };
    CIFButton* button = (CIFButton*)CGWnd::CreateInstance(menu, GFX_RUNTIME_CLASS(CIFButton), rect, id, 0);
    if (button == NULL)
        return;

    char normalTexture[128] = {0};
    char pressedTexture[128] = {0};
    char disabledTexture[128] = {0};
    sprintf(normalTexture, "clientlibrary\\menu_casy\\%s.ddj", asset);
    sprintf(pressedTexture, "clientlibrary\\menu_casy\\%s_press.ddj", asset);
    sprintf(disabledTexture, "clientlibrary\\menu_casy\\%s_disabled.ddj", asset);
    button->TB_Func_13(normalTexture, 1, 1);
    button->FUN_00656590(std::n_string(pressedTexture));
    button->FUN_00656640(std::n_string(disabledTexture));
    button->SetText(L"");
    button->SetEnabledState(true);
    button->ShowGWnd(true);
    button->BringToFront();
}

void ConfigureCasyMenu(CIFMenu* menu)
{
    for (int id = 1; id <= 25; ++id)
        HideResource(menu, id);

    if (menu->m_pTitleText != NULL)
        menu->m_pTitleText->ShowGWnd(false);

    menu->SetText(L"");
    menu->SetGWndSize(CASY_MENU_WIDTH, CASY_MENU_HEIGHT);

    // CIFMenu inherits CIFFrame, whose texture setter expects an eight-piece
    // frame prefix rather than a single DDJ. Clear that native frame first,
    // then render the CASY artwork as one transparent child behind controls.
    CreateImage(menu, ID_CASY_FRAME, 0, 0, CASY_MENU_WIDTH, CASY_MENU_HEIGHT,
                "clientlibrary\\menu_casy\\casy_frame.ddj");
    CasyButtonLayout buttons[] = {
        { GDR_MENU_BTN_GRANTNAME, "casy_grant_name", KmtGetText(L"UIIT_KMT_GRANT_NAME"), IsSettingReadyOrEnabled(m_Settings->GrantName != 0) },
        { GDR_MENU_BTN_TITLEMGR, "casy_title_manager", KmtGetText(L"UIIT_KMT_TITLE_MANAGER"), IsSettingReadyOrEnabled(m_Settings->TitleManager != 0) },
        { GDR_MENU_BTN_DYNAMICRANKING, "casy_rankings", KmtGetText(L"UIIT_KMT_CHARACTER_RANKING"), IsSettingReadyOrEnabled(m_Settings->RankingWnd != 0) },
        { GDR_MENU_BTN_UNIQUEHISTORY, "casy_unique_history", KmtGetText(L"UIIT_KMT_UNIQUE_LOGS"), IsSettingReadyOrEnabled(m_Settings->UniqueHistoryWnd != 0) },
        { GDR_MENU_BTN_EVENT_SCHEDULE, "casy_event_schedule", KmtGetText(L"UIIT_KMT_EVENT_SCHEDULING"), IsSettingReadyOrEnabled(m_Settings->EventScheduleWnd != 0) },
        { GDR_MENU_BTN_ACHIEVEMENTS, "casy_achievements", KmtGetText(L"UIIT_KMT_ACHIEVEMENTS"), IsSettingReadyOrEnabled(m_Settings->AchievementsWnd != 0) }
    };

    int visibleRow = 0;
    for (int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        if (!buttons[i].enabled)
            continue;
        ConfigureCasyButton(menu, buttons[i], CASY_BUTTON_TOP + (visibleRow * CASY_BUTTON_STEP));
        ++visibleRow;
    }

    // Keep the utility group centered directly beneath the final visible row.
    const int utilityY = CASY_BUTTON_TOP + (visibleRow * CASY_BUTTON_STEP) + 6;
    ConfigureCasySideButton(menu, GDR_MENU_BTN_ICONMGR, 118, utilityY, 36, 36, "casy_utility_icon_manager", IsSettingReadyOrEnabled(m_Settings->IconManager != 0));
    ConfigureCasySideButton(menu, GDR_MENU_BTN_CHANGELOG, 162, utilityY, 36, 36, "casy_utility_changelog", IsSettingReadyOrEnabled(m_Settings->EnableChangeLog));
    ConfigureCasySideButton(menu, GDR_SETTINGS, 206, utilityY, 36, 36, "casy_utility_settings", true);

    if (menu->m_pCloseBtn != NULL) {
        menu->m_pCloseBtn->ShowGWnd(true);
        // Keep the close control small and integrated with the upper frame.
        menu->m_pCloseBtn->TB_Func_13("clientlibrary\\menu_casy\\casy_close.ddj", 1, 1);
        menu->m_pCloseBtn->FUN_00656590(std::n_string("clientlibrary\\menu_casy\\casy_close_press.ddj"));
        menu->m_pCloseBtn->FUN_00656640(std::n_string("clientlibrary\\menu_casy\\casy_close_disabled.ddj"));
        menu->m_pCloseBtn->SetGWndSize(CASY_CLOSE_SIZE, CASY_CLOSE_SIZE);
        PositionCasyCloseButton(menu);
    }
}

int GetMenuHeightForRows(int row)
{
    if (row <= 0)
        return MENU_MIN_HEIGHT;

    int height = GetReferenceButtonY(row - 1) + MENU_BUTTON_HEIGHT + MENU_BOTTOM_PADDING;
    if (height < MENU_MIN_HEIGHT)
        height = MENU_MIN_HEIGHT;
    if (height > MENU_MAX_HEIGHT)
        height = MENU_MAX_HEIGHT;
    return height;
}

void ConfigureMenuChrome(CIFMenu* menu)
{
    if (menu->m_pTitleText != NULL)
        menu->m_pTitleText->ShowGWnd(false);

    HideResource(menu, 2);

    if (menu->m_pCloseBtn != NULL) {
        menu->m_pCloseBtn->ShowGWnd(true);
        menu->m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
        menu->m_pCloseBtn->FUN_00656590(std::n_string("clientlibrary\\mall\\mall_web_close_button_press.ddj"));
        menu->m_pCloseBtn->FUN_00656640(std::n_string("clientlibrary\\mall\\mall_web_close_button_focus.ddj"));
        menu->m_pCloseBtn->SetGWndSize(MENU_CLOSE_SIZE, MENU_CLOSE_SIZE);
        PositionMenuCloseButton(menu);
    }
}

void BuildOriginalMenu(CIFMenu* menu)
{
    for (int id = 4; id <= 25; ++id)
        HideResource(menu, id);

    menu->SetGWndSize(MENU_WIDTH, MENU_MAX_HEIGHT);
    menu->TB_Func_13(MENU_BG_TEXTURE, 0, 0);
    menu->SetText(L"");

    CIFStatic* background = CreateImage(menu, ID_MENU_BG, 0, 0, MENU_WIDTH, MENU_MAX_HEIGHT, MENU_BG_TEXTURE);

    int row = 0;
    int column = 0;
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->GrantName != 0), GDR_MENU_BTN_GRANTNAME, KmtGetText(L"UIIT_KMT_GRANT_NAME"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->TitleManager != 0), GDR_MENU_BTN_TITLEMGR, KmtGetText(L"UIIT_KMT_TITLE_MANAGER"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->IconManager != 0), GDR_MENU_BTN_ICONMGR, KmtGetText(L"UIIT_KMT_ICON_MANAGER"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->RankingWnd != 0), GDR_MENU_BTN_DYNAMICRANKING, KmtGetText(L"UIIT_KMT_CHARACTER_RANKING"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->UniqueHistoryWnd != 0), GDR_MENU_BTN_UNIQUEHISTORY, KmtGetText(L"UIIT_KMT_UNIQUE_LOGS"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->EventScheduleWnd != 0), GDR_MENU_BTN_EVENT_SCHEDULE, KmtGetText(L"UIIT_KMT_EVENT_SCHEDULING"));
    AddGridButton(menu, row, column, IsSettingReadyOrEnabled(m_Settings->AchievementsWnd != 0), GDR_MENU_BTN_ACHIEVEMENTS, KmtGetText(L"UIIT_KMT_ACHIEVEMENTS"));

    AddSystemButtons(menu, row);

    const int menuHeight = GetMenuHeightForRows(row);
    menu->SetGWndSize(MENU_WIDTH, menuHeight);
    if (background != NULL)
        background->SetGWndSize(MENU_WIDTH, menuHeight);

    ConfigureMenuChrome(menu);
}

}

GFX_IMPLEMENT_DYNCREATE(CIFMenu, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFMenu, CIFMainFrame)
                    ONG_COMMAND(GDR_MENU_BTN_GRANTNAME, &On_BtnClickGrantName)
                    ONG_COMMAND(GDR_MENU_BTN_TITLEMGR, &On_BtnClickTitleManager)
                    ONG_COMMAND(GDR_MENU_BTN_ICONMGR, &On_BtnClickIconManager)
                    ONG_COMMAND(GDR_MENU_BTN_DYNAMICRANKING, &On_BtnClickRank)
                    ONG_COMMAND(GDR_MENU_BTN_UNIQUEHISTORY, &On_BtnClickUniqueLog)
                    ONG_COMMAND(GDR_MENU_BTN_EVENT_REGISTER, &On_BtnClickEventRegister)
                    ONG_COMMAND(GDR_MENU_BTN_EVENT_SCHEDULE, &On_BtnClickEventTimer)
                    ONG_COMMAND(GDR_MENU_BTN_ACHIEVEMENTS, &On_BtnClickAchievements)
                    ONG_COMMAND(GDR_MENU_BTN_CHANGELOG, &On_BtnClickChangelog)
                    ONG_COMMAND(GDR_MENU_BTN_ALCHEMY_MACRO, &On_BtnClickAlchemyMacro)
                    ONG_COMMAND(GDR_SETTINGS, &On_BtnSettings)
	ONG_CREATE()
	ONG_WM_4002()
	ONG_WM_4003()
	ONG_VISIBLE_CHANGE()
	ONG_WM_4005()
	ONG_MOVE()
GFX_END_MESSAGE_MAP()

CIFMenu::CIFMenu(void)
{
   /* PingRegionID = 0;
    PingPosX = 0;
    PingPosY = 0;
    PingPosZ = 0;
    Seconds = 0;*/
   
	CanSendPing = false;
    m_profileFace = NULL;
    m_profileRace = NULL;
    m_profileName = NULL;
    m_profileGuild = NULL;
    m_originalMenuBuilt = false;
    m_maxiSettingsApplied = false;
    m_casyMenuBuilt = false;
	BS_DEBUG("> " __FUNCTION__);
}

CIFMenu::~CIFMenu(void)
{
	BS_DEBUG("> " __FUNCTION__);
}

bool CIFMenu::OnCreate(long ln)
{
	BS_DEBUG("> " __FUNCTION__ "(%d)", ln);

	// Populate inherited members
	CIFMainFrame::OnCreate(ln);

	m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmenu.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    if (IsSettingsSnapshotReady()) {
        if (IsCasyMenuEnabled()) {
            ConfigureCasyMenu(this);
            m_casyMenuBuilt = true;
        } else if (IsMaxiMenuEnabled()) {
            ConfigureMaxiMenu(this);
            m_maxiSettingsApplied = true;
        } else {
            BuildOriginalMenu(this);
            CreateProfileControls();
            m_originalMenuBuilt = true;
        }
    } else {
        // Do not construct the default/old design before MenuLikeMaxi has
        // arrived. Dynamically-created controls cannot be removed through the
        // resinfo manager later and would cover the imported Maxi layout.
        for (int id = 1; id <= 25; ++id)
            HideResource(this, id);
    }

    this->UpdateMenuSize();
    this->ShowGWnd(false);


	return true;
}

void CIFMenu::RenderMyself()
{
    // CIFFrame always paints its eight-piece black Silkroad window chrome.
    // CASY is a self-contained transparent artwork, so render only the base
    // window and its children while this layout is selected.
    if (IsCasyMenuEnabled()) {
        CIFWnd::RenderMyself();
        return;
    }

    CIFMainFrame::RenderMyself();
}

void CIFMenu::UpdateMenuSize()
{
    // The settings packet can arrive after this window is created. Some open
    // paths call UpdateMenuSize directly without giving a hidden window an
    // OnUpdate tick, so make the transition to the original layout here too.
    if (IsSettingsSnapshotReady() && IsCasyMenuEnabled() && !m_casyMenuBuilt) {
        ConfigureCasyMenu(this);
        m_casyMenuBuilt = true;
    }

    if (IsSettingsSnapshotReady() && !IsCasyMenuEnabled() &&
        !IsMaxiMenuEnabled() && !m_originalMenuBuilt) {
        BuildOriginalMenu(this);
        CreateProfileControls();
        m_originalMenuBuilt = true;
        if (IsVisible())
            SetCharFace();
    }

    const ClientResolutonData &res = CGame::GetClientDimensionStuff();

    if (IsCasyMenuEnabled()) {
        int targetX = (res.width - GetSize().width) / 2;
        int targetY = (res.height - GetSize().height) / 2;
        if (targetX < 0)
            targetX = 0;
        if (targetY < 0)
            targetY = 0;
        MoveGWnd(targetX, targetY);
        PositionCasyCloseButton(this);
        BringToFront();
        return;
    }

    if (IsMaxiMenuEnabled()) {
        MoveGWnd(((res.width - GetSize().width) - 100), GetPos().y);
        BringToFront();
        return;
    }

    int targetY = GetPos().y;
    if (targetY < 70)
        targetY = 70;

    MoveGWnd(((res.width - GetSize().width) - 100), targetY);
    PositionMenuCloseButton(this);
    this->BringToFront();

}
void CIFMenu::OnUpdate()
{
    CIFMainFrame::OnUpdate();

    if (!IsSettingsSnapshotReady())
        return;

    if (IsCasyMenuEnabled() && !m_casyMenuBuilt) {
        UpdateMenuSize();
        return;
    }

    if (!IsCasyMenuEnabled() && !IsMaxiMenuEnabled() && !m_originalMenuBuilt) {
        UpdateMenuSize();
        return;
    }

    if (!IsCasyMenuEnabled() && IsMaxiMenuEnabled() && !m_maxiSettingsApplied &&
        IsSettingsSnapshotReady()) {
        ConfigureMaxiMenu(this);
        m_maxiSettingsApplied = true;
    }
}

void CIFMenu::CreateProfileControls()
{
    m_profileFace = CreateImage(this, 4, 17, 8, 73, 73,
                                "juicer\\character\\char_ch_man1.ddj");
    m_profileRace = CreateImage(this, 7, 68, 56, 24, 24,
                                "interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj");

    m_profileName = CreateStatic(this, 6, 105, 19, 123, 21, L"", 0xFFFFA500,
                                 CTextBoard::JUSTIFY_LEFT);
    m_profileGuild = CreateStatic(this, 9, 105, 52, 123, 21, KmtGetText(L"UIIT_KMT_NO_GUILD"), 0xFFFFA500,
                                  CTextBoard::JUSTIFY_LEFT);

    if (m_profileName != NULL)
        m_profileName->TB_Func_13("juicer\\extra\\custom_text_bg.ddj", 0, 0);
    if (m_profileGuild != NULL)
        m_profileGuild->TB_Func_13("juicer\\extra\\custom_text_bg.ddj", 0, 0);
}

void CIFMenu::SetCharFace()
{
    if(g_pMyPlayerObj == NULL || m_profileFace == NULL || m_profileRace == NULL ||
       m_profileName == NULL || m_profileGuild == NULL)
        return;

    wchar_t playerName[256] = {0};
    wsprintfW(playerName, KmtGetText(L"UIIT_KMT_TEXT_LV_VALUE"), g_pMyPlayerObj->GetCharName().c_str(),
              static_cast<unsigned int>(g_pMyPlayerObj->GetCurrentLevel()));
    m_profileName->SetText(playerName);

    const std::wstring guildName = g_pMyPlayerObj->GetGuildName().c_str();
    m_profileGuild->SetText(guildName.empty() ? KmtGetText(L"UIIT_KMT_NO_GUILD") : guildName.c_str());

    const int refObjectId = g_pMyPlayerObj->GetCommonData()->RefObjectId;
    const char* portraitType = NULL;
    const char* raceTexture = NULL;
    int portraitIndex = 0;

    if (refObjectId >= 1907 && refObjectId <= 1919) {
        portraitType = "char_ch_man";
        portraitIndex = 1920 - refObjectId;
        raceTexture = "interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj";
    } else if (refObjectId >= 1920 && refObjectId <= 1932) {
        portraitType = "char_ch_woman";
        portraitIndex = 1933 - refObjectId;
        raceTexture = "interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj";
    } else if (refObjectId >= 14875 && refObjectId <= 14887) {
        portraitType = "char_eu_man";
        portraitIndex = refObjectId - 14874;
        raceTexture = "interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj";
    } else if (refObjectId >= 14888 && refObjectId <= 14900) {
        portraitType = "char_eu_woman";
        portraitIndex = refObjectId - 14887;
        raceTexture = "interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj";
    }

    if (portraitType != NULL) {
        char portraitTexture[96] = {0};
        sprintf(portraitTexture, "juicer\\character\\%s%d.ddj", portraitType, portraitIndex);
        m_profileFace->TB_Func_13(portraitTexture, 0, 0);
    }
    if (raceTexture != NULL)
        m_profileRace->TB_Func_13(raceTexture, 0, 0);
}

int CIFMenu::OnCreatedInstance(UINT a1, UINT a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", a1, a2);
	return 0;
}

int CIFMenu::On4002(int a1, int a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", a1, a2);
	return 0;
}

int CIFMenu::On4003(int a1, int a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", a1, a2);
	return 0;
}

int CIFMenu::OnVisibleStateChange(int newstate, int a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", newstate, a2);
	if (newstate) {
        OnUpdate();
        UpdateMenuSize();
        if (IsCasyMenuEnabled()) {
            PositionCasyCloseButton(this);
        } else if (!IsMaxiMenuEnabled()) {
            SetCharFace();
            PositionMenuCloseButton(this);
        }
    }
	return 0;
}

int CIFMenu::On4005(int a1, int a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", a1, a2);
	return 0;
}

int CIFMenu::OnWindowPosChanged(UINT a1, UINT a2)
{
	BS_DEBUG("> " __FUNCTION__ " ( %p, %p )", a1, a2);
    if (IsCasyMenuEnabled())
        PositionCasyCloseButton(this);
    else if (IsMaxiMenuEnabled())
        PositionMaxiCloseButton(this);
    else
        PositionMenuCloseButton(this);
	return 0;
}
void CIFMenu::On_BtnClickGrantName(){
    BS_DEBUG("> " __FUNCTION__);
    if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {

        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }

}
void CIFMenu::On_BtnClickTitleManager(){
    BS_DEBUG("> " __FUNCTION__);
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ClearDDJ();
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x0);
        SendMsg(buf);
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ActivateTabPage(0);
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");

    }


}
void CIFMenu::On_BtnClickIconManager(){
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ClearDDJ();
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x1);
        SendMsg(buf);
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->LoadItems();
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }

}
void CIFMenu::On_BtnClickRank()
{
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ClearCategories();
        CMsgStreamBuffer buf(0x180A);
        byte type = 0;
        buf << type;
        SendMsg(buf);
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ResetData();
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->Hide();
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");

    }
}

void CIFMenu::On_BtnClickUniqueLog(){
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->ClearSelection();
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList.clear();
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UpdateRanks(true);
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x2);
        SendMsg(buf);
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");

    }
}
void CIFMenu::On_BtnClickEventRegister(){

    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ClearDDJ();
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x3);
        SendMsg(buf);


        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->UpdateEvents();
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");

    }


}
void CIFMenu::On_BtnClickEventTimer(){
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->Clear();
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x4);
        SendMsg(buf);


        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->UpdateList();
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");

    }
}
void CIFMenu::On_BtnClickAchievements(){

    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {

        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ClearDDJ();
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ActivateTabPage(0);
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}
void CIFMenu::On_BtnClickChangelog(){
    if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else if(g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}
void CIFMenu::On_BtnClickAlchemyMacro(){
    if(g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
        {
            return;
        }
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->ShowGWnd(true);
        g_pCGInterface->GetMainPopup()->ShowGWnd(true);
        g_pCGInterface->GetMainPopup()->ShowSubPage(GDR_INVENTORY);
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->UpdateMenuSize();
        g_pCGInterface->LockMovement(13);
    }
}
void CIFMenu::On_BtnSettings()
{
    if(!g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1)->ShowGWnd(true);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
    else
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
}

CIFWnd *CIFMenu::GetMenuResource(int id)
{
    return m_IRM.GetResObj(id, 1);
}
