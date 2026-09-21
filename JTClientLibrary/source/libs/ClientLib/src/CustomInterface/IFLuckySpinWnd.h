#pragma once

#include "IFButton.h"
#include "IFDecoratedStatic.h"
#include "IFFrame.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFSlotWithHelp.h"
#include "IFStatic.h"

#define LUCKY_SPIN_WINDOW_ID 13421
#define LUCKY_SPIN_SHORTCUT_ID 13422

class CIFLuckySpinWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFLuckySpinWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFLuckySpinWnd)

public:
    CIFLuckySpinWnd();
    ~CIFLuckySpinWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;
    void RefreshRewards();
    void StartSpin(int winningSlot);

private:
    void OnPlay();
    void UpdateWindowPos();
    void UpdateWheelLayout(float angle);
    void ClearSlot(int index);
    void FillSlot(int index, int refObjId, int amount);
    void SetHighlightedSlot(int index);

    CIFFrame* m_panelFrame;
    CIFNormalTile* m_background;
    CIFFrame* m_wheelBorder;
    CIFNormalTile* m_wheelPanel;
    CIFStatic* m_footerBorder;
    CIFStatic* m_priceLabel;
    CIFStatic* m_statusLabel;
    CIFStatic* m_pointerLabel;
    CIFStatic* m_centerLabel;
    CIFStatic* m_resultLabel;
    CIFButton* m_playButton;
    CIFStatic* m_slotBackgrounds[16];
    CIFSlotWithHelp* m_slots[16];
    int m_highlightedSlot;
    int m_totalSpinSteps;
    int m_completedSpinSteps;
    DWORD m_lastSpinStepTick;
    DWORD m_spinRequestTick;
    DWORD m_spinStartTick;
    DWORD m_spinDuration;
    float m_wheelAngle;
    float m_spinStartAngle;
    float m_spinEndAngle;
    int m_winningSlot;
    bool m_waitingForResult;
    bool m_isSpinning;
};

class CIFLuckySpinShortcutButton : public CIFButton {
    GFX_DECLARE_DYNCREATE(CIFLuckySpinShortcutButton)

public:
    CIFLuckySpinShortcutButton();
    ~CIFLuckySpinShortcutButton();

    int OnMouseLeftUp(int a1, int x, int y) override;
};

class CIFLuckySpinGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFLuckySpinGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
