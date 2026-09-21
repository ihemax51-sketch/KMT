#pragma once

#include "IFButton.h"
#include "IFDecoratedStatic.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFRenderStatic.h"
#include "IFSlotWithHelp.h"
#include "IFStatic.h"

#define SPECIAL_OFFERS_WINDOW_ID 13431

class CIFSpecialOffersWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFSpecialOffersWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFSpecialOffersWnd)

public:
    CIFSpecialOffersWnd();
    ~CIFSpecialOffersWnd();

    bool OnCreate(long ln) override;
    void ShowGWnd(bool bVisible) override;
    void OnUpdate() override;
    undefined1 OnCloseWnd() override;

    void RefreshOffers();
    void HandlePurchaseResult(bool success, int offerId, const wchar_t* message);

private:
    void UpdateWindowPos();
    void RequestOffers();
    void OnPrevPage();
    void OnNextPage();
    void OnBuyLeft();
    void OnBuyRight();
    void BuyOfferAtCard(int cardIndex);
    void ClearCard(int cardIndex);
    void RenderCard(int cardIndex, int offerIndex);
    void FillSlot(CIFSlotWithHelp* slot, int refObjId, int amount);
    void ClearSlot(CIFSlotWithHelp* slot);

    CIFStatic* m_background;
    CIFStatic* m_headerBackground;
    CIFStatic* m_sectionTitle;
    CIFStatic* m_cardBackgrounds[2];
    CIFStatic* m_cardRankLabels[2];
    CIFStatic* m_cardTitleLabels[2];
    CIFStatic* m_previewBackgrounds[2];
    CIFRenderStatic* m_renderPreviews[2];
    CIFStatic* m_previews[2];
    CIFStatic* m_itemSlotBackgrounds[2];
    CIFSlotWithHelp* m_itemSlots[2];
    CIFStatic* m_mainPriceLabels[2];
    CIFStatic* m_salePriceLabels[2];
    CIFButton* m_buyButtons[2];
    CIFButton* m_prevButton;
    CIFButton* m_nextButton;
    CIFStatic* m_pageLabel;
    CIFStatic* m_statusLabel;
    int m_currentPage;
    int m_pendingOfferId;
    DWORD m_requestTick;
    bool m_waitingForPurchase;
};

class CIFSpecialOffersGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFSpecialOffersGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
