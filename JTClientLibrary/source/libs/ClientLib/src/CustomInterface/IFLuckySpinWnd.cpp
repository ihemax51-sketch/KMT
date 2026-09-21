#include "IFLuckySpinWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "CustomData/CustomDataManager.h"
#include "CustomData/CustomSettingManager.h"
#include "GInterface.h"
#include "Game.h"
#include "GlobalDataManager.h"
#include "SOItem.h"
#include <Windows.h>
#include <cmath>
#include <cstdio>

#define ID_SPIN_PANEL_FRAME 10
#define ID_SPIN_BG 11
#define ID_SPIN_WHEEL_BORDER 12
#define ID_SPIN_WHEEL_PANEL 13
#define ID_SPIN_FOOTER_BORDER 14
#define ID_SPIN_FOOTER 15
#define ID_SPIN_PRICE 16
#define ID_SPIN_STATUS 17
#define ID_SPIN_POINTER 18
#define ID_SPIN_CENTER 19
#define ID_SPIN_PLAY 20
#define ID_SPIN_RESULT 21
#define ID_SPIN_SLOT_BG 100
#define ID_SPIN_SLOT 200

namespace {
const float TWO_PI = 6.2831853f;
const float HALF_PI = 1.5707963f;
const int WINDOW_WIDTH = 450;
const int WINDOW_HEIGHT = 540;
const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 117);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 238, 215, 168);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 255, 255, 255);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 198, 190, 174);
const D3DCOLOR COLOR_ACTIVE = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARNING = D3DCOLOR_ARGB(255, 255, 132, 118);

void StyleStatic(CIFStatic* control, const wchar_t* text, D3DCOLOR color, CTextBoard::eJustifyHorizontal justify)
{
    if (!control) {
        return;
    }

    control->SetText(text);
    control->SetFont(theApp.GetFont(0));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(justify);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->SetClickable(false);
    control->ShowGWnd(true);
    control->BringToFront();
}

void StyleButton(CIFButton* button, const wchar_t* text)
{
    if (!button) {
        return;
    }

    button->TB_Func_13("interface\\ifcommon\\com_button.ddj", 1, 1);
    button->SetText(text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->SetEnabledState(true);
    button->ShowGWnd(true);
    button->BringToFront();
}

void AlignCloseButton(CIFMainFrame* frame, int width)
{
    if (!frame || !frame->m_pCloseBtn) {
        return;
    }

    frame->m_pCloseBtn->TB_Func_13(
        "interface\\ifcommon\\com_windowclose.ddj", 0, 0);
    frame->m_pCloseBtn->SetGWndSize(16, 16);
    frame->m_pCloseBtn->MoveGWnd(
        frame->GetPos().x + width - 26,
        frame->GetPos().y + 9);
    frame->m_pCloseBtn->ShowGWnd(true);
    frame->m_pCloseBtn->BringToFront();
}

CIFFrame* CreateInsetFrame(CIFMainFrame* owner, int id, int x, int y, int width, int height)
{
    RECT rect = {x, y, width, height};
    CIFFrame* frame = (CIFFrame*)CGWnd::CreateInstance(
        owner, GFX_RUNTIME_CLASS(CIFFrame), rect, id, 0);
    if (frame) {
        frame->SetFrameTexture(std::n_string("interface\\inventory\\int_window_"));
        frame->SetClickable(false);
        frame->ShowGWnd(true);
    }
    return frame;
}

CIFNormalTile* CreateTile(CIFMainFrame* owner, int id, int x, int y, int width, int height, const char* texture)
{
    RECT rect = {x, y, width, height};
    CIFNormalTile* tile = (CIFNormalTile*)CGWnd::CreateInstance(
        owner, GFX_RUNTIME_CLASS(CIFNormalTile), rect, id, 0);
    if (tile) {
        tile->TB_Func_13(texture, 0, 1);
        tile->SetClickable(false);
        tile->ShowGWnd(true);
    }
    return tile;
}

CIFStatic* CreateFormSurface(CIFMainFrame* owner, int id, int x, int y, int width, int height)
{
    RECT rect = {x, y, width, height};
    CIFStatic* surface = (CIFStatic*)CGWnd::CreateInstance(
        owner, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
    if (surface) {
        surface->TB_Func_13(
            "interface\\ifcommon\\com_grad_gage_form.ddj", 0, 0);
        surface->SetClickable(false);
        surface->ShowGWnd(true);
    }
    return surface;
}
}

GFX_IMPLEMENT_DYNCREATE(CIFLuckySpinWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFLuckySpinWnd, CIFMainFrame)
                    ONG_COMMAND(ID_SPIN_PLAY, &CIFLuckySpinWnd::OnPlay)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFLuckySpinShortcutButton, CIFButton)
