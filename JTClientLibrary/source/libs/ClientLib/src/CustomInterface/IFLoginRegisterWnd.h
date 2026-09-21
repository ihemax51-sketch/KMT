#pragma once

#include "IFButton.h"
#include "IFEdit.h"
#include "IFFrame.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFStatic.h"

#define LOGIN_REGISTER_WINDOW_ID 1941
#define LOGIN_REGISTER_BUTTON_ID 1940
#define LOGIN_REGISTER_CAPTION_ID 1942

class CIFLoginRegisterWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFLoginRegisterWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFLoginRegisterWnd)

public:
    CIFLoginRegisterWnd();
    ~CIFLoginRegisterWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    undefined1 OnCloseWnd() override;
    void HandleRegisterResponse(bool success, const wchar_t* message);
    void FocusNextField(CIFEdit* current, bool backwards);
    bool HandlePasswordInput(CIFEdit* edit, UINT message, WPARAM key);
    static CIFLoginRegisterWnd* GetActiveWindow();

private:
    void OnRegister();
    void OnCancel();
    void ClearInputs();
    void UpdateWindowPos();
    void ShowTitleMessage(const wchar_t* message, D3DCOLOR color);
    std::n_string GetAsciiText(CIFEdit* edit) const;
    std::n_string GetAsciiPassword(const std::n_wstring& password) const;
    std::n_wstring* PasswordForEdit(CIFEdit* edit);
    void RefreshPasswordEdit(
        CIFEdit* edit,
        const std::n_wstring& password,
        unsigned int cursor);
    void PastePasswordFromClipboard(
        CIFEdit* edit,
        std::n_wstring& password);
    CIFEdit* CreateEditBox(int id, int x, int y, int width, int maxLength);
    CIFStatic* CreateLabel(
        int id,
        int x,
        int y,
        int width,
        int height,
        const wchar_t* text,
        D3DCOLOR color,
        CTextBoard::eJustifyHorizontal justify);

private:
    CIFFrame* m_pPanelFrame;
    CIFNormalTile* m_pBackground;
    CIFStatic* m_pSectionTitle;
    CIFStatic* m_pDescription;
    CIFStatic* m_pHint;
    CIFEdit* m_pUserId;
    CIFEdit* m_pPassword;
    CIFEdit* m_pConfirmPassword;
    CIFButton* m_pRegisterBtn;
    CIFButton* m_pCancelBtn;
    std::n_wstring m_passwordValue;
    std::n_wstring m_confirmPasswordValue;
};

class CIFLoginRegisterButton : public CIFButton {
    GFX_DECLARE_DYNCREATE(CIFLoginRegisterButton)

public:
    CIFLoginRegisterButton();
    ~CIFLoginRegisterButton();

    int OnMouseLeftUp(int a1, int x, int y) override;
};
