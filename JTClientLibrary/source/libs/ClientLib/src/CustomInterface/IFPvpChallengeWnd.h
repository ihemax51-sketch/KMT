#pragma once

#include "IFButton.h"
#include "IFDecoratedStatic.h"
#include "IFEdit.h"
#include "IFFrame.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFStatic.h"

#define PVP_CHALLENGE_WINDOW_ID 13435
#define PVP_CHALLENGE_ANSWER_WINDOW_ID 13436

class CIFPvpChallengeWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFPvpChallengeWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFPvpChallengeWnd)

public:
    CIFPvpChallengeWnd();
    ~CIFPvpChallengeWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void HandleStatus(bool success, const wchar_t* message);

private:
    void OnChallenge();
    void UpdateWindowPos();
    void ClearRequestFields();
    CIFEdit* CreateEditBox(int id, int x, int y, int width, int maxLength);
    CIFStatic* CreateLabel(int id, int x, int y, int width, const wchar_t* text);
    bool TryParseGold(__int64& value) const;
    void SetStatus(const wchar_t* message, D3DCOLOR color);

    CIFFrame* m_panelFrame;
    CIFNormalTile* m_background;
    CIFStatic* m_targetLabel;
    CIFStatic* m_goldLabel;
    CIFStatic* m_statusLabel;
    CIFEdit* m_targetEdit;
    CIFEdit* m_goldEdit;
    CIFButton* m_challengeButton;
    DWORD m_requestTick;
    bool m_waitingForResponse;
};

class CIFPvpChallengeAnswerWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFPvpChallengeAnswerWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFPvpChallengeAnswerWnd)

public:
    CIFPvpChallengeAnswerWnd();
    ~CIFPvpChallengeAnswerWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void OpenRequest(__int64 matchId, const wchar_t* challengerName, __int64 wagerGold, int timeoutSeconds);

private:
    void OnAccept();
    void OnDecline();
    void SendAnswer(bool accepted);
    void UpdateWindowPos();
    void UpdateCountdown();

    CIFFrame* m_panelFrame;
    CIFNormalTile* m_background;
    CIFStatic* m_messageLabel;
    CIFStatic* m_wagerLabel;
    CIFStatic* m_countdownLabel;
    CIFButton* m_acceptButton;
    CIFButton* m_declineButton;
    __int64 m_matchId;
    __int64 m_wagerGold;
    DWORD m_requestTick;
    int m_timeoutSeconds;
    bool m_hasRequest;
};

class CIFPvpChallengeGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFPvpChallengeGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
