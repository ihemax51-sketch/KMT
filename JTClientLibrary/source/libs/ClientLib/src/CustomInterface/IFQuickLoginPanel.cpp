#include "CustomInterface/IFQuickLoginPanel.h"

#include "PSTitle.h"
#include "Game.h"
#include "ClientNet/MsgStreamBuffer.h"
#include "Hwid/HWIDGenerator.h"
#include "IFEdit.h"
#include <BSLib/multibyte.h>
#include <Windows.h>
#include <wincrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>

#pragma comment(lib, "crypt32.lib")

namespace {
    const int ID_QL_PANEL_BG = 6100;
    const int ID_QL_TITLE = 6101;
    const int ID_QL_EMPTY = 6102;
    const int ID_QL_SAVE = 6103;
    const int ID_QL_MANAGE = 6104;
    const int ID_QL_BACK = 6105;
    const int ID_QL_USER_LABEL = 6106;
    const int ID_QL_PASS_LABEL = 6107;
    const int ID_QL_USER_BG = 6108;
    const int ID_QL_USER_EDIT = 6109;
    const int ID_QL_PASS_BG = 6110;
    const int ID_QL_PASS_EDIT = 6111;
    const int ID_QL_CHAR_LABEL = 6112;
    const int ID_QL_CHAR_BG = 6113;
    const int ID_QL_CHAR_EDIT = 6114;
    const int ID_QL_ROW_BASE = 6120;
    const int ID_QL_ROW_STEP = 10;
    const int QL_ROW_COUNT = 3;
    const int ID_QL_SAVE_CAPTION = 6150;
    const int ID_QL_MANAGE_CAPTION = 6151;
    const int ID_QL_BACK_CAPTION = 6152;
    const int ID_QL_ROW_CAPTION_BASE = 6160;
    const int ID_QL_ROW_CAPTION_STEP = 10;

    const unsigned short OP_CLIENT_CREATE_QUICK_LOGIN_TOKEN = 0x1670;
    const unsigned short OP_CLIENT_QUICK_LOGIN = 0x1672;
    const unsigned short OP_CLIENT_REVOKE_QUICK_LOGIN_TOKEN = 0x1674;

    struct QuickLoginEntry {
        std::n_string username;
        std::n_string protectedTokenHex;
        std::n_string displayName;
        std::n_string lastUsed;
        std::n_string characterName;
    };

    struct QuickLoginPanelState {
        CPSTitle* title;
        CIFStatic* bg;
        CIFStatic* titleText;
        CIFStatic* emptyText;
        CIFStatic* userLabel;
        CIFStatic* passLabel;
        CIFStatic* charLabel;
        CIFStatic* userBg;
        CIFStatic* passBg;
        CIFStatic* charBg;
        CIFEdit* userEdit;
        CIFEdit* passEdit;
        CIFEdit* charEdit;
        CIFQuickLoginButton* saveButton;
        CIFQuickLoginButton* manageButton;
        CIFQuickLoginButton* backButton;
        CIFStatic* saveCaption;
        CIFStatic* manageCaption;
        CIFStatic* backCaption;
        CIFStatic* rowName[QL_ROW_COUNT];
        CIFStatic* rowDate[QL_ROW_COUNT];
        CIFQuickLoginButton* rowButton[QL_ROW_COUNT];
        CIFStatic* rowCaption[QL_ROW_COUNT];
        std::vector<QuickLoginEntry> entries;
        std::n_string pendingCreateCharacterName;
        std::n_string pendingQuickLoginCharacterName;
        std::n_string pendingAutoCharacterName;
        bool manageMode;
        bool addMode;
        bool enabled;
    };

    QuickLoginPanelState g_state = {};

