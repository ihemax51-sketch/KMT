#include "IFLoginRegisterWnd.h"

#include "Game.h"
#include "PSTitle.h"
#include "ClientNet/MsgStreamBuffer.h"
#include <BSLib/multibyte.h>

#define ID_REG_PANEL_FRAME 10
#define ID_REG_BG 11
#define ID_REG_SECTION_TITLE 12
#define ID_REG_DESCRIPTION 13
#define ID_REG_HINT 14
#define ID_REG_LABEL_USER 20
#define ID_REG_LABEL_PASSWORD 21
#define ID_REG_LABEL_CONFIRM 22
#define ID_REG_USER 30
#define ID_REG_PASSWORD 31
#define ID_REG_CONFIRM 32
#define ID_REG_SUBMIT 40
#define ID_REG_CANCEL 41

namespace {
const int WINDOW_WIDTH = 430;
const int WINDOW_HEIGHT = 300;
const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 117);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 238, 215, 168);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 255, 255, 255);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 198, 190, 174);
const size_t PASSWORD_MAX_LENGTH = 32;

void StyleButton(CIFButton* button, const wchar_t* text) {
    if (!button) {
        return;
    }

    button->TB_Func_13("interface\\ifcommon\\com_button.ddj", 1, 1);
    button->SetText(text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->ShowGWnd(true);
    button->BringToFront();
}
}

GFX_IMPLEMENT_DYNCREATE(CIFLoginRegisterWnd, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFLoginRegisterWnd, CIFMainFrame)
                    ONG_COMMAND(ID_REG_SUBMIT, &CIFLoginRegisterWnd::OnRegister)
                    ONG_COMMAND(ID_REG_CANCEL, &CIFLoginRegisterWnd::OnCancel)
GFX_END_MESSAGE_MAP()

static CIFLoginRegisterWnd* g_pLoginRegisterWnd = 0;

GFX_IMPLEMENT_DYNCREATE(CIFLoginRegisterButton, CIFButton)

CIFLoginRegisterWnd::CIFLoginRegisterWnd() {
    m_pPanelFrame = 0;
    m_pBackground = 0;
    m_pSectionTitle = 0;
    m_pDescription = 0;
    m_pHint = 0;
    m_pUserId = 0;
    m_pPassword = 0;
    m_pConfirmPassword = 0;
    m_pRegisterBtn = 0;
    m_pCancelBtn = 0;
}

CIFLoginRegisterWnd::~CIFLoginRegisterWnd() {
    if (g_pLoginRegisterWnd == this) {
        g_pLoginRegisterWnd = 0;
    }
}

