#include "IFOfflineStall.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "CustomData/CustomSettingManager.h"
#include "GInterface.h"
#include "Game.h"
#include "ICPlayer.h"
#include <BSLib/multibyte.h>
#include <Windows.h>

#define ID_OFFLINE_CONFIRM_BG 10
#define ID_OFFLINE_CONFIRM_HEADER 11
#define ID_OFFLINE_CONFIRM_MESSAGE 12
#define ID_OFFLINE_CONFIRM_DETAIL 13
#define ID_OFFLINE_CONFIRM_STATUS 14
#define ID_OFFLINE_CONFIRM_OK 20
#define ID_OFFLINE_CONFIRM_CANCEL 21

#define ID_OFFLINE_LOGIN_BG 30
#define ID_OFFLINE_LOGIN_HEADER 31
#define ID_OFFLINE_LOGIN_MESSAGE 32
#define ID_OFFLINE_LOGIN_QUESTION 33
#define ID_OFFLINE_LOGIN_COUNTDOWN 34
#define ID_OFFLINE_LOGIN_DISCONNECT 40
#define ID_OFFLINE_LOGIN_CANCEL 41

#define GDR_STALL_OWNERSTATE_MSG 14

namespace {
const int CONFIRM_WIDTH = 420;
const int CONFIRM_HEIGHT = 210;
const int LOGIN_WIDTH = 440;
const int LOGIN_HEIGHT = 220;
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 245, 242, 232);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 185, 184, 174);
const D3DCOLOR COLOR_ACTIVE = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARNING = D3DCOLOR_ARGB(255, 255, 132, 118);

DWORD g_lastActivationNonce = 0;

void StyleStatic(CIFStatic* control, const wchar_t* text, D3DCOLOR color,
                 CTextBoard::eJustifyHorizontal justify)
{
    if (!control) {
        return;
    }

    control->SetText(text ? text : L"");
    control->SetFont(theApp.GetFont(0));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(justify);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->SetClickable(false);
    control->ShowGWnd(true);
    control->BringToFront();
}

void StyleButton(CIFButton* button, const wchar_t* text, const char* texture)
{
    if (!button) {
        return;
    }

    button->TB_Func_13(texture, 1, 1);
    button->SetText(text ? text : L"");
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->ShowGWnd(true);
    button->BringToFront();
}

void StyleMainFrame(CIFMainFrame* frame, const wchar_t* title, int width)
{
    if (!frame) {
        return;
    }

    frame->TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    frame->SetText(title);
    if (frame->m_pTitleText) {
        frame->m_pTitleText->m_FontTexture.SetColor(COLOR_LABEL);
        frame->m_pTitleText->BringToFront();
    }
    if (frame->m_pCloseBtn) {
        frame->m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
        frame->m_pCloseBtn->SetGWndSize(28, 32);
        frame->m_pCloseBtn->MoveGWnd(width - 36, 2);
        frame->m_pCloseBtn->ShowGWnd(true);
        frame->m_pCloseBtn->BringToFront();
    }
}

void CenterWindow(CIFWnd* window)
{
    if (!window) {
        return;
    }

    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int x = (width - window->GetSize().width) / 2;
    int y = (height - window->GetSize().height) / 2;
    if (x < 0) x = 0;
    if (y < 35) y = 35;
    window->MoveGWnd(x, y);
}

DWORD CreateActivationNonce()
{
    DWORD next = GetTickCount();
    if (next == 0 || next <= g_lastActivationNonce) {
        next = g_lastActivationNonce + 1;
        if (next == 0) {
            next = 1;
        }
    }
    g_lastActivationNonce = next;
    return next;
}

CIFWnd* GetNativeStallStateControl(CIFWnd* stallWindow)
{
    if (!stallWindow) {
        return 0;
    }

    CGWndBase* control = stallWindow->GetChildControl(GDR_STALL_OWNERSTATE_MSG);
    if (!control || !control->IsKindOf(GFX_RUNTIME_CLASS(CIFWnd))) {
        return 0;
    }

    return static_cast<CIFWnd*>(control);
}