GFX_IMPLEMENT_DYNCREATE(CIFLuckySpinGuide, CIFDecoratedStatic)

CIFLuckySpinWnd::CIFLuckySpinWnd()
    : m_panelFrame(0), m_background(0), m_wheelBorder(0), m_wheelPanel(0),
      m_footerBorder(0),
      m_priceLabel(0), m_statusLabel(0), m_pointerLabel(0), m_centerLabel(0),
      m_resultLabel(0), m_playButton(0),
      m_highlightedSlot(-1), m_totalSpinSteps(0), m_completedSpinSteps(0),
      m_lastSpinStepTick(0), m_spinRequestTick(0), m_spinStartTick(0), m_spinDuration(0),
      m_wheelAngle(0.0f), m_spinStartAngle(0.0f), m_spinEndAngle(0.0f), m_winningSlot(-1),
      m_waitingForResult(false), m_isSpinning(false) {
    for (int i = 0; i < 16; ++i) {
        m_slotBackgrounds[i] = 0;
        m_slots[i] = 0;
    }
}

CIFLuckySpinWnd::~CIFLuckySpinWnd() {
}

bool CIFLuckySpinWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_LUCKY_SPIN"));
    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    m_panelFrame = CreateInsetFrame(this, ID_SPIN_PANEL_FRAME, 7, 37, 436, 454);
    m_background = CreateTile(
        this, ID_SPIN_BG, 20, 49, 410, 429,
        "interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj");
    m_wheelBorder = CreateInsetFrame(
        this, ID_SPIN_WHEEL_BORDER, 36, 111, 378, 327);
    m_wheelPanel = CreateTile(
        this, ID_SPIN_WHEEL_PANEL, 48, 123, 354, 302,
        "interface\\ifcommon\\bg_tile\\com_bg_tile_e.ddj");
    m_footerBorder = CreateFormSurface(
        this, ID_SPIN_FOOTER_BORDER, 36, 448, 378, 24);

    RECT priceRect = {34, 55, 382, 20};
    m_priceLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), priceRect, ID_SPIN_PRICE, 0);
    if (m_priceLabel) {
        StyleStatic(m_priceLabel, L"", COLOR_ACCENT, CTextBoard::JUSTIFY_CENTER);
    }

    RECT statusRect = {34, 81, 382, 20};
    m_statusLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_SPIN_STATUS, 0);
    if (m_statusLabel) {
        StyleStatic(m_statusLabel, KmtGetText(L"UIIT_KMT_LUCKY_REWARDS"), COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);
    }

    RECT pointerRect = {209, 116, 32, 20};
    m_pointerLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), pointerRect, ID_SPIN_POINTER, 0);
    if (m_pointerLabel) {
        StyleStatic(m_pointerLabel, L"V", COLOR_ACCENT, CTextBoard::JUSTIFY_CENTER);
    }

    RECT centerRect = {167, 260, 116, 32};
    m_centerLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), centerRect, ID_SPIN_CENTER, 0);
    if (m_centerLabel) {
        StyleStatic(
            m_centerLabel,
            KmtGetText(L"UIIT_KMT_LUCKY_SPIN"),
            COLOR_LABEL,
            CTextBoard::JUSTIFY_CENTER);
    }

    for (int i = 0; i < 16; ++i) {
        RECT slotRect = {0, 0, 40, 40};

        m_slotBackgrounds[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), slotRect, ID_SPIN_SLOT_BG + i, 0);
        if (m_slotBackgrounds[i]) {
            m_slotBackgrounds[i]->TB_Func_13("interface\\store\\str_slot_02.ddj", 0, 0);
            m_slotBackgrounds[i]->SetClickable(false);
            m_slotBackgrounds[i]->ShowGWnd(true);
        }

        RECT itemRect = {4, 4, 32, 32};
        m_slots[i] = (CIFSlotWithHelp*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), itemRect, ID_SPIN_SLOT + i, 0);
        if (m_slots[i]) {
            m_slots[i]->SetType(19);
            m_slots[i]->SetSlot(ID_SPIN_SLOT + i);
            m_slots[i]->SetClickable(false);
            m_slots[i]->ShowGWnd(true);
        }
    }

    RECT resultRect = {48, 451, 354, 18};
    m_resultLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), resultRect, ID_SPIN_RESULT, 0);
    if (m_resultLabel) {
        StyleStatic(m_resultLabel, KmtGetText(L"UIIT_KMT_REWARDS_ARE_DELIVERED_DIRECTLY_TO_CHEST"), COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);
    }

    RECT playRect = {187, 505, 76, 24};
    m_playButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), playRect, ID_SPIN_PLAY, 0);
    StyleButton(m_playButton, KmtGetText(L"UIIT_KMT_SPIN"));

    if (!m_panelFrame || !m_background || !m_wheelBorder || !m_wheelPanel ||
        !m_footerBorder || !m_priceLabel || !m_statusLabel ||
        !m_pointerLabel || !m_centerLabel || !m_resultLabel || !m_playButton) {
        return false;
    }
    for (int slot = 0; slot < 16; ++slot) {
        if (!m_slotBackgrounds[slot] || !m_slots[slot]) {
            return false;
        }
    }

    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_TEXT);
    }
    AlignCloseButton(this, WINDOW_WIDTH);
    UpdateWindowPos();
    UpdateWheelLayout(0.0f);
    ShowGWnd(false);
    return true;
}

void CIFLuckySpinWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        RefreshRewards();
        BringToFront();

        // Request after the in-game packet processor is ready. Character-select
        // packets can arrive before that processor exists on this client build.
        CMsgStreamBuffer request(0x169A);
        request << byte(29);
        SendMsg(request);
    }
}

void CIFLuckySpinWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();

    const DWORD now = GetTickCount();
    if (m_waitingForResult && !m_isSpinning && now - m_spinRequestTick > 6000) {
        m_waitingForResult = false;
        if (m_playButton) m_playButton->SetEnabledState(true);
        if (m_statusLabel) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_SPIN_REQUEST_TIMED_OUT_PLEASE_TRY_AGAIN"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_WARNING);
        }
    }

    if (!m_isSpinning) {
        return;
    }

    DWORD elapsed = now - m_spinStartTick;
    if (elapsed >= m_spinDuration) {
        m_wheelAngle = m_spinEndAngle;
        UpdateWheelLayout(m_wheelAngle);
        SetHighlightedSlot(m_winningSlot);
        m_isSpinning = false;
        m_waitingForResult = false;
        if (m_playButton) m_playButton->SetEnabledState(true);
        if (m_statusLabel) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_CONGRATULATIONS_YOUR_REWARD_WAS_ADDED_TO_THE_CHEST"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_ACTIVE);
        }
        if (m_resultLabel) {
            m_resultLabel->SetText(KmtGetText(L"UIIT_KMT_WINNER_SELECTED_OPEN_CHEST_TO_CLAIM_YOUR_PRIZE"));
            m_resultLabel->m_FontTexture.SetColor(COLOR_ACTIVE);
        }
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        return;
    }

    float t = (float)elapsed / (float)m_spinDuration;
    float inv = 1.0f - t;
    float eased = 1.0f - (inv * inv * inv);
    m_wheelAngle = m_spinStartAngle + ((m_spinEndAngle - m_spinStartAngle) * eased);
    UpdateWheelLayout(m_wheelAngle);

    const float step = TWO_PI / 16.0f;
    int active = (int)floor((-m_wheelAngle / step) + 0.5f);
    active %= 16;
    if (active < 0) {
        active += 16;
    }
    SetHighlightedSlot(active);
}

undefined1 CIFLuckySpinWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFLuckySpinWnd::UpdateWindowPos() {
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int x = (width - WINDOW_WIDTH) / 2;
    int y = ((height - WINDOW_HEIGHT) / 2) + 10;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (y + GetSize().height > height - 20) {
        y = height - GetSize().height - 20;
    }
    if (y < 0) {
        y = 0;
    }

    MoveGWnd(x, y);
    AlignCloseButton(this, WINDOW_WIDTH);
    UpdateWheelLayout(m_wheelAngle);
}