    void ResetPanelControlPointers() {
        g_state.title = NULL;
        g_state.bg = NULL;
        g_state.titleText = NULL;
        g_state.emptyText = NULL;
        g_state.userLabel = NULL;
        g_state.passLabel = NULL;
        g_state.charLabel = NULL;
        g_state.userBg = NULL;
        g_state.passBg = NULL;
        g_state.charBg = NULL;
        g_state.userEdit = NULL;
        g_state.passEdit = NULL;
        g_state.charEdit = NULL;
        g_state.saveButton = NULL;
        g_state.manageButton = NULL;
        g_state.backButton = NULL;
        g_state.saveCaption = NULL;
        g_state.manageCaption = NULL;
        g_state.backCaption = NULL;
        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            g_state.rowName[i] = NULL;
            g_state.rowDate[i] = NULL;
            g_state.rowButton[i] = NULL;
            g_state.rowCaption[i] = NULL;
        }
    }

    bool HasCompletePanel() {
        if (!g_state.title || !g_state.bg || !g_state.titleText || !g_state.emptyText ||
            !g_state.userLabel || !g_state.passLabel || !g_state.charLabel ||
            !g_state.userBg || !g_state.passBg || !g_state.charBg ||
            !g_state.userEdit || !g_state.passEdit || !g_state.charEdit ||
            !g_state.saveButton || !g_state.manageButton || !g_state.backButton) {
            return false;
        }

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            if (!g_state.rowName[i] || !g_state.rowDate[i] || !g_state.rowButton[i])
                return false;
        }
        return true;
    }

    bool BindPanelControls(CPSTitle* title) {
        if (!title) {
            return false;
        }

        // CPSTitle is kept alive when the player returns to the login screen
        // to use another account.  Its native refresh can rebuild the child
        // list, so resolve every control from the active title instead of
        // continuing with process-wide cached UI pointers.
        CIFStatic* bg = title->GetGuiFromList<CIFStatic>(ID_QL_PANEL_BG);
        CIFStatic* titleText = title->GetGuiFromList<CIFStatic>(ID_QL_TITLE);
        CIFStatic* emptyText = title->GetGuiFromList<CIFStatic>(ID_QL_EMPTY);
        CIFStatic* userLabel = title->GetGuiFromList<CIFStatic>(ID_QL_USER_LABEL);
        CIFStatic* passLabel = title->GetGuiFromList<CIFStatic>(ID_QL_PASS_LABEL);
        CIFStatic* charLabel = title->GetGuiFromList<CIFStatic>(ID_QL_CHAR_LABEL);
        CIFStatic* userBg = title->GetGuiFromList<CIFStatic>(ID_QL_USER_BG);
        CIFStatic* passBg = title->GetGuiFromList<CIFStatic>(ID_QL_PASS_BG);
        CIFStatic* charBg = title->GetGuiFromList<CIFStatic>(ID_QL_CHAR_BG);
        CIFEdit* userEdit = title->GetGuiFromList<CIFEdit>(ID_QL_USER_EDIT);
        CIFEdit* passEdit = title->GetGuiFromList<CIFEdit>(ID_QL_PASS_EDIT);
        CIFEdit* charEdit = title->GetGuiFromList<CIFEdit>(ID_QL_CHAR_EDIT);
        CIFQuickLoginButton* saveButton = title->GetGuiFromList<CIFQuickLoginButton>(ID_QL_SAVE);
        CIFQuickLoginButton* manageButton = title->GetGuiFromList<CIFQuickLoginButton>(ID_QL_MANAGE);
        CIFQuickLoginButton* backButton = title->GetGuiFromList<CIFQuickLoginButton>(ID_QL_BACK);
        CIFStatic* saveCaption = title->GetGuiFromList<CIFStatic>(ID_QL_SAVE_CAPTION);
        CIFStatic* manageCaption = title->GetGuiFromList<CIFStatic>(ID_QL_MANAGE_CAPTION);
        CIFStatic* backCaption = title->GetGuiFromList<CIFStatic>(ID_QL_BACK_CAPTION);
        CIFStatic* rowName[QL_ROW_COUNT] = {};
        CIFStatic* rowDate[QL_ROW_COUNT] = {};
        CIFQuickLoginButton* rowButton[QL_ROW_COUNT] = {};
        CIFStatic* rowCaption[QL_ROW_COUNT] = {};

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            const int base = ID_QL_ROW_BASE + i * ID_QL_ROW_STEP;
            rowName[i] = title->GetGuiFromList<CIFStatic>(base + 1);
            rowDate[i] = title->GetGuiFromList<CIFStatic>(base + 2);
            rowButton[i] = title->GetGuiFromList<CIFQuickLoginButton>(base);
            rowCaption[i] = title->GetGuiFromList<CIFStatic>(ID_QL_ROW_CAPTION_BASE + i * ID_QL_ROW_CAPTION_STEP);
        }

        if (!bg || !titleText || !emptyText || !userLabel || !passLabel || !charLabel ||
            !userBg || !passBg || !charBg || !userEdit || !passEdit || !charEdit ||
            !saveButton || !manageButton || !backButton) {
            return false;
        }

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            if (!rowName[i] || !rowDate[i] || !rowButton[i]) {
                return false;
            }
        }

        g_state.title = title;
        g_state.bg = bg;
        g_state.titleText = titleText;
        g_state.emptyText = emptyText;
        g_state.userLabel = userLabel;
        g_state.passLabel = passLabel;
        g_state.charLabel = charLabel;
        g_state.userBg = userBg;
        g_state.passBg = passBg;
        g_state.charBg = charBg;
        g_state.userEdit = userEdit;
        g_state.passEdit = passEdit;
        g_state.charEdit = charEdit;
        g_state.saveButton = saveButton;
        g_state.manageButton = manageButton;
        g_state.backButton = backButton;
        g_state.saveCaption = saveCaption;
        g_state.manageCaption = manageCaption;
        g_state.backCaption = backCaption;
        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            g_state.rowName[i] = rowName[i];
            g_state.rowDate[i] = rowDate[i];
            g_state.rowButton[i] = rowButton[i];
            g_state.rowCaption[i] = rowCaption[i];
        }
        return true;
    }

    std::n_string ToNString(const std::string& value) {
        return std::n_string(value.c_str());
    }

    std::string ToStdString(const std::n_string& value) {
        return std::string(value.c_str());
    }

    std::n_string GetNowString() {
        time_t now = time(NULL);
        tm localTime;
        localtime_s(&localTime, &now);
        char buffer[32] = {0};
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime);
        return std::n_string(buffer);
    }

    int GetUnixTime() {
        return static_cast<int>(time(NULL));
    }

    std::string GetStorePath() {
        char appData[MAX_PATH] = {0};
        DWORD len = GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
        std::string base = (len > 0 && len < MAX_PATH) ? appData : ".";
        std::string folder = base + "\\KMTGuard";
        CreateDirectoryA(folder.c_str(), NULL);
        std::string storePath = folder + "\\secure_quick_login.dat";
        std::string oldStorePath = base + "\\KMTGuard\\secure_quick_login.dat";
        if (GetFileAttributesA(storePath.c_str()) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesA(oldStorePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileA(oldStorePath.c_str(), storePath.c_str(), TRUE);
        }
        return storePath;
    }

    std::string BytesToHex(const BYTE* data, DWORD length) {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        for (DWORD i = 0; i < length; ++i)
            ss << std::setw(2) << static_cast<unsigned int>(data[i]);
        return ss.str();
    }

    bool HexToBytes(const std::string& hex, std::vector<BYTE>& out) {
        if ((hex.length() % 2) != 0)
            return false;
        out.clear();
        out.reserve(hex.length() / 2);
        for (size_t i = 0; i < hex.length(); i += 2) {
            unsigned int value = 0;
            std::stringstream ss;
            ss << std::hex << hex.substr(i, 2);
            ss >> value;
            if (ss.fail())
                return false;
            out.push_back(static_cast<BYTE>(value));
        }
        return true;
    }

    bool ProtectToken(const std::n_string& token, std::n_string& protectedHex) {
        DATA_BLOB in = {};
        DATA_BLOB out = {};
        std::string tokenString = ToStdString(token);
        in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(tokenString.data()));
        in.cbData = static_cast<DWORD>(tokenString.size());

        if (!CryptProtectData(&in, L"KMTGuard Secure Quick Login", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out))
            return false;

        protectedHex = ToNString(BytesToHex(out.pbData, out.cbData));
        LocalFree(out.pbData);
        return true;
    }

    bool UnprotectToken(const std::n_string& protectedHex, std::n_string& token) {
        std::vector<BYTE> encrypted;
        if (!HexToBytes(ToStdString(protectedHex), encrypted) || encrypted.empty())
            return false;

        DATA_BLOB in = {};
        DATA_BLOB out = {};
        in.pbData = &encrypted[0];
        in.cbData = static_cast<DWORD>(encrypted.size());

        if (!CryptUnprotectData(&in, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out))
            return false;

        std::string tokenString(reinterpret_cast<char*>(out.pbData), out.cbData);
        token = ToNString(tokenString);
        LocalFree(out.pbData);
        return true;
    }

    void LoadEntries() {
        g_state.entries.clear();
        std::ifstream file(GetStorePath().c_str());
        if (!file)
            return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;

            std::stringstream ss(line);
            std::string username;
            std::string protectedToken;
            std::string displayName;
            std::string lastUsed;
            std::string characterName;

            std::getline(ss, username, '\t');
            std::getline(ss, protectedToken, '\t');
            std::getline(ss, displayName, '\t');
            std::getline(ss, lastUsed, '\t');
            std::getline(ss, characterName, '\t');

            if (username.empty() || protectedToken.empty())
                continue;

            QuickLoginEntry entry;
            entry.username = ToNString(username);
            entry.protectedTokenHex = ToNString(protectedToken);
            entry.characterName = ToNString(characterName);
            if (!displayName.empty())
                entry.displayName = ToNString(displayName);
            else
                entry.displayName = ToNString(characterName.empty() ? username : characterName);
            entry.lastUsed = ToNString(lastUsed);
            g_state.entries.push_back(entry);
        }
    }

    void SaveEntries() {
        std::ofstream file(GetStorePath().c_str(), std::ios::trunc);
        if (!file)
            return;

        for (size_t i = 0; i < g_state.entries.size(); ++i) {
            const QuickLoginEntry& entry = g_state.entries[i];
            file << ToStdString(entry.username) << '\t'
                 << ToStdString(entry.protectedTokenHex) << '\t'
                 << ToStdString(entry.displayName) << '\t'
                 << ToStdString(entry.lastUsed) << '\t'
                 << ToStdString(entry.characterName) << '\n';
        }
    }

    QuickLoginEntry* FindEntry(const std::n_string& username, const std::n_string& characterName) {
        if (!characterName.empty()) {
            for (size_t i = 0; i < g_state.entries.size(); ++i) {
                if (!g_state.entries[i].characterName.empty() &&
                    _stricmp(g_state.entries[i].characterName.c_str(), characterName.c_str()) == 0)
                    return &g_state.entries[i];
            }
        }

        for (size_t i = 0; i < g_state.entries.size(); ++i) {
            if (g_state.entries[i].characterName.empty() &&
                _stricmp(g_state.entries[i].username.c_str(), username.c_str()) == 0)
                return &g_state.entries[i];
        }
        return NULL;
    }

    void RemoveEntryAt(int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(g_state.entries.size()))
            return;

        g_state.entries.erase(g_state.entries.begin() + rowIndex);
        SaveEntries();
    }

    std::n_string GetDeviceId() {
        HWIDGenerator generator;
        return ToNString(generator.GenerateHWID());
    }

    std::n_string GetClientVersion() {
        return std::n_string("KMTGuardKit");
    }

    CIFStatic* EnsureActionCaption(CPSTitle* title, int id) {
        if (!title) {
            return NULL;
        }

        CIFStatic* caption = title->GetGuiFromList<CIFStatic>(id);
        if (!caption) {
            RECT captionRect = {0, 0, 1, 1};
            caption = (CIFStatic*)CGWnd::CreateInstance(
                title,
                GFX_RUNTIME_CLASS(CIFStatic),
                captionRect,
                id,
                0);
            if (caption) {
                caption->SetClickable(false);
                caption->ShowGWnd(false);
            }
        }
        return caption;
    }

    void EnsureActionCaptions(CPSTitle* title) {
        if (!title) {
            return;
        }

        g_state.saveCaption = EnsureActionCaption(title, ID_QL_SAVE_CAPTION);
        g_state.manageCaption = EnsureActionCaption(title, ID_QL_MANAGE_CAPTION);
        g_state.backCaption = EnsureActionCaption(title, ID_QL_BACK_CAPTION);
        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            g_state.rowCaption[i] = EnsureActionCaption(
                title,
                ID_QL_ROW_CAPTION_BASE + i * ID_QL_ROW_CAPTION_STEP);
        }
    }

    void SyncActionCaption(
        CIFStatic* caption,
        CIFQuickLoginButton* button,
        const wchar_t* text) {
        if (!caption) {
            return;
        }
        if (!button) {
            caption->ShowGWnd(false);
            return;
        }

        // Keep the custom button as a click surface only.  The native
        // CIFStatic caption uses the same proven renderer as the Saved
        // Accounts, ID, PW and Char labels that survive account switching.
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
        caption->SetText(text);

        const bool visible = g_state.enabled && button->IsVisible();
        caption->ShowGWnd(visible);
        if (visible) {
            caption->BringToFront();
        }
    }

    void SyncActionCaptions() {
        if (!g_state.title) {
            return;
        }

        EnsureActionCaptions(g_state.title);
        SyncActionCaption(
            g_state.saveCaption,
            g_state.saveButton,
            g_state.addMode ? KmtGetText(L"UIIT_KMT_SAVE") : KmtGetText(L"UIIT_KMT_PLUS_SAVE"));
        SyncActionCaption(g_state.manageCaption, g_state.manageButton, KmtGetText(L"UIIT_KMT_MANAGE"));
        SyncActionCaption(g_state.backCaption, g_state.backButton, KmtGetText(L"UIIT_KMT_BACK"));
        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            SyncActionCaption(
                g_state.rowCaption[i],
                g_state.rowButton[i],
                g_state.manageMode ? KmtGetText(L"UIIT_KMT_DELETE") : KmtGetText(L"UIIT_KMT_PLAY"));
        }
    }

    CIFStatic* CreateStatic(CPSTitle* parent, int id, int x, int y, int w, int h, const wchar_t* text, D3DCOLOR color) {
        RECT rect = {x, y, w, h};
        CIFStatic* control = (CIFStatic*)CGWnd::CreateInstance(parent, GFX_RUNTIME_CLASS(CIFStatic), rect, id, 0);
        if (control) {
            control->SetText(text);
            control->m_FontTexture.SetColor(color);
            control->ShowGWnd(true);
        }
        return control;
    }

    CIFQuickLoginButton* CreateButton(CPSTitle* parent, int id, int x, int y, int w, int h, const wchar_t*) {
        RECT rect = {x, y, w, h};
        CIFQuickLoginButton* button = (CIFQuickLoginButton*)CGWnd::CreateInstance(parent, GFX_RUNTIME_CLASS(CIFQuickLoginButton), rect, id, 0);
        if (button) {
            button->TB_Func_13("interface\\outer\\button.ddj", 1, 1);
            button->SetText(L"");
            button->ShowGWnd(true);
        }
        return button;
    }

    CIFEdit* CreateEdit(CPSTitle* parent, CIFStatic** bgOut, int bgId, int editId, int x, int y, int width, int maxLength) {
        RECT bgRect = {x, y, width, 22};
        CIFStatic* bg = (CIFStatic*)CGWnd::CreateInstance(parent, GFX_RUNTIME_CLASS(CIFStatic), bgRect, bgId, 0);
        if (bg) {
            bg->TB_Func_13("interface\\ifcommon\\com_grad_gage_form.ddj", 0, 0);
            bg->ShowGWnd(true);
        }

        RECT editRect = {x + 4, y + 2, width - 8, 18};
        CIFEdit* edit = (CIFEdit*)CGWnd::CreateInstance(parent, GFX_RUNTIME_CLASS(CIFEdit), editRect, editId, 0);
        if (edit) {
            edit->SetMaxLength(maxLength);
            edit->SetTextmode(width - 8);
            edit->ShowGWnd(true);
        }

        if (bgOut)
            *bgOut = bg;
        return edit;
    }

    void PositionPanel() {
        if (!g_state.title || !g_state.bg)
            return;

        int x = 620;
        int y = 360;
        int frameX = 0;
        int frameY = 0;
        int frameWidth = 0;
        int frameHeight = 0;
        if (g_state.title->GetLoginFrameRect(frameX, frameY, frameWidth, frameHeight)) {
            x = frameX + frameWidth + 12;
            y = frameY;
        }

        if (g_CGame && g_CGame->GetRes().res) {
            int screenWidth = g_CGame->GetRes().res->width;
            if (x + 288 > screenWidth)
                x = screenWidth - 298;
        }

        g_state.bg->MoveGWnd(x, y);
        g_state.bg->ShowGWnd(true);
        g_state.titleText->MoveGWnd(x + 44, y + 24);
        g_state.emptyText->MoveGWnd(x + 42, y + 132);
        g_state.userLabel->MoveGWnd(x + 36, y + 58);
        g_state.passLabel->MoveGWnd(x + 36, y + 85);
        g_state.charLabel->MoveGWnd(x + 36, y + 112);
        g_state.userBg->MoveGWnd(x + 92, y + 55);
        g_state.passBg->MoveGWnd(x + 92, y + 82);
        g_state.charBg->MoveGWnd(x + 92, y + 109);
        g_state.userEdit->MoveGWnd(x + 96, y + 57);
        g_state.passEdit->MoveGWnd(x + 96, y + 84);
        g_state.charEdit->MoveGWnd(x + 96, y + 111);
        g_state.saveButton->MoveGWnd(x + 38, y + 144);
        g_state.manageButton->MoveGWnd(x + 150, y + 144);
        g_state.backButton->MoveGWnd(x + 184, y + 144);

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            int rowY = y + 56 + i * 30;
            g_state.rowName[i]->MoveGWnd(x + 38, rowY);
            g_state.rowDate[i]->MoveGWnd(x + 38, rowY + 13);
            g_state.rowButton[i]->MoveGWnd(x + 184, rowY - 1);
        }
    }

    void ShowPanelControls(bool visible) {
        if (g_state.bg)
            g_state.bg->ShowGWnd(visible);
        if (g_state.titleText)
            g_state.titleText->ShowGWnd(visible);
        if (g_state.emptyText)
            g_state.emptyText->ShowGWnd(false);
        if (g_state.userLabel)
            g_state.userLabel->ShowGWnd(false);
        if (g_state.passLabel)
            g_state.passLabel->ShowGWnd(false);
        if (g_state.charLabel)
            g_state.charLabel->ShowGWnd(false);
        if (g_state.userBg)
            g_state.userBg->ShowGWnd(false);
        if (g_state.passBg)
            g_state.passBg->ShowGWnd(false);
        if (g_state.charBg)
            g_state.charBg->ShowGWnd(false);
        if (g_state.userEdit)
            g_state.userEdit->ShowGWnd(false);
        if (g_state.passEdit)
            g_state.passEdit->ShowGWnd(false);
        if (g_state.charEdit)
            g_state.charEdit->ShowGWnd(false);
        if (g_state.saveButton)
            g_state.saveButton->ShowGWnd(false);
        if (g_state.manageButton)
            g_state.manageButton->ShowGWnd(false);
        if (g_state.backButton)
            g_state.backButton->ShowGWnd(false);
        if (g_state.saveCaption)
            g_state.saveCaption->ShowGWnd(false);
        if (g_state.manageCaption)
            g_state.manageCaption->ShowGWnd(false);
        if (g_state.backCaption)
            g_state.backCaption->ShowGWnd(false);

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            if (g_state.rowName[i])
                g_state.rowName[i]->ShowGWnd(false);
            if (g_state.rowDate[i])
                g_state.rowDate[i]->ShowGWnd(false);
            if (g_state.rowButton[i])
                g_state.rowButton[i]->ShowGWnd(false);
            if (g_state.rowCaption[i])
                g_state.rowCaption[i]->ShowGWnd(false);
        }
    }

    void RefreshRows() {
        if (!g_state.bg)
            return;

        if (!HasCompletePanel()) {
            ShowPanelControls(false);
            return;
        }

        if (!g_state.enabled) {
            ShowPanelControls(false);
            return;
        }

        g_state.bg->ShowGWnd(true);
        g_state.titleText->ShowGWnd(true);

        LoadEntries();
        const bool hasEntries = !g_state.entries.empty();
        if (!hasEntries) {
            g_state.addMode = true;
            g_state.manageMode = false;
        }

        const bool showForm = g_state.addMode;
        const bool showRows = hasEntries && !showForm;

        g_state.emptyText->ShowGWnd(!hasEntries && !showForm);
        g_state.userLabel->ShowGWnd(showForm);
        g_state.passLabel->ShowGWnd(showForm);
        g_state.charLabel->ShowGWnd(showForm);
        g_state.userBg->ShowGWnd(showForm);
        g_state.passBg->ShowGWnd(showForm);
        g_state.charBg->ShowGWnd(showForm);
        g_state.userEdit->ShowGWnd(showForm);
        g_state.passEdit->ShowGWnd(showForm);
        g_state.charEdit->ShowGWnd(showForm);
        g_state.saveButton->ShowGWnd(true);
        g_state.manageButton->ShowGWnd(showRows && !g_state.manageMode);
        g_state.backButton->ShowGWnd((showForm && hasEntries) || (showRows && g_state.manageMode));

        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            bool visible = showRows && i < static_cast<int>(g_state.entries.size());
            g_state.rowName[i]->ShowGWnd(visible);
            g_state.rowDate[i]->ShowGWnd(visible);
            g_state.rowButton[i]->ShowGWnd(visible);
            if (!visible)
                continue;

            const QuickLoginEntry& entry = g_state.entries[i];
            g_state.rowName[i]->SetText(TO_NWSTRING(ToStdString(entry.displayName)).c_str());
            g_state.rowDate[i]->SetText(TO_NWSTRING(ToStdString(entry.lastUsed)).c_str());
        }

        PositionPanel();
        SyncActionCaptions();
    }

    void EnsurePresentationText() {
        if (!HasCompletePanel() || !g_state.enabled)
            return;

        g_state.titleText->SetText(KmtGetText(L"UIIT_KMT_SAVED_ACCOUNTS"));
        g_state.emptyText->SetText(KmtGetText(L"UIIT_KMT_NO_SAVED_ACCOUNTS_YET"));
        g_state.userLabel->SetText(KmtGetText(L"UIIT_KMT_ID"));
        g_state.passLabel->SetText(KmtGetText(L"UIIT_KMT_PW"));
        g_state.charLabel->SetText(KmtGetText(L"UIIT_KMT_CHAR"));
        const bool showRows = !g_state.entries.empty() && !g_state.addMode;
        for (int i = 0; i < QL_ROW_COUNT; ++i) {
            if (!showRows || i >= static_cast<int>(g_state.entries.size()))
                continue;

            const QuickLoginEntry& entry = g_state.entries[i];
            g_state.rowName[i]->SetText(TO_NWSTRING(ToStdString(entry.displayName)).c_str());
            g_state.rowDate[i]->SetText(TO_NWSTRING(ToStdString(entry.lastUsed)).c_str());
        }

        SyncActionCaptions();
    }

    void ShowSavedAccountsList() {
        if (g_state.userEdit)
            g_state.userEdit->SetText(L"");
        if (g_state.passEdit)
            g_state.passEdit->SetText(L"");
        if (g_state.charEdit)
            g_state.charEdit->SetText(L"");

        LoadEntries();
        g_state.addMode = g_state.entries.empty();
        g_state.manageMode = false;
        RefreshRows();
    }

    std::n_string GetEditText(CPSTitle* title, int id) {
        return title->GetLoginEditText(id);
    }

    void SendCreateToken(CPSTitle* title) {
        std::n_string username;
        std::n_string password;
        std::n_string characterName;
        if (g_state.userEdit)
            username = std::n_string(TO_STRING(g_state.userEdit->GetCurrentText()).c_str());
        if (g_state.passEdit)
            password = std::n_string(TO_STRING(g_state.passEdit->GetCurrentText()).c_str());
        if (g_state.charEdit)
            characterName = std::n_string(TO_STRING(g_state.charEdit->GetCurrentText()).c_str());
        if (username.empty() || password.empty()) {
            title->ShowMessage(KmtGetText(L"UIIT_KMT_PLEASE_ENTER_USERNAME_AND_PASSWORD_FIRST"), 0xFFFF671D);
            return;
        }
        if (characterName.empty()) {
            title->ShowMessage(KmtGetText(L"UIIT_KMT_PLEASE_ENTER_CHARACTER_NAME_FIRST"), 0xFFFF671D);
            return;
        }

        g_state.pendingCreateCharacterName = characterName;
        CMsgStreamBuffer buf(OP_CLIENT_CREATE_QUICK_LOGIN_TOKEN);
        buf << username << password << GetDeviceId() << GetClientVersion() << GetUnixTime()
            << std::n_string(HWIDGenerator::GetSessionNonce().c_str()) << characterName;
        SendMsg(buf);
        title->ShowMessage(KmtGetText(L"UIIT_KMT_SAVING_CHARACTER_LOGIN_SECURELY"), 0xFFFFD953);
    }

    void SendQuickLogin(CPSTitle* title, int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(g_state.entries.size()))
            return;

        QuickLoginEntry& entry = g_state.entries[rowIndex];
        std::n_string token;
        if (!UnprotectToken(entry.protectedTokenHex, token)) {
            title->ShowMessage(KmtGetText(L"UIIT_KMT_SAVED_LOGIN_TOKEN_CANNOT_BE_OPENED_ON_THIS_WINDOWS_USER"), 0xFFFF671D);
            return;
        }

        g_state.pendingAutoCharacterName.clear();
        g_state.pendingQuickLoginCharacterName = entry.characterName;
        g_state.pendingAutoCharacterName = entry.characterName;
        CMsgStreamBuffer buf(OP_CLIENT_QUICK_LOGIN);
        buf << entry.username << token << GetDeviceId() << GetUnixTime()
            << std::n_string(HWIDGenerator::GetSessionNonce().c_str());
        SendMsg(buf);
        title->ShowMessage(KmtGetText(L"UIIT_KMT_CHECKING_SAVED_ACCOUNT"), 0xFFFFD953);
    }

    void SendRevoke(CPSTitle* title, int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(g_state.entries.size()))
            return;

        QuickLoginEntry entry = g_state.entries[rowIndex];
        std::n_string token;
        if (UnprotectToken(entry.protectedTokenHex, token)) {
            CMsgStreamBuffer buf(OP_CLIENT_REVOKE_QUICK_LOGIN_TOKEN);
            buf << entry.username << token << GetDeviceId() << GetUnixTime()
                << std::n_string(HWIDGenerator::GetSessionNonce().c_str());
            SendMsg(buf);
        }

        RemoveEntryAt(rowIndex);
        RefreshRows();
        title->ShowMessage(KmtGetText(L"UIIT_KMT_SAVED_ACCOUNT_REMOVED_LOCALLY"), 0xFFFFD953);
    }
}