bool CIFLoginRegisterWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);
    g_pLoginRegisterWnd = this;

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_ACCOUNT_REGISTER"));
    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);

    RECT panelFrameRect = {7, 37, 416, 220};
    m_pPanelFrame = (CIFFrame*)CreateInstance(
        this,
        GFX_RUNTIME_CLASS(CIFFrame),
        panelFrameRect,
        ID_REG_PANEL_FRAME,
        0);
    if (m_pPanelFrame) {
        m_pPanelFrame->SetFrameTexture(
            std::n_string("interface\\inventory\\int_window_"));
        m_pPanelFrame->SetClickable(false);
        m_pPanelFrame->ShowGWnd(true);
    }

    RECT bgRect = {20, 49, 390, 195};
    m_pBackground = (CIFNormalTile*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFNormalTile), bgRect, ID_REG_BG, 0);
    if (m_pBackground) {
        m_pBackground->TB_Func_13("interface\\ifcommon\\bg_tile\\com_bg_tile_b.ddj", 0, 1);
        m_pBackground->SetClickable(false);
        m_pBackground->ShowGWnd(true);
    }

    m_pSectionTitle = CreateLabel(
        ID_REG_SECTION_TITLE,
        28,
        52,
        374,
        20,
        KmtGetText(L"UIIT_KMT_ACCOUNT_REGISTER"),
        COLOR_ACCENT,
        CTextBoard::JUSTIFY_LEFT);
    m_pDescription = CreateLabel(
        ID_REG_DESCRIPTION,
        28,
        76,
        374,
        20,
        KmtGetText(L"UIIT_KMT_ID_MUST_BE_4_16_CHARACTERS"),
        COLOR_MUTED,
        CTextBoard::JUSTIFY_CENTER);
    m_pHint = CreateLabel(
        ID_REG_HINT,
        34,
        218,
        362,
        18,
        KmtGetText(L"UIIT_KMT_PASSWORD_MUST_BE_6_32_CHARACTERS"),
        COLOR_MUTED,
        CTextBoard::JUSTIFY_CENTER);

    CIFStatic* userLabel = CreateLabel(
        ID_REG_LABEL_USER,
        38,
        117,
        126,
        18,
        KmtGetText(L"UIIT_KMT_ID"),
        COLOR_LABEL,
        CTextBoard::JUSTIFY_LEFT);
    CIFStatic* passwordLabel = CreateLabel(
        ID_REG_LABEL_PASSWORD,
        38,
        153,
        126,
        18,
        KmtGetText(L"UIIT_KMT_PASSWORD_E7CF3EF4"),
        COLOR_LABEL,
        CTextBoard::JUSTIFY_LEFT);
    CIFStatic* confirmLabel = CreateLabel(
        ID_REG_LABEL_CONFIRM,
        38,
        189,
        126,
        18,
        KmtGetText(L"UIIT_KMT_CONFIRM"),
        COLOR_LABEL,
        CTextBoard::JUSTIFY_LEFT);

    m_pUserId = CreateEditBox(ID_REG_USER, 172, 112, 220, 16);
    m_pPassword = CreateEditBox(ID_REG_PASSWORD, 172, 148, 220, 32);
    m_pConfirmPassword = CreateEditBox(ID_REG_CONFIRM, 172, 184, 220, 32);
    if (m_pPassword) {
        m_pPassword->SetStyleThingy(PASSWORD_MASKED);
    }
    if (m_pConfirmPassword) {
        m_pConfirmPassword->SetStyleThingy(PASSWORD_MASKED);
    }

    RECT registerRect = {135, 266, 76, 24};
    m_pRegisterBtn = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), registerRect, ID_REG_SUBMIT, 0);
    StyleButton(m_pRegisterBtn, KmtGetText(L"UIIT_KMT_REGISTER"));

    RECT cancelRect = {219, 266, 76, 24};
    m_pCancelBtn = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), cancelRect, ID_REG_CANCEL, 0);
    StyleButton(m_pCancelBtn, KmtGetText(L"UIIT_KMT_CANCEL"));

    if (!m_pPanelFrame || !m_pBackground || !m_pSectionTitle ||
        !m_pDescription || !m_pHint || !userLabel || !passwordLabel ||
        !confirmLabel || !m_pUserId || !m_pPassword ||
        !m_pConfirmPassword || !m_pRegisterBtn || !m_pCancelBtn) {
        return false;
    }

    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_TEXT);
    }
    if (m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13(
            "interface\\ifcommon\\com_windowclose.ddj", 0, 0);
        m_pCloseBtn->SetGWndSize(16, 16);
        m_pCloseBtn->MoveGWnd(
            GetPos().x + WINDOW_WIDTH - 26,
            GetPos().y + 9);
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }

    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFLoginRegisterWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        if (m_pUserId) {
            m_pUserId->SetFocus_MAYBE();
            if (m_pUserId->m_hEditBoxWnd &&
                IsWindow(m_pUserId->m_hEditBoxWnd)) {
                ::SetFocus(m_pUserId->m_hEditBoxWnd);
            }
        }
        BringToFront();
    }
}

undefined1 CIFLoginRegisterWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFLoginRegisterWnd::FocusNextField(
    CIFEdit* current,
    bool backwards) {
    CIFEdit* fields[3] = {
        m_pUserId,
        m_pPassword,
        m_pConfirmPassword
    };

    int currentIndex = 0;
    for (int i = 0; i < 3; ++i) {
        if (fields[i] == current) {
            currentIndex = i;
            break;
        }
    }

    const int nextIndex = backwards
        ? (currentIndex + 2) % 3
        : (currentIndex + 1) % 3;
    CIFEdit* next = fields[nextIndex];
    if (!next) {
        return;
    }

    next->SetFocus_MAYBE();
    if (next->m_hEditBoxWnd && IsWindow(next->m_hEditBoxWnd)) {
        ::SetFocus(next->m_hEditBoxWnd);
    }
}