bool IsNativeStallOperating(CIFWnd* stallWindow)
{
    if (!stallWindow) {
        return false;
    }

    // CIFStall::SetState (v1.188, 0x006BAB20) stores the authoritative
    // Modifying/Operating byte at +0x800. Reading the native state avoids
    // coupling the feature to the current client language.
    const BYTE state = *reinterpret_cast<const BYTE*>(
        reinterpret_cast<const char*>(stallWindow) + 0x800);
    return state == 0x01;
}
}

GFX_IMPLEMENT_DYNCREATE(CIFOfflineStallButton, CIFButton)

GFX_IMPLEMENT_DYNCREATE(CIFOfflineStallConfirmWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFOfflineStallConfirmWnd, CIFMainFrame)
                    ONG_COMMAND(ID_OFFLINE_CONFIRM_OK, &CIFOfflineStallConfirmWnd::OnConfirm)
                    ONG_COMMAND(ID_OFFLINE_CONFIRM_CANCEL, &CIFOfflineStallConfirmWnd::OnCancel)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFOfflineStallLoginPrompt, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFOfflineStallLoginPrompt, CIFMainFrame)
                    ONG_COMMAND(ID_OFFLINE_LOGIN_DISCONNECT, &CIFOfflineStallLoginPrompt::OnDisconnect)
                    ONG_COMMAND(ID_OFFLINE_LOGIN_CANCEL, &CIFOfflineStallLoginPrompt::OnCancel)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFOfflineStallLoginButton, CIFButton)

bool OfflineStall_IsEnabled()
{
    return m_Settings != 0 && m_Settings->EnableOfflineStall;
}

CIFOfflineStallButton::CIFOfflineStallButton() {
}

CIFOfflineStallButton::~CIFOfflineStallButton() {
}

bool CIFOfflineStallButton::OnCreate(long ln)
{
    CIFButton::OnCreate(ln);
    StyleButton(this, KmtGetText(L"UIIT_KMT_GO_OFFLINE"), "clientlibrary\\mall\\mall_pre_big_button.ddj");
    SetGWndSize(96, 24);
    MoveGWnd(-500, -500);
    ShowGWnd(true);
    SetEnabledState(true);
    return true;
}

void CIFOfflineStallButton::OnUpdate()
{
    CIFButton::OnUpdate();

    CIFWnd* stallWindow = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFWnd>(GDR_STALL, 1)
        : 0;

    const bool featureEnabled = OfflineStall_IsEnabled();
    const bool ownerStallOpen = featureEnabled && g_pMyPlayerObj != 0 &&
                                g_pMyPlayerObj->CHARACTER_STATUS == Stall;
    CIFWnd* stateControl = GetNativeStallStateControl(stallWindow);
    const bool stallOperating = ownerStallOpen && IsNativeStallOperating(stallWindow);

    // Never hide this overlay: hidden native controls may stop receiving
    // OnUpdate. Park it off-screen for visitors and while CIFStall is closed,
    // then anchor it only for the owner of the currently operating stall.
    if (!stallWindow || !stallWindow->IsVisible() || !stallOperating || !stateControl) {
        MoveGWnd(-500, -500);
        return;
    }

    const wnd_rect stateBounds = stateControl->GetBounds();
    const wnd_size buttonSize = GetSize();
    const int buttonX = stateBounds.pos.x - buttonSize.width - 8;
    int buttonY = stateBounds.pos.y;
    if (stateBounds.size.height > buttonSize.height) {
        buttonY += (stateBounds.size.height - buttonSize.height) / 2;
    }
    // Anchor to the native Operating/Modifying control itself so the button
    // remains exactly aligned at every UI scale and resolution.
    MoveGWnd(buttonX, buttonY);
    BringToFront();

    CIFOfflineStallConfirmWnd* window = g_pCGInterface
        ? g_pCGInterface->GetGuiFromList<CIFOfflineStallConfirmWnd>(OFFLINE_STALL_CONFIRM_WINDOW_ID)
        : 0;
    const bool pending = window && window->IsRequestPending();
    const bool armed = window && window->IsArmed();
    // Keep the ready state visually textured. The native client renders some
    // dynamically-created disabled buttons as solid white after a scene/state
    // transition; OnMouseLeftUp already rejects an armed request.
    // Keep the overlay out of the broken native disabled rendering path.
    // Request state is enforced by OnMouseLeftUp/confirm-window flags.
    SetEnabledState(true);
    SetText(armed ? KmtGetText(L"UIIT_KMT_OFFLINE_READY") : (pending ? KmtGetText(L"UIIT_KMT_ACTIVATING") : KmtGetText(L"UIIT_KMT_GO_OFFLINE")));
}