void QuickLogin_ResetPanel() {
    // CPSTitle can be rebuilt without unloading KMTGuardKit (for example after a
    // server restart). Its child controls are destroyed, while this module's
    // static state survives. Never carry those stale UI pointers into the new
    // title scene.
    ResetPanelControlPointers();
    g_state.manageMode = false;
    g_state.addMode = false;
}

GFX_IMPLEMENT_DYNCREATE(CIFQuickLoginButton, CIFButton)

CIFQuickLoginButton::CIFQuickLoginButton() {
}

CIFQuickLoginButton::~CIFQuickLoginButton() {
}

int CIFQuickLoginButton::OnMouseLeftUp(int a1, int x, int y) {
    if (!g_state.title)
        return CIFButton::OnMouseLeftUp(a1, x, y);
    if (!g_state.enabled)
        return 0;

    const int id = UniqueID();
    if (id == ID_QL_SAVE) {
        if (!g_state.addMode && !g_state.entries.empty()) {
            g_state.addMode = true;
            g_state.manageMode = false;
            RefreshRows();
            return 0;
        }

        SendCreateToken(g_state.title);
        return 0;
    }
    if (id == ID_QL_MANAGE) {
        g_state.addMode = false;
        g_state.manageMode = true;
        RefreshRows();
        return 0;
    }
    if (id == ID_QL_BACK) {
        ShowSavedAccountsList();
        return 0;
    }
    if (id >= ID_QL_ROW_BASE && id < ID_QL_ROW_BASE + QL_ROW_COUNT * ID_QL_ROW_STEP) {
        int rowIndex = (id - ID_QL_ROW_BASE) / ID_QL_ROW_STEP;
        if (g_state.manageMode)
            SendRevoke(g_state.title, rowIndex);
        else
            SendQuickLogin(g_state.title, rowIndex);
        return 0;
    }

    return CIFButton::OnMouseLeftUp(a1, x, y);
}