bool CIFLoginRegisterWnd::HandlePasswordInput(
    CIFEdit* edit,
    UINT message,
    WPARAM key) {
    std::n_wstring* password = PasswordForEdit(edit);
    if (!password || !edit) {
        return false;
    }

    unsigned int cursor = edit->GetCurrentIndex();
    if (cursor > password->length()) {
        cursor = (unsigned int)password->length();
    }

    if (message == WM_CHAR) {
        if (key == VK_BACK) {
            if (cursor > 0) {
                password->erase(cursor - 1, 1);
                --cursor;
            }
        } else if (key >= 0x21 && key <= 0x7E) {
            if (password->length() < PASSWORD_MAX_LENGTH) {
                password->insert(cursor, 1, (wchar_t)key);
                ++cursor;
            }
        } else {
            return true;
        }

        RefreshPasswordEdit(edit, *password, cursor);
        return true;
    }

    if (message == WM_KEYDOWN && key == VK_DELETE) {
        if (cursor < password->length()) {
            password->erase(cursor, 1);
        }
        RefreshPasswordEdit(edit, *password, cursor);
        return true;
    }

    if (message == WM_PASTE) {
        PastePasswordFromClipboard(edit, *password);
        return true;
    }

    return false;
}

void CIFLoginRegisterWnd::HandleRegisterResponse(bool success, const wchar_t* message) {
    ShowTitleMessage(message, success ? 0xFF00FF00 : 0xFFFF671D);
    if (success) {
        ClearInputs();
        OnCloseWnd();
    }
}

CIFLoginRegisterWnd* CIFLoginRegisterWnd::GetActiveWindow() {
    return g_pLoginRegisterWnd;
}

void CIFLoginRegisterWnd::OnRegister() {
    std::n_string userId = GetAsciiText(m_pUserId);
    std::n_string password = GetAsciiPassword(m_passwordValue);
    std::n_string confirm = GetAsciiPassword(m_confirmPasswordValue);

    if (userId.length() < 4 || userId.length() > 16) {
        ShowTitleMessage(KmtGetText(L"UIIT_KMT_ID_MUST_BE_4_16_CHARACTERS"), 0xFFFF671D);
        return;
    }

    if (password.length() < 6 || password.length() > 32) {
        ShowTitleMessage(KmtGetText(L"UIIT_KMT_PASSWORD_MUST_BE_6_32_CHARACTERS"), 0xFFFF671D);
        return;
    }

    if (password != confirm) {
        ShowTitleMessage(KmtGetText(L"UIIT_KMT_PASSWORD_CONFIRMATION_DOES_NOT_MATCH"), 0xFFFF671D);
        return;
    }

    CMsgStreamBuffer buf(0x166A);
    buf << userId << password << confirm;
    SendMsg(buf);

    ShowTitleMessage(KmtGetText(L"UIIT_KMT_REGISTER_REQUEST_SENT"), 0xFFFFD953);
}

void CIFLoginRegisterWnd::OnCancel() {
    OnCloseWnd();
}

void CIFLoginRegisterWnd::ClearInputs() {
    m_passwordValue.clear();
    m_confirmPasswordValue.clear();

    if (m_pUserId) {
        m_pUserId->SetText(L"");
        m_pUserId->SetCurrentIndex(0);
    }
    if (m_pPassword) {
        m_pPassword->SetText(L"");
        m_pPassword->SetCurrentIndex(0);
    }
    if (m_pConfirmPassword) {
        m_pConfirmPassword->SetText(L"");
        m_pConfirmPassword->SetCurrentIndex(0);
    }
}

void CIFLoginRegisterWnd::UpdateWindowPos() {
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
    if (y + WINDOW_HEIGHT > height) {
        y = height - WINDOW_HEIGHT;
    }
    if (y < 0) {
        y = 0;
    }

    MoveGWnd(x, y);
    if (m_pCloseBtn) {
        m_pCloseBtn->MoveGWnd(
            GetPos().x + WINDOW_WIDTH - 26,
            GetPos().y + 9);
        m_pCloseBtn->BringToFront();
    }
}

