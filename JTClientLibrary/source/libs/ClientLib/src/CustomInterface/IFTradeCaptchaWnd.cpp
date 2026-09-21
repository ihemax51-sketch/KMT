#include "IFTradeCaptchaWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "GInterface.h"
#include "Game.h"
#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <string>

#define ID_TRADE_CAPTCHA_BG 10
#define ID_TRADE_CAPTCHA_HEADER 11
#define ID_TRADE_CAPTCHA_PANEL 12
#define ID_TRADE_CAPTCHA_LOCK 13
#define ID_TRADE_CAPTCHA_SEAL_TITLE 14
#define ID_TRADE_CAPTCHA_TIMER 15
#define ID_TRADE_CAPTCHA_CAPTION 16
#define ID_TRADE_CAPTCHA_DIGIT_BG 20
#define ID_TRADE_CAPTCHA_DIGIT 30
#define ID_TRADE_CAPTCHA_PROGRESS_TRACK 40
#define ID_TRADE_CAPTCHA_PROGRESS_FILL 41
#define ID_TRADE_CAPTCHA_ATTEMPTS 42
#define ID_TRADE_CAPTCHA_INPUT_LABEL 50
#define ID_TRADE_CAPTCHA_INPUT_BG 51
#define ID_TRADE_CAPTCHA_INPUT 52
#define ID_TRADE_CAPTCHA_STATUS 53
#define ID_TRADE_CAPTCHA_VERIFY 60
#define ID_TRADE_CAPTCHA_CANCEL 61

namespace {
const int WINDOW_WIDTH = 520;
const int WINDOW_HEIGHT = 326;
const int PROGRESS_X = 68;
const int PROGRESS_Y = 202;
const int PROGRESS_WIDTH = 384;
const int PROGRESS_HEIGHT = 10;

const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 83);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 245, 242, 232);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 185, 184, 174);
const D3DCOLOR COLOR_SELECTED = D3DCOLOR_ARGB(255, 132, 225, 221);
const D3DCOLOR COLOR_ACTIVE = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARNING = D3DCOLOR_ARGB(255, 255, 132, 118);

void StyleStatic(CIFStatic* control, const wchar_t* text, D3DCOLOR color,
                 CTextBoard::eJustifyHorizontal justify, int fontIndex)
{
    if (!control) {
        return;
    }
    control->SetText(text ? text : L"");
    control->SetFont(theApp.GetFont(fontIndex));
    control->m_FontTexture.SetColor(color);
    control->JustifyHorizontal(justify);
    control->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    control->ShowGWnd(true);
    control->BringToFront();
}