int CIFOfflineStallButton::OnMouseLeftUp(int a1, int x, int y)
{
    CIFWnd* stallWindow = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFWnd>(GDR_STALL, 1)
        : 0;
    if (!OfflineStall_IsEnabled() || !g_pCGInterface || !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS != Stall ||
        !stallWindow || !stallWindow->IsVisible() || !IsNativeStallOperating(stallWindow)) {
        return 0;
    }

    CIFOfflineStallConfirmWnd* window =
        g_pCGInterface->GetGuiFromList<CIFOfflineStallConfirmWnd>(OFFLINE_STALL_CONFIRM_WINDOW_ID);
    if (window && !window->IsRequestPending() && !window->IsArmed()) {
        window->Open();
    }
    return 0;
}

CIFOfflineStallConfirmWnd::CIFOfflineStallConfirmWnd()
    : m_background(0), m_header(0), m_message(0), m_detail(0), m_status(0),
      m_confirmButton(0), m_cancelButton(0), m_nonce(0), m_requestTick(0),
      m_pending(false), m_armed(false) {
}

CIFOfflineStallConfirmWnd::~CIFOfflineStallConfirmWnd() {
}

bool CIFOfflineStallConfirmWnd::OnCreate(long ln)
{
    CIFMainFrame::OnCreate(ln);
    SetGWndSize(CONFIRM_WIDTH, CONFIRM_HEIGHT);
    StyleMainFrame(this, KmtGetText(L"UIIT_KMT_OFFLINE_STALL"), CONFIRM_WIDTH);

    RECT bgRect = {0, 31, CONFIRM_WIDTH, CONFIRM_HEIGHT - 31};
    m_background = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), bgRect, ID_OFFLINE_CONFIRM_BG, 0);
    if (m_background) {
        m_background->TB_Func_13("clientlibrary\\mall\\win_bg.ddj", 0, 0);
        m_background->SetClickable(false);
        m_background->ShowGWnd(true);
    }

    RECT headerRect = {0, 31, CONFIRM_WIDTH, 45};
    m_header = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), headerRect, ID_OFFLINE_CONFIRM_HEADER, 0);
    if (m_header) {
        m_header->TB_Func_13("clientlibrary\\mall\\header.ddj", 0, 0);
        m_header->SetClickable(false);
        m_header->ShowGWnd(true);
    }

    RECT messageRect = {24, 72, CONFIRM_WIDTH - 48, 24};
    m_message = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), messageRect, ID_OFFLINE_CONFIRM_MESSAGE, 0);
    StyleStatic(m_message, KmtGetText(L"UIIT_KMT_KEEP_THIS_STALL_SELLING_AFTER_YOU_CLOSE_THE_CLIENT"), COLOR_TEXT, CTextBoard::JUSTIFY_CENTER);

    RECT detailRect = {24, 99, CONFIRM_WIDTH - 48, 22};
    m_detail = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), detailRect, ID_OFFLINE_CONFIRM_DETAIL, 0);
    StyleStatic(m_detail, KmtGetText(L"UIIT_KMT_YOUR_CHARACTER_WILL_REMAIN_IN_GAME_AS_A_PROTECTED_STALL"), COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

    RECT statusRect = {24, 126, CONFIRM_WIDTH - 48, 22};
    m_status = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_OFFLINE_CONFIRM_STATUS, 0);
    StyleStatic(m_status, L"", COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

    RECT confirmRect = {88, 159, 112, 32};
    m_confirmButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), confirmRect, ID_OFFLINE_CONFIRM_OK, 0);
    StyleButton(m_confirmButton, KmtGetText(L"UIIT_KMT_ACTIVATE"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    RECT cancelRect = {220, 159, 112, 32};
    m_cancelButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), cancelRect, ID_OFFLINE_CONFIRM_CANCEL, 0);
    StyleButton(m_cancelButton, KmtGetText(L"UIIT_KMT_CANCEL"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    CenterWindow(this);
    ShowGWnd(false);
    return true;
}

void CIFOfflineStallConfirmWnd::ShowGWnd(bool visible)
{
    CIFMainFrame::ShowGWnd(visible);
    if (visible) {
        UpdateWindowPos();
        BringToFront();
    }
}

void CIFOfflineStallConfirmWnd::OnUpdate()
{
    CIFMainFrame::OnUpdate();
    if (m_pending && GetTickCount() - m_requestTick >= 8000) {
        m_pending = false;
        if (m_confirmButton) m_confirmButton->SetEnabledState(true);
        SetStatus(KmtGetText(L"UIIT_KMT_ACTIVATION_TIMED_OUT_PLEASE_TRY_AGAIN"), COLOR_WARNING);
    }

    if ((m_pending || m_armed) && (!g_pMyPlayerObj || g_pMyPlayerObj->CHARACTER_STATUS != Stall)) {
        m_pending = false;
        m_armed = false;
        m_nonce = 0;
        ShowGWnd(false);
    }
}

undefined1 CIFOfflineStallConfirmWnd::OnCloseWnd()
{
    ShowGWnd(false);
    return true;
}

void CIFOfflineStallConfirmWnd::Open()
{
    CIFWnd* stallWindow = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFWnd>(GDR_STALL, 1)
        : 0;
    if (!OfflineStall_IsEnabled() || !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS != Stall ||
        !stallWindow || !stallWindow->IsVisible() || !IsNativeStallOperating(stallWindow)) {
        return;
    }

    if (!m_pending && !m_armed) {
        SetStatus(KmtGetText(L"UIIT_KMT_READY_TO_ACTIVATE"), COLOR_LABEL);
        if (m_confirmButton) m_confirmButton->SetEnabledState(true);
        if (m_cancelButton) {
            m_cancelButton->SetEnabledState(true);
            m_cancelButton->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));
        }
    }
    ShowGWnd(true);
}