void CIFLoginRegisterWnd::ShowTitleMessage(const wchar_t* message, D3DCOLOR color) {
    if (GetParentControl() && GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CPSTitle))) {
        ((CPSTitle*)GetParentControl())->ShowMessage(message, color);
    }
}

std::n_string CIFLoginRegisterWnd::GetAsciiText(CIFEdit* edit) const {
    if (!edit) {
        return std::n_string();
    }

    std::string value = TO_STRING(edit->GetCurrentText());
    return std::n_string(value.c_str());
}

std::n_string CIFLoginRegisterWnd::GetAsciiPassword(
    const std::n_wstring& password) const {
    std::string value = TO_STRING(password);
    return std::n_string(value.c_str());
}

std::n_wstring* CIFLoginRegisterWnd::PasswordForEdit(CIFEdit* edit) {
    if (edit == m_pPassword) {
        return &m_passwordValue;
    }
    if (edit == m_pConfirmPassword) {
        return &m_confirmPasswordValue;
    }
    return 0;
}

void CIFLoginRegisterWnd::RefreshPasswordEdit(
    CIFEdit* edit,
    const std::n_wstring& password,
    unsigned int cursor) {
    if (!edit) {
        return;
    }

    edit->SetText(password.c_str());
    edit->SetCurrentIndex(cursor);
}

void CIFLoginRegisterWnd::PastePasswordFromClipboard(
    CIFEdit* edit,
    std::n_wstring& password) {
    if (!edit || !IsClipboardFormatAvailable(CF_UNICODETEXT) ||
        !OpenClipboard(edit->m_hEditBoxWnd)) {
        return;
    }

    std::n_wstring clipboardText;
    HANDLE clipboardData = GetClipboardData(CF_UNICODETEXT);
    if (clipboardData) {
        const wchar_t* text =
            (const wchar_t*)GlobalLock(clipboardData);
        if (text) {
            clipboardText = text;
            GlobalUnlock(clipboardData);
        }
    }
    CloseClipboard();

    unsigned int cursor = edit->GetCurrentIndex();
    if (cursor > password.length()) {
        cursor = (unsigned int)password.length();
    }

    for (size_t i = 0;
         i < clipboardText.length() &&
         password.length() < PASSWORD_MAX_LENGTH;
         ++i) {
        const wchar_t character = clipboardText[i];
        if (character < 0x21 || character > 0x7E) {
            continue;
        }

        password.insert(cursor, 1, character);
        ++cursor;
    }

    RefreshPasswordEdit(edit, password, cursor);
}

CIFEdit* CIFLoginRegisterWnd::CreateEditBox(int id, int x, int y, int width, int maxLength) {
    RECT backgroundRect = {x, y + 2, width, 24};
    CIFStatic* background = (CIFStatic*)CreateInstance(
        this,
        GFX_RUNTIME_CLASS(CIFStatic),
        backgroundRect,
        id + 100,
        0);
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

CIFStatic* CIFLoginRegisterWnd::CreateLabel(
    int id,
    int x,
    int y,
    int width,
    int height,
    const wchar_t* text,
    D3DCOLOR color,
    CTextBoard::eJustifyHorizontal justify) {
    RECT labelRect = {x, y, width, height};
    CIFStatic* label = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), labelRect, id, 0);
    if (label) {
        label->SetText(text);
        label->SetFont(theApp.GetFont(0));
        label->m_FontTexture.SetColor(color);
        label->JustifyHorizontal(justify);
        label->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
        label->ShowGWnd(true);
        label->BringToFront();
    }

    return label;
}

CIFLoginRegisterButton::CIFLoginRegisterButton() {
}

CIFLoginRegisterButton::~CIFLoginRegisterButton() {
}

int CIFLoginRegisterButton::OnMouseLeftUp(int a1, int x, int y) {
    // Prefer the button's live title scene.  The former scene can be torn
    // down while the player returns to change accounts.
    if (GetParentControl() && GetParentControl()->IsSame(GFX_RUNTIME_CLASS(CPSTitle))) {
        ((CPSTitle*)GetParentControl())->ShowLoginRegisterWindow();
        return 0;
    }

    if (g_pLoginRegisterWnd) {
        g_pLoginRegisterWnd->ShowGWnd(true);
        return 0;
    }

    return CIFButton::OnMouseLeftUp(a1, x, y);
}