void StyleButton(CIFButton* button, const wchar_t* text)
{
    if (!button) {
        return;
    }
    button->TB_Func_13("clientlibrary\\mall\\mall_pre_big_button.ddj", 1, 1);
    button->SetText(text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->ShowGWnd(true);
    button->BringToFront();
}

CIFStatic* CreateSurface(CIFMainFrame* owner, int id, int x, int y, int width,
                         int height, const char* texture)
{
    RECT rect = {x, y, width, height};
    CIFStatic* surface = (CIFStatic*)CGWnd::CreateInstance(
        owner, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
    if (surface) {
        surface->TB_Func_13(texture, 0, 0);
        surface->SetClickable(false);
        surface->ShowGWnd(true);
    }
    return surface;
}
}

GFX_IMPLEMENT_DYNCREATE(CIFTradeCaptchaWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFTradeCaptchaWnd, CIFMainFrame)
                    ONG_COMMAND(ID_TRADE_CAPTCHA_VERIFY, &CIFTradeCaptchaWnd::OnVerify)
                    ONG_COMMAND(ID_TRADE_CAPTCHA_CANCEL, &CIFTradeCaptchaWnd::OnCancel)
GFX_END_MESSAGE_MAP()

CIFTradeCaptchaWnd::CIFTradeCaptchaWnd()
    : m_background(0), m_headerBackground(0), m_challengePanel(0), m_lockIcon(0),
      m_sealTitle(0), m_timerLabel(0), m_captionLabel(0), m_progressTrack(0),
      m_progressFill(0), m_attemptsLabel(0), m_inputLabel(0), m_inputBackground(0),
      m_inputEdit(0), m_statusLabel(0), m_verifyButton(0), m_cancelButton(0),
      m_challengeTick(0), m_timeoutSeconds(0), m_lastRemainingSeconds(-1),
      m_attemptsRemaining(0), m_active(false)
{
    for (int i = 0; i < 4; ++i) {
        m_digitBackgrounds[i] = 0;
        m_digitLabels[i] = 0;
    }
}

CIFTradeCaptchaWnd::~CIFTradeCaptchaWnd()
{
}

bool CIFTradeCaptchaWnd::OnCreate(long ln)
{
    CIFMainFrame::OnCreate(ln);
    TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    SetText(KmtGetText(L"UIIT_KMT_SECURE_TRADE_AUTHORIZATION"));

    m_background = CreateSurface(this, ID_TRADE_CAPTCHA_BG, 0, 31,
        WINDOW_WIDTH, WINDOW_HEIGHT - 31, "clientlibrary\\mall\\win_bg.ddj");
    m_headerBackground = CreateSurface(this, ID_TRADE_CAPTCHA_HEADER, 0, 31,
        WINDOW_WIDTH, 45, "clientlibrary\\mall\\header.ddj");
    m_challengePanel = CreateSurface(this, ID_TRADE_CAPTCHA_PANEL, 20, 82,
        WINDOW_WIDTH - 40, 142, "clientlibrary\\mall\\mall_pre_start.ddj");
    m_lockIcon = CreateSurface(this, ID_TRADE_CAPTCHA_LOCK, 44, 135, 32, 32,
        "clientlibrary\\slotup\\com_item_lock.ddj");

    RECT sealTitleRect = {44, 88, 282, 22};
    m_sealTitle = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), sealTitleRect, ID_TRADE_CAPTCHA_SEAL_TITLE, 0);
    StyleStatic(m_sealTitle, KmtGetText(L"UIIT_KMT_TRADE_AUTHORIZATION_SEAL"), COLOR_ACCENT,
        CTextBoard::JUSTIFY_LEFT, 0);

    RECT timerRect = {326, 88, 150, 22};
    m_timerLabel = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), timerRect, ID_TRADE_CAPTCHA_TIMER, 0);
    StyleStatic(m_timerLabel, KmtGetText(L"UIIT_KMT_EXPIRES_01_00"), COLOR_ACTIVE,
        CTextBoard::JUSTIFY_RIGHT, 0);

    RECT captionRect = {88, 111, 344, 18};
    m_captionLabel = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), captionRect, ID_TRADE_CAPTCHA_CAPTION, 0);
    StyleStatic(m_captionLabel, KmtGetText(L"UIIT_KMT_GOODS_RELEASE_CODE"), COLOR_MUTED,
        CTextBoard::JUSTIFY_CENTER, 0);

    for (int i = 0; i < 4; ++i) {
        const int x = 112 + (i * 74);
        m_digitBackgrounds[i] = CreateSurface(this, ID_TRADE_CAPTCHA_DIGIT_BG + i,
            x, 134, 62, 50, "clientlibrary\\common\\com_box.ddj");
        RECT digitRect = {x + 2, 137, 58, 44};
        m_digitLabels[i] = (CIFStatic*)CreateInstance(
            this, GFX_RUNTIME_CLASS(CIFStatic), digitRect, ID_TRADE_CAPTCHA_DIGIT + i, 0);
        StyleStatic(m_digitLabels[i], L"-", COLOR_SELECTED,
            CTextBoard::JUSTIFY_CENTER, 4);
    }

    m_progressTrack = CreateSurface(this, ID_TRADE_CAPTCHA_PROGRESS_TRACK,
        PROGRESS_X, PROGRESS_Y, PROGRESS_WIDTH, PROGRESS_HEIGHT,
        "clientlibrary\\title\\title_list.ddj");
    m_progressFill = CreateSurface(this, ID_TRADE_CAPTCHA_PROGRESS_FILL,
        PROGRESS_X, PROGRESS_Y, PROGRESS_WIDTH, PROGRESS_HEIGHT,
        "clientlibrary\\mall\\gage.ddj");
    if (m_progressFill) {
        m_progressFill->BringToFront();
    }

    RECT attemptsRect = {340, 184, 112, 18};
    m_attemptsLabel = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), attemptsRect, ID_TRADE_CAPTCHA_ATTEMPTS, 0);
    StyleStatic(m_attemptsLabel, L"", COLOR_LABEL, CTextBoard::JUSTIFY_RIGHT, 0);

    RECT inputLabelRect = {38, 240, 112, 32};
    m_inputLabel = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), inputLabelRect, ID_TRADE_CAPTCHA_INPUT_LABEL, 0);
    StyleStatic(m_inputLabel, KmtGetText(L"UIIT_KMT_ENTER_SEAL"), COLOR_LABEL,
        CTextBoard::JUSTIFY_LEFT, 0);

    m_inputBackground = CreateSurface(this, ID_TRADE_CAPTCHA_INPUT_BG,
        150, 240, 186, 32, "interface\\exchange\\exc_box.ddj");
    RECT inputRect = {160, 246, 166, 20};
    m_inputEdit = (CIFEdit*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFEdit), inputRect, ID_TRADE_CAPTCHA_INPUT, 0);
    if (m_inputEdit) {
        m_inputEdit->SetMaxLength(4);
        m_inputEdit->SetTextmode(166);
        m_inputEdit->SetFont(theApp.GetFont(0));
        m_inputEdit->m_FontTexture.SetColor(COLOR_TEXT);
        m_inputEdit->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
        m_inputEdit->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        m_inputEdit->ShowGWnd(true);
        m_inputEdit->BringToFront();
    }

    RECT verifyRect = {366, 240, 112, 32};
    m_verifyButton = (CIFButton*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFButton), verifyRect, ID_TRADE_CAPTCHA_VERIFY, 0);
    StyleButton(m_verifyButton, KmtGetText(L"UIIT_KMT_VERIFY"));

    RECT statusRect = {38, 280, 298, 24};
    m_statusLabel = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_TRADE_CAPTCHA_STATUS, 0);
    StyleStatic(m_statusLabel, KmtGetText(L"UIIT_KMT_AWAITING_AUTHORIZATION"), COLOR_MUTED,
        CTextBoard::JUSTIFY_LEFT, 0);

    RECT cancelRect = {366, 278, 112, 32};
    m_cancelButton = (CIFButton*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFButton), cancelRect, ID_TRADE_CAPTCHA_CANCEL, 0);
    StyleButton(m_cancelButton, KmtGetText(L"UIIT_KMT_CANCEL"));

    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_LABEL);
        m_pTitleText->BringToFront();
    }
    if (m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
        m_pCloseBtn->SetGWndSize(28, 32);
        m_pCloseBtn->MoveGWnd(WINDOW_WIDTH - 36, 2);
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }

    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFTradeCaptchaWnd::ShowGWnd(bool visible)
{
    CIFMainFrame::ShowGWnd(visible);
    if (visible) {
        UpdateWindowPos();
        BringToFront();
        FocusInput();
    }
}

