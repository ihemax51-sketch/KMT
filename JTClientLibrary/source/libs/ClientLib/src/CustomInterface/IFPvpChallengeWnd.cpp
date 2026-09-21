#include "IFPvpChallengeWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "CustomData/CustomSettingManager.h"
#include "GInterface.h"
#include "Game.h"
#include <BSLib/multibyte.h>
#include <Windows.h>
#include <cstdio>
#include <string>

#define ID_PVP_PANEL_FRAME 10
#define ID_PVP_BG 11
#define ID_PVP_HEADER_TEXT 12
#define ID_PVP_TARGET_LABEL 20
#define ID_PVP_GOLD_LABEL 21
#define ID_PVP_HELP_TEXT 22
#define ID_PVP_TARGET_EDIT 30
#define ID_PVP_GOLD_EDIT 31
#define ID_PVP_STATUS 40
#define ID_PVP_CHALLENGE 50

#define ID_PVP_ANSWER_PANEL_FRAME 110
#define ID_PVP_ANSWER_BG 111
#define ID_PVP_ANSWER_HEADER_TEXT 112
#define ID_PVP_ANSWER_MESSAGE 120
#define ID_PVP_ANSWER_WAGER 121
#define ID_PVP_ANSWER_COUNTDOWN 122
#define ID_PVP_ACCEPT 130
#define ID_PVP_DECLINE 131

namespace {
const int CHALLENGE_WIDTH = 430;
const int CHALLENGE_HEIGHT = 300;
const int ANSWER_WIDTH = 390;
const int ANSWER_HEIGHT = 230;
const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 117);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 238, 215, 168);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 255, 255, 255);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 198, 190, 174);
const D3DCOLOR COLOR_OK = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARN = D3DCOLOR_ARGB(255, 255, 132, 118);

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
    CIFNormalTile* tile = (CIFNormalTile*)CGWnd::CreateInstance(owner, GFX_RUNTIME_CLASS(CIFNormalTile), rect, id, 0);
    if (tile) {
        tile->TB_Func_13(texture, 0, 1);
        tile->SetClickable(false);
        tile->ShowGWnd(true);
    }
    return tile;
}

}

GFX_IMPLEMENT_DYNCREATE(CIFPvpChallengeWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFPvpChallengeWnd, CIFMainFrame)
                    ONG_COMMAND(ID_PVP_CHALLENGE, &CIFPvpChallengeWnd::OnChallenge)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFPvpChallengeAnswerWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFPvpChallengeAnswerWnd, CIFMainFrame)
                    ONG_COMMAND(ID_PVP_ACCEPT, &CIFPvpChallengeAnswerWnd::OnAccept)
                    ONG_COMMAND(ID_PVP_DECLINE, &CIFPvpChallengeAnswerWnd::OnDecline)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFPvpChallengeGuide, CIFDecoratedStatic)

CIFPvpChallengeWnd::CIFPvpChallengeWnd()
    : m_panelFrame(0), m_background(0), m_targetLabel(0), m_goldLabel(0), m_statusLabel(0),
      m_targetEdit(0), m_goldEdit(0), m_challengeButton(0),
      m_requestTick(0), m_waitingForResponse(false) {
}

CIFPvpChallengeWnd::~CIFPvpChallengeWnd() {
}

bool CIFPvpChallengeWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_PVP_CHALLENGE"));
    SetGWndSize(CHALLENGE_WIDTH, CHALLENGE_HEIGHT);

    m_panelFrame = CreateInsetFrame(
        this, ID_PVP_PANEL_FRAME, 7, 37, 416, 220);
    m_background = CreateTile(
        this, ID_PVP_BG, 20, 49, 390, 195,
        "interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj");

    CIFStatic* headerText = CreateLabel(
        ID_PVP_HEADER_TEXT, 28, 52, 374,
        KmtGetText(L"UIIT_KMT_CREATE_A_PVP_WAGER_CHALLENGE"));
    StyleStatic(
        headerText,
        KmtGetText(L"UIIT_KMT_CREATE_A_PVP_WAGER_CHALLENGE"),
        COLOR_ACCENT,
        CTextBoard::JUSTIFY_LEFT);

    CIFStatic* helpText = CreateLabel(
        ID_PVP_HELP_TEXT,
        28,
        76,
        374,
        KmtGetText(L"UIIT_KMT_ENTER_THE_OPPONENT_CHARACTER_AND_THE_GOLD_WAGER_AMOUNT"));
    if (helpText) {
        helpText->m_FontTexture.SetColor(COLOR_MUTED);
        helpText->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    }

    m_targetLabel = CreateLabel(
        ID_PVP_TARGET_LABEL, 38, 117, 126,
        KmtGetText(L"UIIT_KMT_OPPONENT_CHARACTER"));
    m_goldLabel = CreateLabel(
        ID_PVP_GOLD_LABEL, 38, 153, 126,
        KmtGetText(L"UIIT_KMT_GOLD_WAGER_AMOUNT"));
    m_targetEdit = CreateEditBox(ID_PVP_TARGET_EDIT, 172, 112, 220, 32);
    m_goldEdit = CreateEditBox(ID_PVP_GOLD_EDIT, 172, 148, 220, 19);

    RECT statusRect = {34, 218, 362, 18};
    m_statusLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_PVP_STATUS, 0);
    StyleStatic(m_statusLabel, L"", COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

    RECT challengeRect = {(CHALLENGE_WIDTH - 96) / 2, 266, 96, 24};
    m_challengeButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), challengeRect, ID_PVP_CHALLENGE, 0);
    StyleButton(m_challengeButton, KmtGetText(L"UIIT_KMT_CHALLENGE"));

    if (!m_panelFrame || !m_background || !headerText || !helpText ||
        !m_targetLabel || !m_goldLabel || !m_targetEdit || !m_goldEdit ||
        !m_statusLabel || !m_challengeButton) {
        return false;
    }
    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_TEXT);
    }
    AlignCloseButton(this, CHALLENGE_WIDTH);

    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFPvpChallengeWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        BringToFront();
        if (!m_waitingForResponse) {
            SetStatus(L"", COLOR_MUTED);
        }
    }
}

void CIFPvpChallengeWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();

    if (m_waitingForResponse && GetTickCount() - m_requestTick > 15000) {
        m_waitingForResponse = false;
        if (m_challengeButton) {
            m_challengeButton->SetEnabledState(true);
        }
        SetStatus(KmtGetText(L"UIIT_KMT_REQUEST_TIMED_OUT_PLEASE_TRY_AGAIN"), COLOR_WARN);
    }
}

undefined1 CIFPvpChallengeWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFPvpChallengeWnd::HandleStatus(bool success, const wchar_t* message) {
    m_waitingForResponse = false;
    if (m_challengeButton) {
        m_challengeButton->SetEnabledState(true);
    }
    SetStatus(message ? message : L"", success ? COLOR_OK : COLOR_WARN);
    if (success) {
        ClearRequestFields();
        ShowGWnd(false);
    }
}

void CIFPvpChallengeWnd::OnChallenge() {
    if (m_waitingForResponse) {
        return;
    }
    if (!m_Settings->EnablePvpChallenge) {
        SetStatus(KmtGetText(L"UIIT_KMT_PVP_CHALLENGE_IS_DISABLED"), COLOR_WARN);
        return;
    }

    std::string target = m_targetEdit ? TO_STRING(m_targetEdit->GetCurrentText()) : std::string();
    while (!target.empty() && target[0] == ' ') {
        target.erase(target.begin());
    }
    while (!target.empty() && target[target.length() - 1] == ' ') {
        target.erase(target.length() - 1);
    }

    if (target.empty()) {
        SetStatus(KmtGetText(L"UIIT_KMT_ENTER_OPPONENT_NAME"), COLOR_WARN);
        return;
    }

    __int64 wagerGold = 0;
    if (!TryParseGold(wagerGold)) {
        SetStatus(KmtGetText(L"UIIT_KMT_ENTER_A_VALID_WAGER"), COLOR_WARN);
        return;
    }

    CMsgStreamBuffer packet(0x169A);
    packet << byte(35);
    packet << std::n_string(target.c_str());
    packet << wagerGold;
    SendMsg(packet);

    m_waitingForResponse = true;
    m_requestTick = GetTickCount();
    if (m_challengeButton) {
        m_challengeButton->SetEnabledState(false);
    }
    SetStatus(KmtGetText(L"UIIT_KMT_CHALLENGE_REQUEST_SENT"), COLOR_LABEL);
}

void CIFPvpChallengeWnd::UpdateWindowPos() {
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int x = (width - CHALLENGE_WIDTH) / 2;
    int y = ((height - CHALLENGE_HEIGHT) / 2) + 10;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (y + CHALLENGE_HEIGHT > height) {
        y = height - CHALLENGE_HEIGHT;
    }
    if (y < 0) {
        y = 0;
    }
    MoveGWnd(x, y);
    AlignCloseButton(this, CHALLENGE_WIDTH);
}

void CIFPvpChallengeWnd::ClearRequestFields() {
    if (m_targetEdit) {
        m_targetEdit->SetText(L"");
        m_targetEdit->SetCurrentIndex(0);
    }
    if (m_goldEdit) {
        m_goldEdit->SetText(L"");
        m_goldEdit->SetCurrentIndex(0);
    }
    SetStatus(L"", COLOR_MUTED);
}

