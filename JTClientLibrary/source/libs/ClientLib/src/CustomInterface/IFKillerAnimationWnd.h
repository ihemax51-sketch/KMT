#pragma once

#include "IFButton.h"
#include "IFDecoratedStatic.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFRenderStatic.h"
#include "IFStatic.h"

#define KILLER_ANIMATION_WINDOW_ID 1396

class CIFKillerAnimationWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFKillerAnimationWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFKillerAnimationWnd)

public:
    CIFKillerAnimationWnd();
    ~CIFKillerAnimationWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    void OnTimer(int timerId) override;
    undefined1 OnCloseWnd() override;

    void RefreshAnimations();
    void HandleActionResult(bool success, byte action, int animationId, const wchar_t* message);

private:
    void UpdateWindowPos();
    void RequestAnimations();
    void SelectRow(int row);
    void OnRow0();
    void OnRow1();
    void OnRow2();
    void OnRow3();
    void OnRow4();
    void OnRow5();
    void OnPrevPage();
    void OnNextPage();
    void OnPreview();
    void OnBuy();
    void OnActivate();
    void RefreshPreview(bool restartAnimation);
    void RefreshButtons();
    int GetSelectedAnimationIndex() const;
    int GetSelectedAnimationId() const;

    CIFStatic* m_background;
    CIFStatic* m_headerBackground;
    CIFStatic* m_listBackground;
    CIFStatic* m_previewBackground;
    CIFStatic* m_listTitle;
    CIFStatic* m_previewSectionTitle;
    CIFButton* m_rows[6];
    CIFStatic* m_rowStatus[6];
    CIFStatic* m_rowPrice[6];
    CIFRenderStatic* m_renderPreview;
    CIFStatic* m_previewTitle;
    CIFStatic* m_statusLabel;
    CIFStatic* m_pageLabel;
    CIFButton* m_prevButton;
    CIFButton* m_nextButton;
    CIFButton* m_previewButton;
    CIFButton* m_buyButton;
    CIFButton* m_activateButton;
    int m_currentPage;
    int m_selectedIndex;
    int m_pendingAnimationId;
    int m_previewAnimationId;
    unsigned int m_previewObject;
    DWORD m_requestTick;
    DWORD m_lastPreviewTick;
    bool m_waitingForResult;
};

class CIFKillerAnimationGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFKillerAnimationGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