void CIFTradeCaptchaWnd::OnUpdate()
{
    CIFMainFrame::OnUpdate();
    if (IsVisible() && m_active) {
        UpdateCountdown(false);
    }
}

undefined1 CIFTradeCaptchaWnd::OnCloseWnd()
{
    OnCancel();
    return true;
}

void CIFTradeCaptchaWnd::OpenChallenge(
    int code, int timeoutSeconds, int attemptsRemaining, bool retry)
{
    if (timeoutSeconds < 1) {
        timeoutSeconds = 1;
    }
    m_timeoutSeconds = timeoutSeconds;
    m_attemptsRemaining = attemptsRemaining;
    m_challengeTick = GetTickCount();
    m_lastRemainingSeconds = -1;
    m_active = true;

    UpdateChallengeDigits(code);
    ResetInput();
    if (m_verifyButton) {
        m_verifyButton->SetEnabledState(true);
    }

    if (m_attemptsLabel) {
        if (m_attemptsRemaining > 0) {
            wchar_t attempts[96];
            swprintf(
                attempts,
                96,
                KmtGetText(
                    m_attemptsRemaining == 1
                        ? L"UIIT_KMT_VALUE_ATTEMPT_LEFT"
                        : L"UIIT_KMT_VALUE_ATTEMPTS_LEFT"),
                m_attemptsRemaining);
            m_attemptsLabel->SetText(attempts);
        } else {
            m_attemptsLabel->SetText(L"");
        }
    }

    SetStatus(retry ? KmtGetText(L"UIIT_KMT_INCORRECT_SEAL_A_NEW_CODE_WAS_ISSUED")
                    : KmtGetText(L"UIIT_KMT_AWAITING_AUTHORIZATION"),
              retry ? COLOR_WARNING : COLOR_MUTED);
    UpdateCountdown(true);
    ShowGWnd(true);
    FocusInput();
}

