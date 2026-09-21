#pragma once

#include "IFButton.h"
#include "IFMainFrame.h"
#include "IFStatic.h"

#define OFFLINE_STALL_CONFIRM_WINDOW_ID 13437
#define OFFLINE_STALL_LOGIN_PROMPT_ID 13438
#define OFFLINE_STALL_BUTTON_ID 13439

#define OFFLINE_STALL_ACTIVATE_REQUEST_OPCODE 0x18D0
#define OFFLINE_STALL_ACTIVATE_RESULT_OPCODE 0x18D1
#define OFFLINE_STALL_LOGIN_PROMPT_OPCODE 0x18D2
#define OFFLINE_STALL_LOGIN_DECISION_OPCODE 0x18D3

class CIFOfflineStallButton : public CIFButton {
    GFX_DECLARE_DYNCREATE(CIFOfflineStallButton)

public:
    CIFOfflineStallButton();
    ~CIFOfflineStallButton();

    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
};

class CIFOfflineStallConfirmWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFOfflineStallConfirmWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFOfflineStallConfirmWnd)

public:
    CIFOfflineStallConfirmWnd();
    ~CIFOfflineStallConfirmWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool visible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void Open();
    void HandleResult(DWORD nonce, bool success, const wchar_t* message);
    bool IsRequestPending() const;
    bool IsArmed() const;

private:
    void OnConfirm();
    void OnCancel();
    void UpdateWindowPos();
    void SetStatus(const wchar_t* text, D3DCOLOR color);

    CIFStatic* m_background;
    CIFStatic* m_header;
    CIFStatic* m_message;
    CIFStatic* m_detail;
    CIFStatic* m_status;
    CIFButton* m_confirmButton;
    CIFButton* m_cancelButton;
    DWORD m_nonce;
    DWORD m_requestTick;
    bool m_pending;
    bool m_armed;
};

class CIFOfflineStallLoginPrompt : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFOfflineStallLoginPrompt)
    GFX_DECLARE_MESSAGE_MAP(CIFOfflineStallLoginPrompt)

public:
    CIFOfflineStallLoginPrompt();
    ~CIFOfflineStallLoginPrompt();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool visible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void Open(DWORD token, BYTE timeoutSeconds, const wchar_t* characterName, int actionButtonId);
    void ResetPrompt();
    void HandleActionButton(int buttonId);

private:
    void OnDisconnect();
    void OnCancel();
    void SendDecision(bool disconnectOfflineStall);
    void UpdateWindowPos();
    void UpdateCountdown();
    void SetActionButtonEnabled(bool enabled);

    CIFStatic* m_background;
    CIFStatic* m_header;
    CIFStatic* m_message;
    CIFStatic* m_question;
    CIFStatic* m_countdown;
    CIFButton* m_disconnectButton;
    CIFButton* m_cancelButton;
    DWORD m_token;
    DWORD m_requestTick;
    int m_timeoutSeconds;
    int m_actionButtonId;
    int m_lastDisplayedSecond;
    bool m_hasRequest;
    bool m_decisionSent;
};

class CIFOfflineStallLoginButton : public CIFButton {
    GFX_DECLARE_DYNCREATE(CIFOfflineStallLoginButton)

public:
    CIFOfflineStallLoginButton();
    ~CIFOfflineStallLoginButton();

    int OnMouseLeftUp(int a1, int x, int y) override;
};

bool OfflineStall_IsEnabled();
void OfflineStall_CreateLoginPrompt(CGWnd* owner);
void OfflineStall_OpenLoginPrompt(CGWnd* owner, DWORD token, BYTE timeoutSeconds,
                                  const std::n_string& characterName, int actionButtonId);
void OfflineStall_ResetLoginPrompt(CGWnd* owner);
void OfflineStall_HandleActivationResult(DWORD nonce, bool success, const std::n_string& message);