void QuickLogin_CreatePanel(CPSTitle* title) {
    if (!title)
        return;

    if (BindPanelControls(title)) {
        RefreshRows();
        EnsurePresentationText();
        return;
    }

    ResetPanelControlPointers();
    g_state.title = title;
    g_state.manageMode = false;
    g_state.addMode = false;

    // Creating a new CPSTitle only rebuilds controls; it must not overwrite
    // the last server-owned feature state. On a cold start enabled remains
    // false until the settings packet arrives. Across an in-process Restart,
    // the previously received value survives while stale controls are rebound.

    RECT bgRect = {620, 360, 288, 190};
    g_state.bg = (CIFStatic*)CGWnd::CreateInstance(title, GFX_RUNTIME_CLASS(CIFStatic), bgRect, ID_QL_PANEL_BG, 0);
    if (g_state.bg) {
        g_state.bg->TB_Func_13("clientlibrary\\charselect\\quicklogin.ddj", 1, 1);
        g_state.bg->SetGWndSize(288, 190);
        g_state.bg->ShowGWnd(true);
    }

    g_state.titleText = CreateStatic(title, ID_QL_TITLE, 664, 384, 210, 18, KmtGetText(L"UIIT_KMT_SAVED_ACCOUNTS"), D3DCOLOR_ARGB(255, 239, 218, 164));
    g_state.emptyText = CreateStatic(title, ID_QL_EMPTY, 662, 476, 210, 34, KmtGetText(L"UIIT_KMT_NO_SAVED_ACCOUNTS_YET"), D3DCOLOR_ARGB(255, 235, 228, 210));
    g_state.userLabel = CreateStatic(title, ID_QL_USER_LABEL, 656, 418, 60, 18, KmtGetText(L"UIIT_KMT_ID"), D3DCOLOR_ARGB(255, 239, 218, 164));
    g_state.passLabel = CreateStatic(title, ID_QL_PASS_LABEL, 656, 445, 60, 18, KmtGetText(L"UIIT_KMT_PW"), D3DCOLOR_ARGB(255, 239, 218, 164));
    g_state.charLabel = CreateStatic(title, ID_QL_CHAR_LABEL, 656, 472, 60, 18, KmtGetText(L"UIIT_KMT_CHAR"), D3DCOLOR_ARGB(255, 239, 218, 164));
    g_state.userEdit = CreateEdit(title, &g_state.userBg, ID_QL_USER_BG, ID_QL_USER_EDIT, 712, 415, 150, 32);
    g_state.passEdit = CreateEdit(title, &g_state.passBg, ID_QL_PASS_BG, ID_QL_PASS_EDIT, 712, 442, 150, 32);
    g_state.charEdit = CreateEdit(title, &g_state.charBg, ID_QL_CHAR_BG, ID_QL_CHAR_EDIT, 712, 469, 150, 32);

    for (int i = 0; i < QL_ROW_COUNT; ++i) {
        int base = ID_QL_ROW_BASE + i * ID_QL_ROW_STEP;
        g_state.rowName[i] = CreateStatic(title, base + 1, 638, 403 + i * 27, 136, 14, L"", D3DCOLOR_ARGB(255, 255, 255, 255));
        g_state.rowDate[i] = CreateStatic(title, base + 2, 638, 416 + i * 27, 136, 12, L"", D3DCOLOR_ARGB(255, 166, 166, 166));
        g_state.rowButton[i] = CreateButton(title, base, 784, 402 + i * 27, 72, 24, KmtGetText(L"UIIT_KMT_PLAY"));
    }

    g_state.saveButton = CreateButton(title, ID_QL_SAVE, 638, 504, 104, 30, KmtGetText(L"UIIT_KMT_PLUS_SAVE"));
    g_state.manageButton = CreateButton(title, ID_QL_MANAGE, 750, 504, 104, 30, KmtGetText(L"UIIT_KMT_MANAGE"));
    g_state.backButton = CreateButton(title, ID_QL_BACK, 784, 504, 72, 30, KmtGetText(L"UIIT_KMT_BACK"));

    RefreshRows();
}