bool CIFTradeCaptchaWnd::HandleKeyboardInput(UINT virtualKey)
{
    if (!IsVisible() || !m_active || !m_inputEdit) {
        return false;
    }

    wchar_t digit = 0;
    if (virtualKey >= '0' && virtualKey <= '9') {
        digit = (wchar_t)virtualKey;
    } else if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_NUMPAD9) {
        digit = (wchar_t)(L'0' + (virtualKey - VK_NUMPAD0));
    }

    std::n_wstring value = m_inputEdit->GetCurrentText();
    if (digit != 0) {
        if (value.length() < 4) {
            value += digit;
            m_inputEdit->SetText(value.c_str());
            m_inputEdit->SetCurrentIndex((unsigned int)value.length());
            SetStatus(KmtGetText(L"UIIT_KMT_AWAITING_AUTHORIZATION"), COLOR_MUTED);
        }
        FocusInput();
        return true;
    }

    if (virtualKey == VK_BACK) {
        if (!value.empty()) {
            value.erase(value.length() - 1, 1);
            m_inputEdit->SetText(value.c_str());
            m_inputEdit->SetCurrentIndex((unsigned int)value.length());
        }
        FocusInput();
        return true;
    }

    if (virtualKey == VK_DELETE) {
        ResetInput();
        FocusInput();
        return true;
    }

    return false;
}

void CIFTradeCaptchaWnd::SubmitFromKeyboard()
{
    if (IsVisible()) {
        OnVerify();
    }
}

void CIFTradeCaptchaWnd::CancelFromKeyboard()
{
    if (IsVisible()) {
        OnCancel();
    }
}

void CIFTradeCaptchaWnd::OnVerify()
{
    if (!m_active) {
        return;
    }
    UpdateCountdown(false);
    if (!m_active) {
        return;
    }

    int submittedCode = 0;
    if (!TryReadCode(submittedCode)) {
        SetStatus(KmtGetText(L"UIIT_KMT_ENTER_THE_COMPLETE_FOUR_DIGIT_SEAL"), COLOR_WARNING);
        if (m_inputEdit) {
            m_inputEdit->SetFocus_MAYBE();
        }
        return;
    }

    CMsgStreamBuffer packet(0x169A);
    packet << (byte)30;
    packet << submittedCode;
    SendMsg(packet);

    m_active = false;
    if (m_verifyButton) {
        m_verifyButton->SetEnabledState(false);
    }
    ShowGWnd(false);
}

void CIFTradeCaptchaWnd::OnCancel()
{
    m_active = false;
    ResetInput();
    ShowGWnd(false);
}