void CIFLuckySpinWnd::UpdateWheelLayout(float angle) {
    const int baseX = GetPos().x;
    const int baseY = GetPos().y;
    const int centerX = 225;
    const int centerY = 276;
    const int radius = 118;

    for (int i = 0; i < 16; ++i) {
        const float slotAngle = ((TWO_PI * (float)i) / 16.0f) - HALF_PI + angle;
        const int x = centerX + (int)(radius * cos(slotAngle)) - 20;
        const int y = centerY + (int)(radius * sin(slotAngle)) - 20;

        if (m_slotBackgrounds[i]) {
            m_slotBackgrounds[i]->MoveGWnd(baseX + x, baseY + y);
            m_slotBackgrounds[i]->BringToFront();
        }

        if (m_slots[i]) {
            m_slots[i]->MoveGWnd(baseX + x + 4, baseY + y + 4);
            m_slots[i]->BringToFront();
        }
    }

    if (m_playButton) m_playButton->BringToFront();
    if (m_centerLabel) m_centerLabel->BringToFront();
    if (m_pointerLabel) m_pointerLabel->BringToFront();
}

void CIFLuckySpinWnd::ClearSlot(int index) {
    if (index >= 0 && index < 16 && m_slots[index]) {
        m_slots[index]->SetSlotData(0);
        m_slots[index]->ItemInfo = 0;
    }
}

void CIFLuckySpinWnd::SetHighlightedSlot(int index) {
    if (index < 0) {
        for (int i = 0; i < 16; ++i) {
            if (m_slots[i]) {
                m_slots[i]->SlotisLocked = 0;
            }
        }
        m_highlightedSlot = -1;
        return;
    }

    if (index >= 16) {
        return;
    }

    for (int i = 0; i < 16; ++i) {
        if (m_slots[i]) {
            // SlotisLocked=10 uses the native golden mall selection overlay.
            m_slots[i]->SlotisLocked = i == index ? 10 : 0;
        }
    }
    m_highlightedSlot = index;
}

void CIFLuckySpinWnd::FillSlot(int index, int refObjId, int amount) {
    if (index < 0 || index >= 16 || !m_slots[index] || refObjId <= 0) {
        return;
    }

    const SItemData* data = &g_CGlobalDataManager->GetItemData(refObjId);
    if (!data) {
        return;
    }

    CMsgStreamBuffer packet(0xB034);
    packet << INT32(0) << INT32(refObjId);
    const u_short typeID2 = data->m_typeId.getTypeID2();
    const u_short typeID3 = data->m_typeId.getTypeID3();
    const u_short typeID4 = data->m_typeId.getTypeID4();
    if (typeID2 == 1) {
        packet << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
    } else if (typeID2 == 2) {
        if (typeID3 == 1) packet << UINT8(1);
        else if (typeID3 == 2) packet << UINT32(0);
        else if (typeID4 == 3) packet << UINT32(1);
    } else if (typeID2 == 3) {
        packet << UINT16(1);
        if (typeID3 == 11 && (typeID4 == 1 || typeID4 == 2)) packet << UINT8(0);
    }

    CSOItem* item = new CSOItem();
    item->ReadFromPacket(&packet, 1);
    item->SetEnabled(true);
    item->m_quantity = amount;
    item->m_OptLevel = 0;
    m_slots[index]->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);
    m_slots[index]->ItemInfo = item;
    m_slots[index]->SetType(19);
    m_slots[index]->ShowGWnd(true);
}