void CIFOfflineStallConfirmWnd::HandleResult(DWORD nonce, bool success, const wchar_t* message)
{
    if (!m_pending || nonce != m_nonce) {
        return;
    }

    m_pending = false;
    m_armed = success;
    if (m_confirmButton) m_confirmButton->SetEnabledState(!success);
    if (m_cancelButton) m_cancelButton->SetText(success ? KmtGetText(L"UIIT_KMT_CLOSE") : KmtGetText(L"UIIT_KMT_CANCEL"));
    SetStatus(message && message[0] ? message :
              (success ? KmtGetText(L"UIIT_KMT_OFFLINE_STALL_ACTIVATED_YOU_MAY_CLOSE_THE_CLIENT")
                       : KmtGetText(L"UIIT_KMT_OFFLINE_STALL_ACTIVATION_FAILED")),
              success ? COLOR_ACTIVE : COLOR_WARNING);
    ShowGWnd(true);
}

bool CIFOfflineStallConfirmWnd::IsRequestPending() const {
    return m_pending;
}

bool CIFOfflineStallConfirmWnd::IsArmed() const {
    return m_armed;
}

void CIFOfflineStallConfirmWnd::OnConfirm()
{
    CIFWnd* stallWindow = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFWnd>(GDR_STALL, 1)
        : 0;
    if (m_pending || m_armed || !OfflineStall_IsEnabled() || !g_pMyPlayerObj ||
        g_pMyPlayerObj->CHARACTER_STATUS != Stall ||
        !stallWindow || !stallWindow->IsVisible() || !IsNativeStallOperating(stallWindow)) {
        SetStatus(KmtGetText(L"UIIT_KMT_OPEN_THE_STALL_BEFORE_ACTIVATING_OFFLINE_STALL"), COLOR_WARNING);
        return;
    }

    m_nonce = CreateActivationNonce();
    CMsgStreamBuffer packet(OFFLINE_STALL_ACTIVATE_REQUEST_OPCODE);
    packet << m_nonce;
    SendMsg(packet);

    m_pending = true;
    m_requestTick = GetTickCount();
    if (m_confirmButton) m_confirmButton->SetEnabledState(false);
    SetStatus(KmtGetText(L"UIIT_KMT_ACTIVATING_OFFLINE_STALL"), COLOR_LABEL);
}