void CIFTradeCaptchaWnd::UpdateWindowPos()
{
    int screenWidth = 1024;
    int screenHeight = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        screenWidth = g_CGame->GetRes().res->width;
        screenHeight = g_CGame->GetRes().res->height;
    }

    int x = (screenWidth - WINDOW_WIDTH) / 2;
    int y = ((screenHeight - WINDOW_HEIGHT) / 2) + 35;
    if (x < 8) x = 8;
    if (y < 20) y = 20;
    if (y + WINDOW_HEIGHT > screenHeight - 20) y = screenHeight - WINDOW_HEIGHT - 20;
    if (y < 0) y = 0;
    MoveGWnd(x, y);
}

void CIFTradeCaptchaWnd::UpdateChallengeDigits(int code)
{
    if (code < 0) code = 0;
    std::wstringstream stream;
    stream << std::setw(4) << std::setfill(L'0') << (code % 10000);
    const std::wstring digits = stream.str();
    for (int i = 0; i < 4; ++i) {
        if (m_digitLabels[i]) {
            wchar_t value[2] = {digits[i], 0};
            m_digitLabels[i]->SetText(value);
        }
    }
}

void CIFTradeCaptchaWnd::UpdateCountdown(bool force)
{
    if (m_timeoutSeconds <= 0) return;
    const DWORD elapsedMs = GetTickCount() - m_challengeTick;
    const DWORD totalMs = (DWORD)m_timeoutSeconds * 1000;
    const DWORD remainingMs = elapsedMs >= totalMs ? 0 : totalMs - elapsedMs;
    const int remainingSeconds = (int)((remainingMs + 999) / 1000);

    if (force || remainingSeconds != m_lastRemainingSeconds) {
        m_lastRemainingSeconds = remainingSeconds;
        if (m_timerLabel) {
            wchar_t timer[64];
            swprintf(
                timer,
                64,
                KmtGetText(L"UIIT_KMT_EXPIRES_02D_02D"),
                remainingSeconds / 60,
                remainingSeconds % 60);
            m_timerLabel->SetText(timer);
            m_timerLabel->m_FontTexture.SetColor(
                remainingSeconds <= 10 ? COLOR_WARNING : COLOR_ACTIVE);
        }
        if (m_progressFill) {
            int width = (int)(((__int64)PROGRESS_WIDTH * remainingMs) / totalMs);
            if (width > 0) {
                m_progressFill->SetGWndSize(width, PROGRESS_HEIGHT);
                m_progressFill->ShowGWnd(true);
            } else {
                m_progressFill->ShowGWnd(false);
            }
        }
    }

    if (remainingMs == 0 && m_active) {
        m_active = false;
        if (m_verifyButton) m_verifyButton->SetEnabledState(false);
        SetStatus(KmtGetText(L"UIIT_KMT_SEAL_EXPIRED_SELL_THE_GOODS_AGAIN"), COLOR_WARNING);
    }
}

void CIFTradeCaptchaWnd::SetStatus(const wchar_t* message, D3DCOLOR color)
{
    if (m_statusLabel) {
        m_statusLabel->SetText(message ? message : L"");
        m_statusLabel->m_FontTexture.SetColor(color);
    }
}

void CIFTradeCaptchaWnd::FocusInput()
{
    if (!m_inputEdit || !m_active) {
        return;
    }

    m_inputEdit->SetFocus_MAYBE();
    if (m_inputEdit->m_hEditBoxWnd && IsWindow(m_inputEdit->m_hEditBoxWnd)) {
        ::SetFocus(m_inputEdit->m_hEditBoxWnd);
    }
}

void CIFTradeCaptchaWnd::ResetInput()
{
    if (m_inputEdit) {
        m_inputEdit->SetText(L"");
        m_inputEdit->SetCurrentIndex(0);
    }
}

bool CIFTradeCaptchaWnd::TryReadCode(int& code) const
{
    code = 0;
    if (!m_inputEdit) return false;
    const std::n_wstring& text = m_inputEdit->GetCurrentText();
    if (text.length() != 4) return false;
    for (std::n_wstring::const_iterator it = text.begin(); it != text.end(); ++it) {
        if (*it < L'0' || *it > L'9') return false;
        code = (code * 10) + (*it - L'0');
    }
    return true;
}