void CIFLuckySpinWnd::RefreshRewards() {
    wchar_t price[128];
    const wchar_t* currency = m_Settings->EnableLuckySpinSilk
        ? KmtGetText(L"UIIT_KMT_SILK")
        : KmtGetText(L"UIIT_KMT_GOLD");
    swprintf(price, 128, KmtGetText(L"UIIT_KMT_PLAY_PRICE_VALUE_TEXT"), m_Settings->LuckySpinPrice, currency);
    if (m_priceLabel) m_priceLabel->SetText(price);

    for (int i = 0; i < 16; ++i) ClearSlot(i);
    const std::vector<CustomDataManager::LuckySpinReward>& rewards = m_CustomDataManager->LuckySpinRewards;
    const int count = rewards.size() < 16 ? (int)rewards.size() : 16;
    for (int i = 0; i < count; ++i) FillSlot(i, rewards[i].ItemID, rewards[i].Amount);

    if (m_statusLabel) {
        m_statusLabel->SetText(count > 0 ? KmtGetText(L"UIIT_KMT_PRESS_LUCKY_SPIN_TO_TRY_YOUR_LUCK") : KmtGetText(L"UIIT_KMT_NO_REWARDS_CONFIGURED_YET"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_LABEL);
    }
    if (m_resultLabel) {
        m_resultLabel->SetText(count > 0 ? KmtGetText(L"UIIT_KMT_THE_WHEEL_WILL_STOP_ON_YOUR_SELECTED_REWARD") : KmtGetText(L"UIIT_KMT_CONFIGURE_REWARDS_IN_KMTGUARD_DATABASE_FIRST"));
        m_resultLabel->m_FontTexture.SetColor(COLOR_MUTED);
    }
    UpdateWheelLayout(m_wheelAngle);
    SetHighlightedSlot(-1);
    // Keep the Silkroad button style even while the admin is still configuring rewards.
    // OnPlay gives the player the precise reason when a spin is unavailable.
    if (m_playButton) m_playButton->SetEnabledState(true);
}

void CIFLuckySpinWnd::OnPlay() {
    if (!m_Settings->EnableLuckySpin || m_CustomDataManager->LuckySpinRewards.empty() || m_waitingForResult || m_isSpinning) {
        return;
    }
    m_waitingForResult = true;
    m_spinRequestTick = GetTickCount();
    if (m_playButton) m_playButton->SetEnabledState(true);
    if (m_statusLabel) {
        m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_PREPARING_THE_WHEEL"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    if (m_resultLabel) {
        m_resultLabel->SetText(KmtGetText(L"UIIT_KMT_WAITING_FOR_KMTGUARD_TO_SELECT_THE_REWARD"));
        m_resultLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    CMsgStreamBuffer packet(0x169A);
    packet << byte(27);
    SendMsg(packet);
}

void CIFLuckySpinWnd::StartSpin(int winningSlot) {
    const int rewardCount = m_CustomDataManager->LuckySpinRewards.size() < 16
        ? (int)m_CustomDataManager->LuckySpinRewards.size()
        : 16;
    if (rewardCount == 0) {
        return;
    }

    const int target = ((winningSlot % rewardCount) + rewardCount) % rewardCount;
    m_totalSpinSteps = 0;
    m_completedSpinSteps = 0;
    m_winningSlot = target;

    const float step = TWO_PI / 16.0f;
    m_spinStartAngle = m_wheelAngle;
    m_spinEndAngle = -(step * (float)target);
    while (m_spinEndAngle <= m_spinStartAngle + (TWO_PI * 4.0f)) {
        m_spinEndAngle += TWO_PI;
    }

    m_spinStartTick = GetTickCount();
    m_spinDuration = 4300;
    m_lastSpinStepTick = m_spinStartTick;
    m_isSpinning = true;
    m_waitingForResult = false;
    SetHighlightedSlot(-1);
    if (m_statusLabel) {
        m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_THE_WHEEL_IS_SPINNING"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    if (m_resultLabel) {
        m_resultLabel->SetText(KmtGetText(L"UIIT_KMT_HOLD_ON_THE_WHEEL_IS_SLOWING_DOWN"));
        m_resultLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
}

CIFLuckySpinShortcutButton::CIFLuckySpinShortcutButton() {
}

CIFLuckySpinShortcutButton::~CIFLuckySpinShortcutButton() {
}

int CIFLuckySpinShortcutButton::OnMouseLeftUp(int a1, int x, int y) {
    if (g_pCGInterface) {
        CIFLuckySpinWnd* window = g_pCGInterface->GetGuiFromList<CIFLuckySpinWnd>(LUCKY_SPIN_WINDOW_ID);
        if (window) {
            window->ShowGWnd(!window->IsVisible());
            return 0;
        }
    }
    return CIFButton::OnMouseLeftUp(a1, x, y);
}

bool CIFLuckySpinGuide::OnCreate(long ln) {
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_lucky_spin_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_lucky_spin_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_LUCKY_SPIN"));
        label->SetStyleThingy(TOOLTIP);
    }
    return true;
}

int CIFLuckySpinGuide::OnMouseLeftUp(int a1, int x, int y) {
    if (!g_pCGInterface) {
        return 0;
    }
    CIFLuckySpinWnd* window = g_pCGInterface->GetGuiFromList<CIFLuckySpinWnd>(LUCKY_SPIN_WINDOW_ID);
    if (!window) {
        return 0;
    }
    window->ShowGWnd(!window->IsVisible());
    CGEffSoundBody::get()->PlaySound(window->IsVisible() ? L"snd_window_open" : L"snd_window_close");
    return 0;
}

void CIFLuckySpinGuide::OnCIFReady() {
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