CIFEdit* CIFPvpChallengeWnd::CreateEditBox(int id, int x, int y, int width, int maxLength) {
    RECT backgroundRect = {x, y + 2, width, 24};
    CIFStatic* background = (CIFStatic*)CreateInstance(
        this, GFX_RUNTIME_CLASS(CIFStatic), backgroundRect, id + 1000, 0);
    if (background) {
        background->TB_Func_13(
            "interface\\ifcommon\\com_grad_gage_form.ddj", 0, 0);
        background->SetClickable(false);
        background->ShowGWnd(true);
    }

    RECT editRect = {x + 7, y + 5, width - 14, 18};
    CIFEdit* edit = (CIFEdit*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFEdit), editRect, id, 0);
    if (edit) {
        edit->SetMaxLength(maxLength);
        edit->SetTextmode(edit->GetSize().width - 8);
        edit->SetFont(theApp.GetFont(0));
        edit->m_FontTexture.SetColor(COLOR_TEXT);
        edit->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
        edit->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        edit->ShowGWnd(true);
        edit->BringToFront();
    }

    return background ? edit : 0;
}

CIFStatic* CIFPvpChallengeWnd::CreateLabel(int id, int x, int y, int width, const wchar_t* text) {
    RECT rect = {x, y, width, 20};
    CIFStatic* label = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
    StyleStatic(label, text, COLOR_LABEL, CTextBoard::JUSTIFY_LEFT);
    return label;
}

bool CIFPvpChallengeWnd::TryParseGold(__int64& value) const {
    value = 0;
    std::string text = m_goldEdit ? TO_STRING(m_goldEdit->GetCurrentText()) : std::string();
    if (text.empty()) {
        return false;
    }

    for (std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
        char ch = *it;
        if (ch == ',' || ch == ' ') {
            continue;
        }
        if (ch < '0' || ch > '9') {
            return false;
        }

        int digit = ch - '0';
        if (value > (9223372036854775807LL - digit) / 10) {
            return false;
        }
        value = (value * 10) + digit;
    }

    return value > 0;
}

void CIFPvpChallengeWnd::SetStatus(const wchar_t* message, D3DCOLOR color) {
    if (!m_statusLabel) {
        return;
    }

    m_statusLabel->SetText(message ? message : L"");
    m_statusLabel->m_FontTexture.SetColor(color);
}

CIFPvpChallengeAnswerWnd::CIFPvpChallengeAnswerWnd()
    : m_panelFrame(0), m_background(0), m_messageLabel(0), m_wagerLabel(0), m_countdownLabel(0),
      m_acceptButton(0), m_declineButton(0), m_matchId(0), m_wagerGold(0),
      m_requestTick(0), m_timeoutSeconds(0), m_hasRequest(false) {
}

CIFPvpChallengeAnswerWnd::~CIFPvpChallengeAnswerWnd() {
}

bool CIFPvpChallengeAnswerWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_PVP_CHALLENGE"));
    SetGWndSize(ANSWER_WIDTH, ANSWER_HEIGHT);

    m_panelFrame = CreateInsetFrame(
        this, ID_PVP_ANSWER_PANEL_FRAME, 7, 37, 376, 147);
    m_background = CreateTile(
        this, ID_PVP_ANSWER_BG, 20, 49, 350, 122,
        "interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj");

    RECT headerRect = {28, 52, 334, 20};
    CIFStatic* answerHeader = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), headerRect, ID_PVP_ANSWER_HEADER_TEXT, 0);
    StyleStatic(answerHeader, KmtGetText(L"UIIT_KMT_INCOMING_PVP_CHALLENGE"), COLOR_ACCENT, CTextBoard::JUSTIFY_LEFT);

    RECT msgRect = {28, 80, 334, 20};
    m_messageLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), msgRect, ID_PVP_ANSWER_MESSAGE, 0);
    StyleStatic(m_messageLabel, L"", COLOR_TEXT, CTextBoard::JUSTIFY_CENTER);

    RECT wagerRect = {28, 106, 334, 22};
    m_wagerLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), wagerRect, ID_PVP_ANSWER_WAGER, 0);
    StyleStatic(m_wagerLabel, L"", COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);

    RECT countdownRect = {28, 136, 334, 18};
    m_countdownLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), countdownRect, ID_PVP_ANSWER_COUNTDOWN, 0);
    StyleStatic(m_countdownLabel, L"", COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

    RECT acceptRect = {115, 196, 76, 24};
    m_acceptButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), acceptRect, ID_PVP_ACCEPT, 0);
    StyleButton(m_acceptButton, KmtGetText(L"UIIT_KMT_ACCEPT"));

    RECT declineRect = {199, 196, 76, 24};
    m_declineButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), declineRect, ID_PVP_DECLINE, 0);
    StyleButton(m_declineButton, KmtGetText(L"UIIT_KMT_DECLINE"));

    if (!m_panelFrame || !m_background || !answerHeader ||
        !m_messageLabel || !m_wagerLabel || !m_countdownLabel ||
        !m_acceptButton || !m_declineButton) {
        return false;
    }
    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_TEXT);
    }
    AlignCloseButton(this, ANSWER_WIDTH);

    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFPvpChallengeAnswerWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        BringToFront();
    }
}

void CIFPvpChallengeAnswerWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();
    if (!m_hasRequest || !IsVisible()) {
        return;
    }

    DWORD elapsedSeconds = (GetTickCount() - m_requestTick) / 1000;
    if ((int)elapsedSeconds >= m_timeoutSeconds) {
        m_hasRequest = false;
        ShowGWnd(false);
        return;
    }

    UpdateCountdown();
}

undefined1 CIFPvpChallengeAnswerWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFPvpChallengeAnswerWnd::OpenRequest(__int64 matchId, const wchar_t* challengerName, __int64 wagerGold, int timeoutSeconds) {
    m_matchId = matchId;
    m_wagerGold = wagerGold;
    m_timeoutSeconds = timeoutSeconds > 0 ? timeoutSeconds : 60;
    m_requestTick = GetTickCount();
    m_hasRequest = true;

    wchar_t message[160];
    swprintf(message, 160, KmtGetText(L"UIIT_KMT_TEXT_CHALLENGED_YOU_TO_PVP"), challengerName ? challengerName : KmtGetText(L"UIIT_KMT_SOMEONE"));
    if (m_messageLabel) {
        m_messageLabel->SetText(message);
    }

    wchar_t wager[128];
    swprintf(wager, 128, KmtGetText(L"UIIT_KMT_WAGER_VALUE64_GOLD"), wagerGold);
    if (m_wagerLabel) {
        m_wagerLabel->SetText(wager);
    }

    if (m_acceptButton) {
        m_acceptButton->SetEnabledState(true);
    }
    if (m_declineButton) {
        m_declineButton->SetEnabledState(true);
    }

    UpdateCountdown();
    ShowGWnd(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
}

void CIFPvpChallengeAnswerWnd::OnAccept() {
    SendAnswer(true);
}

void CIFPvpChallengeAnswerWnd::OnDecline() {
    SendAnswer(false);
}

void CIFPvpChallengeAnswerWnd::SendAnswer(bool accepted) {
    if (!m_hasRequest) {
        ShowGWnd(false);
        return;
    }

    CMsgStreamBuffer packet(0x169A);
    packet << byte(36);
    packet << m_matchId;
    packet << byte(accepted ? 1 : 0);
    SendMsg(packet);

    m_hasRequest = false;
    if (m_acceptButton) {
        m_acceptButton->SetEnabledState(false);
    }
    if (m_declineButton) {
        m_declineButton->SetEnabledState(false);
    }
    ShowGWnd(false);
}

void CIFPvpChallengeAnswerWnd::UpdateWindowPos() {
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int x = (width - ANSWER_WIDTH) / 2;
    int y = ((height - ANSWER_HEIGHT) / 2) + 10;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (y + ANSWER_HEIGHT > height) {
        y = height - ANSWER_HEIGHT;
    }
    if (y < 0) {
        y = 0;
    }
    MoveGWnd(x, y);
    AlignCloseButton(this, ANSWER_WIDTH);
}

void CIFPvpChallengeAnswerWnd::UpdateCountdown() {
    if (!m_countdownLabel) {
        return;
    }

    int elapsedSeconds = (int)((GetTickCount() - m_requestTick) / 1000);
    int remaining = m_timeoutSeconds - elapsedSeconds;
    if (remaining < 0) {
        remaining = 0;
    }

    wchar_t text[80];
    swprintf(text, 80, KmtGetText(L"UIIT_KMT_EXPIRES_IN_VALUE_SECONDS"), remaining);
    m_countdownLabel->SetText(text);
}

bool CIFPvpChallengeGuide::OnCreate(long ln) {
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_pvp_challenge_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_pvp_challenge_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_PVP_CHALLENGE"));
        label->SetStyleThingy(TOOLTIP);
    }
    return true;
}

int CIFPvpChallengeGuide::OnMouseLeftUp(int a1, int x, int y) {
    if (!g_pCGInterface) {
        return 0;
    }

    CIFPvpChallengeWnd* window = g_pCGInterface->GetGuiFromList<CIFPvpChallengeWnd>(PVP_CHALLENGE_WINDOW_ID);
    if (window) {
        window->ShowGWnd(!window->IsVisible());
        CGEffSoundBody::get()->PlaySound(window->IsVisible() ? L"snd_window_open" : L"snd_window_close");
    }
    return 0;
}

void CIFPvpChallengeGuide::OnCIFReady() {
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