void CIFOfflineStallConfirmWnd::OnCancel()
{
    ShowGWnd(false);
}

void CIFOfflineStallConfirmWnd::UpdateWindowPos() {
    CenterWindow(this);
}

void CIFOfflineStallConfirmWnd::SetStatus(const wchar_t* text, D3DCOLOR color)
{
    if (!m_status) return;
    m_status->SetText(text ? text : L"");
    m_status->m_FontTexture.SetColor(color);
}

CIFOfflineStallLoginPrompt::CIFOfflineStallLoginPrompt()
    : m_background(0), m_header(0), m_message(0), m_question(0), m_countdown(0),
      m_disconnectButton(0), m_cancelButton(0), m_token(0), m_requestTick(0),
      m_timeoutSeconds(5), m_actionButtonId(0), m_lastDisplayedSecond(-1),
      m_hasRequest(false), m_decisionSent(false) {
}

CIFOfflineStallLoginPrompt::~CIFOfflineStallLoginPrompt() {
}

bool CIFOfflineStallLoginPrompt::OnCreate(long ln)
{
    CIFMainFrame::OnCreate(ln);
    SetGWndSize(LOGIN_WIDTH, LOGIN_HEIGHT);
    StyleMainFrame(this, KmtGetText(L"UIIT_KMT_OFFLINE_STALL"), LOGIN_WIDTH);
    if (m_pCloseBtn) {
        m_pCloseBtn->ShowGWnd(false);
    }

    RECT bgRect = {0, 31, LOGIN_WIDTH, LOGIN_HEIGHT - 31};
    m_background = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), bgRect, ID_OFFLINE_LOGIN_BG, 0);
    if (m_background) {
        m_background->TB_Func_13("clientlibrary\\mall\\win_bg.ddj", 0, 0);
        m_background->SetClickable(false);
        m_background->ShowGWnd(true);
    }

    RECT headerRect = {0, 31, LOGIN_WIDTH, 45};
    m_header = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), headerRect, ID_OFFLINE_LOGIN_HEADER, 0);
    if (m_header) {
        m_header->TB_Func_13("clientlibrary\\mall\\header.ddj", 0, 0);
        m_header->SetClickable(false);
        m_header->ShowGWnd(true);
    }

    RECT messageRect = {20, 70, LOGIN_WIDTH - 40, 27};
    m_message = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), messageRect, ID_OFFLINE_LOGIN_MESSAGE, 0);
    StyleStatic(m_message, KmtGetText(L"UIIT_KMT_AN_OFFLINE_STALL_IS_RUNNING_FOR_THIS_CHARACTER"), COLOR_TEXT, CTextBoard::JUSTIFY_CENTER);

    RECT questionRect = {20, 101, LOGIN_WIDTH - 40, 27};
    m_question = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), questionRect, ID_OFFLINE_LOGIN_QUESTION, 0);
    StyleStatic(m_question, KmtGetText(L"UIIT_KMT_DISCONNECT_IT_AND_LOGIN_NORMALLY"), COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);

    RECT countdownRect = {20, 132, LOGIN_WIDTH - 40, 22};
    m_countdown = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), countdownRect, ID_OFFLINE_LOGIN_COUNTDOWN, 0);
    StyleStatic(m_countdown, L"", COLOR_WARNING, CTextBoard::JUSTIFY_CENTER);

    RECT disconnectRect = {98, 169, 112, 32};
    m_disconnectButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFOfflineStallLoginButton), disconnectRect, ID_OFFLINE_LOGIN_DISCONNECT, 0);
    StyleButton(m_disconnectButton, KmtGetText(L"UIIT_KMT_DISCONNECT"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    RECT cancelRect = {230, 169, 112, 32};
    m_cancelButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFOfflineStallLoginButton), cancelRect, ID_OFFLINE_LOGIN_CANCEL, 0);
    StyleButton(m_cancelButton, KmtGetText(L"UIIT_KMT_CANCEL"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    CenterWindow(this);
    ShowGWnd(false);
    return true;
}

void CIFOfflineStallLoginPrompt::ShowGWnd(bool visible)
{
    CIFMainFrame::ShowGWnd(visible);
    if (visible) {
        UpdateWindowPos();
        BringToFront();
    }
}

void CIFOfflineStallLoginPrompt::OnUpdate()
{
    CIFMainFrame::OnUpdate();
    if (!m_hasRequest || !IsVisible()) {
        return;
    }

    const DWORD elapsed = (GetTickCount() - m_requestTick) / 1000;
    if ((int)elapsed >= m_timeoutSeconds) {
        SendDecision(false);
        return;
    }
    UpdateCountdown();
}

undefined1 CIFOfflineStallLoginPrompt::OnCloseWnd()
{
    if (m_hasRequest) {
        SendDecision(false);
    } else {
        ShowGWnd(false);
    }
    return true;
}

void CIFOfflineStallLoginPrompt::Open(DWORD token, BYTE timeoutSeconds,
                                      const wchar_t* characterName, int actionButtonId)
{
    if (token == 0) {
        return;
    }

    m_token = token;
    m_timeoutSeconds = timeoutSeconds > 0 ? timeoutSeconds : 5;
    if (m_timeoutSeconds > 30) m_timeoutSeconds = 30;
    m_actionButtonId = actionButtonId;
    m_requestTick = GetTickCount();
    m_lastDisplayedSecond = -1;
    m_hasRequest = true;
    m_decisionSent = false;

    wchar_t text[256];
    swprintf(text, 256, KmtGetText(L"UIIT_KMT_TEXT_IS_CURRENTLY_RUNNING_AN_OFFLINE_STALL"),
             characterName && characterName[0]
                 ? characterName
                 : KmtGetText(L"UIIT_KMT_THIS_CHARACTER"));
    if (m_message) m_message->SetText(text);
    if (m_disconnectButton) m_disconnectButton->SetEnabledState(true);
    if (m_cancelButton) m_cancelButton->SetEnabledState(true);
    SetActionButtonEnabled(false);
    UpdateCountdown();
    ShowGWnd(true);
}

void CIFOfflineStallLoginPrompt::ResetPrompt()
{
    m_hasRequest = false;
    m_decisionSent = false;
    m_token = 0;
    m_lastDisplayedSecond = -1;
    ShowGWnd(false);
}

void CIFOfflineStallLoginPrompt::HandleActionButton(int buttonId)
{
    if (buttonId == ID_OFFLINE_LOGIN_DISCONNECT) {
        OnDisconnect();
    } else if (buttonId == ID_OFFLINE_LOGIN_CANCEL) {
        OnCancel();
    }
}

void CIFOfflineStallLoginPrompt::OnDisconnect() {
    SendDecision(true);
}

void CIFOfflineStallLoginPrompt::OnCancel() {
    SendDecision(false);
}

void CIFOfflineStallLoginPrompt::SendDecision(bool disconnectOfflineStall)
{
    if (!m_hasRequest || m_decisionSent) {
        return;
    }

    m_decisionSent = true;
    m_hasRequest = false;
    // Keep the title-screen buttons textured while the decision is in flight.
    // m_decisionSent prevents duplicate packets without using the broken
    // white disabled rendering path on this client build.
    if (m_disconnectButton) m_disconnectButton->SetEnabledState(true);
    if (m_cancelButton) m_cancelButton->SetEnabledState(true);

    CMsgStreamBuffer packet(OFFLINE_STALL_LOGIN_DECISION_OPCODE);
    packet << m_token;
    packet << byte(disconnectOfflineStall ? 1 : 0);
    SendMsg(packet);

    if (disconnectOfflineStall) {
        if (m_countdown) {
            m_countdown->SetText(KmtGetText(L"UIIT_KMT_DISCONNECTING_OFFLINE_STALL"));
            m_countdown->m_FontTexture.SetColor(COLOR_ACTIVE);
        }
    } else {
        SetActionButtonEnabled(true);
        ShowGWnd(false);
    }
}

void CIFOfflineStallLoginPrompt::UpdateWindowPos() {
    CenterWindow(this);
}

void CIFOfflineStallLoginPrompt::UpdateCountdown()
{
    if (!m_countdown) return;
    int remaining = m_timeoutSeconds - (int)((GetTickCount() - m_requestTick) / 1000);
    if (remaining < 0) remaining = 0;
    if (remaining == m_lastDisplayedSecond) return;
    m_lastDisplayedSecond = remaining;

    wchar_t text[80];
    swprintf(text, 80, KmtGetText(L"UIIT_KMT_CONFIRM_WITHIN_VALUE_SECONDS"), remaining);
    m_countdown->SetText(text);
    m_countdown->m_FontTexture.SetColor(COLOR_WARNING);
}

void CIFOfflineStallLoginPrompt::SetActionButtonEnabled(bool enabled)
{
    // The filter already holds the login request until this prompt is
    // answered, so disabling the native Connect/Play button is unnecessary.
    // Several vSRO clients do not ship a valid disabled texture for those
    // controls and render them as a solid white rectangle.
    (void)enabled;
}

CIFOfflineStallLoginButton::CIFOfflineStallLoginButton() {
}

CIFOfflineStallLoginButton::~CIFOfflineStallLoginButton() {
}

int CIFOfflineStallLoginButton::OnMouseLeftUp(int a1, int x, int y)
{
    CGWndBase* parent = GetParentControl();
    if (parent && parent->IsKindOf(GFX_RUNTIME_CLASS(CIFOfflineStallLoginPrompt))) {
        // Title and character-selection scenes do not reliably forward the
        // native button command to a dynamically-created child message map.
        static_cast<CIFOfflineStallLoginPrompt*>(parent)->HandleActionButton(UniqueID());
        return 0;
    }

    return CIFButton::OnMouseLeftUp(a1, x, y);
}

void OfflineStall_CreateLoginPrompt(CGWnd* owner)
{
    if (!owner || owner->GetGuiFromList<CIFOfflineStallLoginPrompt>(OFFLINE_STALL_LOGIN_PROMPT_ID)) {
        return;
    }

    RECT rect = {0, 0, LOGIN_WIDTH, LOGIN_HEIGHT};
    CIFOfflineStallLoginPrompt* prompt = (CIFOfflineStallLoginPrompt*)CGWnd::CreateInstance(
        owner, GFX_RUNTIME_CLASS(CIFOfflineStallLoginPrompt), rect, OFFLINE_STALL_LOGIN_PROMPT_ID, 0);
    if (prompt) {
        prompt->ShowGWnd(false);
    }
}

void OfflineStall_OpenLoginPrompt(CGWnd* owner, DWORD token, BYTE timeoutSeconds,
                                  const std::n_string& characterName, int actionButtonId)
{
    // The server challenge is authoritative. It may arrive before the optional
    // settings tail has been parsed on a freshly connected client.
    if (!owner) {
        return;
    }

    OfflineStall_CreateLoginPrompt(owner);
    CIFOfflineStallLoginPrompt* prompt =
        owner->GetGuiFromList<CIFOfflineStallLoginPrompt>(OFFLINE_STALL_LOGIN_PROMPT_ID);
    if (!prompt) return;

    std::n_wstring wideName = TO_NWSTRING(characterName);
    prompt->Open(token, timeoutSeconds, wideName.c_str(), actionButtonId);
}

void OfflineStall_ResetLoginPrompt(CGWnd* owner)
{
    if (!owner) return;
    CIFOfflineStallLoginPrompt* prompt =
        owner->GetGuiFromList<CIFOfflineStallLoginPrompt>(OFFLINE_STALL_LOGIN_PROMPT_ID);
    if (prompt) {
        prompt->ResetPrompt();
    }
}

void OfflineStall_HandleActivationResult(DWORD nonce, bool success, const std::n_string& message)
{
    if (!g_pCGInterface) return;
    CIFOfflineStallConfirmWnd* window =
        g_pCGInterface->GetGuiFromList<CIFOfflineStallConfirmWnd>(OFFLINE_STALL_CONFIRM_WINDOW_ID);
    if (!window) return;

    std::n_wstring wideMessage = TO_NWSTRING(message);
    window->HandleResult(nonce, success, wideMessage.c_str());
}
