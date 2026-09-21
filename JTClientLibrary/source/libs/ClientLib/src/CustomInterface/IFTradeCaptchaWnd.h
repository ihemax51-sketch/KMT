#pragma once

#include "IFButton.h"
#include "IFEdit.h"
#include "IFMainFrame.h"
#include "IFStatic.h"

#define TRADE_SELL_CAPTCHA_WINDOW_ID 13430

class CIFTradeCaptchaWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFTradeCaptchaWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFTradeCaptchaWnd)

public:
    CIFTradeCaptchaWnd();
    ~CIFTradeCaptchaWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool visible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void OpenChallenge(int code, int timeoutSeconds, int attemptsRemaining, bool retry);
    bool HandleKeyboardInput(UINT virtualKey);
    void SubmitFromKeyboard();
    void CancelFromKeyboard();

private:
    void OnVerify();
    void OnCancel();
    void UpdateWindowPos();
    void UpdateChallengeDigits(int code);
    void UpdateCountdown(bool force);
    void SetStatus(const wchar_t* message, D3DCOLOR color);
    void FocusInput();
    void ResetInput();
    bool TryReadCode(int& code) const;

    CIFStatic* m_background;
    CIFStatic* m_headerBackground;
    CIFStatic* m_challengePanel;
    CIFStatic* m_lockIcon;
    CIFStatic* m_sealTitle;
    CIFStatic* m_timerLabel;
    CIFStatic* m_captionLabel;
    CIFStatic* m_digitBackgrounds[4];
    CIFStatic* m_digitLabels[4];
    CIFStatic* m_progressTrack;
    CIFStatic* m_progressFill;
    CIFStatic* m_attemptsLabel;
    CIFStatic* m_inputLabel;
    CIFStatic* m_inputBackground;
    CIFEdit* m_inputEdit;
    CIFStatic* m_statusLabel;
    CIFButton* m_verifyButton;
    CIFButton* m_cancelButton;
    DWORD m_challengeTick;
    int m_timeoutSeconds;
    int m_lastRemainingSeconds;
    int m_attemptsRemaining;
    bool m_active;
};