void QuickLogin_SyncPanel(CPSTitle* title) {
    if (!title) {
        return;
    }

    if (!BindPanelControls(title)) {
        ResetPanelControlPointers();
        QuickLogin_CreatePanel(title);
        return;
    }

    RefreshRows();
    EnsurePresentationText();
}

void QuickLogin_EnsurePresentation(CPSTitle* title) {
    // Rebind to the active title after Restart/account switching, then keep
    // the native caption overlays synchronized without reloading saved-account
    // data every frame.
    if (!title) {
        return;
    }

    if (!BindPanelControls(title)) {
        ResetPanelControlPointers();
        if (g_state.enabled) {
            QuickLogin_CreatePanel(title);
        }
        return;
    }

    EnsurePresentationText();
}

void QuickLogin_SetEnabled(CPSTitle* title, bool enabled) {
    g_state.enabled = enabled;

    if (title && !BindPanelControls(title)) {
        ResetPanelControlPointers();
    }

    if (!g_state.bg && enabled) {
        QuickLogin_CreatePanel(title);
        return;
    }

    if (title)
        g_state.title = title;
    RefreshRows();
}

void QuickLogin_HandleCreateTokenResult(CPSTitle* title, bool success, const std::n_string& username, const std::n_string& token, const wchar_t* message) {
    if (success) {
        std::n_string protectedHex;
        if (!ProtectToken(token, protectedHex)) {
            title->ShowMessage(KmtGetText(L"UIIT_KMT_WINDOWS_FAILED_TO_PROTECT_THE_TOKEN_ACCOUNT_WAS_NOT_SAVED_LOCALLY"), 0xFFFF671D);
            g_state.pendingCreateCharacterName.clear();
            return;
        }

        std::n_string characterName = g_state.pendingCreateCharacterName;
        QuickLoginEntry* existing = FindEntry(username, characterName);
        if (existing) {
            existing->username = username;
            existing->protectedTokenHex = protectedHex;
            existing->characterName = characterName;
            existing->displayName = characterName.empty() ? username : characterName;
            existing->lastUsed = GetNowString();
        } else {
            QuickLoginEntry entry;
            entry.username = username;
            entry.protectedTokenHex = protectedHex;
            entry.characterName = characterName;
            entry.displayName = characterName.empty() ? username : characterName;
            entry.lastUsed = GetNowString();
            g_state.entries.push_back(entry);
        }

        SaveEntries();
        if (g_state.userEdit)
            g_state.userEdit->SetText(L"");
        if (g_state.passEdit)
            g_state.passEdit->SetText(L"");
        if (g_state.charEdit)
            g_state.charEdit->SetText(L"");
        g_state.pendingCreateCharacterName.clear();
        g_state.addMode = false;
        g_state.manageMode = false;
        RefreshRows();
    } else {
        g_state.pendingCreateCharacterName.clear();
    }

    title->ShowMessage(message, success ? 0xFF00FF00 : 0xFFFF671D);
}

void QuickLogin_HandleQuickLoginResult(CPSTitle* title, bool success, const wchar_t* message) {
    if (success)
        g_state.pendingAutoCharacterName = g_state.pendingQuickLoginCharacterName;
    else
        g_state.pendingAutoCharacterName.clear();
    g_state.pendingQuickLoginCharacterName.clear();
    title->ShowMessage(message, success ? 0xFF00FF00 : 0xFFFF671D);
}

void QuickLogin_HandleRevokeTokenResult(CPSTitle* title, bool success, const wchar_t* message) {
    title->ShowMessage(message, success ? 0xFF00FF00 : 0xFFFF671D);
}

bool QuickLogin_GetPendingCharacterName(std::n_string& characterName) {
    if (g_state.pendingAutoCharacterName.empty())
        return false;

    characterName = g_state.pendingAutoCharacterName;
    return true;
}

void QuickLogin_ClearPendingCharacterName() {
    g_state.pendingAutoCharacterName.clear();
}
