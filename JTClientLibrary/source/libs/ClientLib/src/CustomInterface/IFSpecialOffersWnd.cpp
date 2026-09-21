#include "IFSpecialOffersWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "CustomData/CustomDataManager.h"
#include "CustomData/CustomSettingManager.h"
#include "GInterface.h"
#include "Game.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"
#include "IFEquipment.h"
#include "IFMainPopup.h"
#include "SOItem.h"
#include <Windows.h>
#include <cstdio>

#define ID_SPECIAL_BG 10
#define ID_SPECIAL_HEADER_BG 11
#define ID_SPECIAL_SECTION_TITLE 12
#define ID_SPECIAL_CARD_BG 20
#define ID_SPECIAL_RANK 30
#define ID_SPECIAL_TITLE 40
#define ID_SPECIAL_PREVIEW_BG 50
#define ID_SPECIAL_PREVIEW 60
#define ID_SPECIAL_RENDER_PREVIEW 160
#define ID_SPECIAL_SLOT_BG 70
#define ID_SPECIAL_SLOT 80
#define ID_SPECIAL_MAIN_PRICE 90
#define ID_SPECIAL_SALE_PRICE 100
#define ID_SPECIAL_BUY_LEFT 110
#define ID_SPECIAL_BUY_RIGHT 111
#define ID_SPECIAL_PREV 120
#define ID_SPECIAL_NEXT 121
#define ID_SPECIAL_PAGE 122
#define ID_SPECIAL_STATUS 123

namespace {
const int OFFERS_PER_PAGE = 2;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 500;
const D3DCOLOR COLOR_ACCENT = D3DCOLOR_ARGB(255, 255, 216, 83);
const D3DCOLOR COLOR_LABEL = D3DCOLOR_ARGB(255, 239, 218, 164);
const D3DCOLOR COLOR_TEXT = D3DCOLOR_ARGB(255, 245, 242, 232);
const D3DCOLOR COLOR_MUTED = D3DCOLOR_ARGB(255, 185, 184, 174);
const D3DCOLOR COLOR_SELECTED = D3DCOLOR_ARGB(255, 132, 225, 221);
const D3DCOLOR COLOR_ACTIVE = D3DCOLOR_ARGB(255, 114, 255, 154);
const D3DCOLOR COLOR_WARNING = D3DCOLOR_ARGB(255, 255, 132, 118);

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
    control->ShowGWnd(true);
    control->BringToFront();
}

void SetFormattedText(CIFStatic* control, D3DCOLOR color, const wchar_t* format, int value)
{
    if (!control) {
        return;
    }

    wchar_t buffer[128];
    swprintf(buffer, 128, format, value);
    control->SetText(buffer);
    control->m_FontTexture.SetColor(color);
}

void StyleButton(CIFButton* button, const wchar_t* text, const char* texture)
{
    if (!button) {
        return;
    }

    button->TB_Func_13(texture, 1, 1);
    button->SetText(text);
    button->SetFont(theApp.GetFont(0));
    button->m_FontTexture.SetColor(COLOR_TEXT);
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->ShowGWnd(true);
    button->BringToFront();
}

bool InitializeRenderPreview(CIFRenderStatic* render)
{
    if (!render || !g_pCGInterface || !g_pMyPlayerObj ||
        !g_pMyPlayerObj->GetCommonData() ||
        !g_pCGInterface->GetMainPopup()) {
        return false;
    }

    CIFEquipment* inventory = g_pCGInterface->GetMainPopup()->GetEquipment();
    if (!inventory) {
        return false;
    }

    render->TB_Func_13("interface\\mall\\mall_charac_frame.ddj", 1, 0);
    render->Clear();
    render->FUN_005602c0(render->GetCharacterObj(g_pMyPlayerObj->GetCommonData()->RefObjectId, &inventory->Char), 0);
    render->Test1();

    undefined4 renderArg = 0;
    render->Test(&renderArg);
    render->Test2(&renderArg, 0x420d1000);
    render->field_0x430 = 0x43fa0000;
    render->Test3(0, 0);
    render->yukariasagi = 15.000f;
    render->N00000609 = 1;
    return true;
}

void RemoveCurrentAvatarPart(CIFRenderStatic* render, const SItemData* offeredItem)
{
    if (!render || !offeredItem || !g_pCGInterface || !g_pCGInterface->GetMainPopup()) {
        return;
    }

    CIFEquipment* inventory = g_pCGInterface->GetMainPopup()->GetEquipment();
    if (!inventory) {
        return;
    }

    for (int i = 0; i < 5; ++i) {
        CSOItem* currentItem = inventory->Test4((BYTE)i);
        if (!currentItem || !currentItem->GetItemData()) {
            continue;
        }

        const SItemData* currentData = currentItem->GetItemData();
        const bool sameAvatarSlot =
            (offeredItem->IsAvatar() && currentData->IsAvatar()) ||
            (offeredItem->IsAvatarHat() && currentData->IsAvatarHat()) ||
            (offeredItem->IsAvatarAttach() && currentData->IsAvatarAttach());

        if (sameAvatarSlot) {
            render->RemoveItem(0, currentData->RefObjectId, 1);
        }
    }
}

bool RenderItemMallPreview(CIFRenderStatic* render, const CustomDataManager::SpecialOfferItem& offer, const SItemData* itemData)
{
    if (!render || !itemData) {
        return false;
    }

    const bool isAvatarItem = itemData->IsAvatar() || itemData->IsAvatarHat() || itemData->IsAvatarAttach();
    const bool isPetPreview = offer.PreviewMode == 2 && offer.PreviewRefObjID > 0;

    if (!isAvatarItem && !isPetPreview && offer.PreviewMode != 1) {
        return false;
    }

    if (!InitializeRenderPreview(render)) {
        return false;
    }

    if (isAvatarItem) {
        RemoveCurrentAvatarPart(render, itemData);
        render->WearItem(0, itemData->RefObjectId, 1);
    } else if (isPetPreview) {
        unsigned int petDistance = 0x40900000;
        render->FUN_00561780((unsigned int)offer.PreviewRefObjID);
        render->FUN_0055fe70(1, &petDistance);
        render->FUN_005602c0(1, 0);
    }

    render->ShowGWnd(true);
    render->BringToFront();
    return true;
}
}

GFX_IMPLEMENT_DYNCREATE(CIFSpecialOffersWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFSpecialOffersWnd, CIFMainFrame)
                    ONG_COMMAND(ID_SPECIAL_PREV, &CIFSpecialOffersWnd::OnPrevPage)
                    ONG_COMMAND(ID_SPECIAL_NEXT, &CIFSpecialOffersWnd::OnNextPage)
                    ONG_COMMAND(ID_SPECIAL_BUY_LEFT, &CIFSpecialOffersWnd::OnBuyLeft)
                    ONG_COMMAND(ID_SPECIAL_BUY_RIGHT, &CIFSpecialOffersWnd::OnBuyRight)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFSpecialOffersGuide, CIFDecoratedStatic)

CIFSpecialOffersWnd::CIFSpecialOffersWnd()
    : m_background(0), m_headerBackground(0), m_sectionTitle(0),
      m_prevButton(0), m_nextButton(0), m_pageLabel(0), m_statusLabel(0),
      m_currentPage(0), m_pendingOfferId(0), m_requestTick(0), m_waitingForPurchase(false)
{
    for (int i = 0; i < 2; ++i) {
        m_cardBackgrounds[i] = 0;
        m_cardRankLabels[i] = 0;
        m_cardTitleLabels[i] = 0;
        m_previewBackgrounds[i] = 0;
        m_renderPreviews[i] = 0;
        m_previews[i] = 0;
        m_itemSlotBackgrounds[i] = 0;
        m_itemSlots[i] = 0;
        m_mainPriceLabels[i] = 0;
        m_salePriceLabels[i] = 0;
        m_buyButtons[i] = 0;
    }
}

CIFSpecialOffersWnd::~CIFSpecialOffersWnd() {
}

bool CIFSpecialOffersWnd::OnCreate(long ln) {
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    SetText(KmtGetText(L"UIIT_KMT_SPECIAL_OFFERS"));

    RECT bgRect = {0, 31, WINDOW_WIDTH, WINDOW_HEIGHT - 31};
    m_background = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), bgRect, ID_SPECIAL_BG, 0);
    if (m_background) {
        m_background->TB_Func_13("clientlibrary\\mall\\win_bg.ddj", 0, 0);
        m_background->SetClickable(false);
        m_background->ShowGWnd(true);
    }

    RECT headerRect = {0, 31, WINDOW_WIDTH, 45};
    m_headerBackground = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), headerRect, ID_SPECIAL_HEADER_BG, 0);
    if (m_headerBackground) {
        m_headerBackground->TB_Func_13("clientlibrary\\mall\\header.ddj", 0, 0);
        m_headerBackground->SetClickable(false);
        m_headerBackground->ShowGWnd(true);
    }

    RECT sectionTitleRect = {24, 43, WINDOW_WIDTH - 48, 24};
    m_sectionTitle = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sectionTitleRect, ID_SPECIAL_SECTION_TITLE, 0);
    StyleStatic(m_sectionTitle, KmtGetText(L"UIIT_KMT_LIMITED_EDITION_OFFERS"), COLOR_ACCENT, CTextBoard::JUSTIFY_CENTER);

    for (int i = 0; i < 2; ++i) {
        const int x = i == 0 ? 20 : 407;

        RECT cardRect = {x, 82, 373, 350};
        m_cardBackgrounds[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), cardRect, ID_SPECIAL_CARD_BG + i, 0);
        if (m_cardBackgrounds[i]) {
            m_cardBackgrounds[i]->TB_Func_13("clientlibrary\\mall\\leftbg.ddj", 0, 0);
            m_cardBackgrounds[i]->SetClickable(false);
            m_cardBackgrounds[i]->ShowGWnd(true);
        }

        RECT rankRect = {x + 325, 91, 30, 18};
        m_cardRankLabels[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), rankRect, ID_SPECIAL_RANK + i, 0);
        StyleStatic(m_cardRankLabels[i], L"", COLOR_ACCENT, CTextBoard::JUSTIFY_RIGHT);

        RECT slotBgRect = {x + 18, 92, 40, 40};
        m_itemSlotBackgrounds[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), slotBgRect, ID_SPECIAL_SLOT_BG + i, 0);
        if (m_itemSlotBackgrounds[i]) {
            m_itemSlotBackgrounds[i]->TB_Func_13("interface\\store\\str_slot_02.ddj", 0, 0);
            m_itemSlotBackgrounds[i]->SetClickable(false);
            m_itemSlotBackgrounds[i]->ShowGWnd(true);
        }

        RECT slotRect = {x + 22, 96, 32, 32};
        m_itemSlots[i] = (CIFSlotWithHelp*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), slotRect, ID_SPECIAL_SLOT + i, 0);
        if (m_itemSlots[i]) {
            m_itemSlots[i]->SetType(19);
            m_itemSlots[i]->SetSlot(ID_SPECIAL_SLOT + i);
            m_itemSlots[i]->SetClickable(false);
            m_itemSlots[i]->ShowGWnd(true);
        }

        RECT titleRect = {x + 66, 92, 252, 28};
        m_cardTitleLabels[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), titleRect, ID_SPECIAL_TITLE + i, 0);
        StyleStatic(m_cardTitleLabels[i], L"", COLOR_SELECTED, CTextBoard::JUSTIFY_CENTER);

        RECT previewBgRect = {x + 18, 140, 337, 190};
        m_previewBackgrounds[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), previewBgRect, ID_SPECIAL_PREVIEW_BG + i, 0);
        if (m_previewBackgrounds[i]) {
            m_previewBackgrounds[i]->TB_Func_13("clientlibrary\\mall\\mall_pre_start.ddj", 0, 0);
            m_previewBackgrounds[i]->SetClickable(false);
            m_previewBackgrounds[i]->ShowGWnd(true);
        }

        RECT previewRect = {x + 24, 146, 325, 178};
        m_renderPreviews[i] = (CIFRenderStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFRenderStatic), previewRect, ID_SPECIAL_RENDER_PREVIEW + i, 0);
        if (m_renderPreviews[i]) {
            m_renderPreviews[i]->SetClickable(false);
            m_renderPreviews[i]->TB_Func_13("interface\\mall\\mall_charac_frame.ddj", 1, 0);
            m_renderPreviews[i]->ShowGWnd(false);
        }

        m_previews[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), previewRect, ID_SPECIAL_PREVIEW + i, 0);
        if (m_previews[i]) {
            m_previews[i]->SetClickable(false);
            m_previews[i]->ShowGWnd(false);
        }

        RECT mainPriceRect = {x + 24, 337, 325, 18};
        m_mainPriceLabels[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), mainPriceRect, ID_SPECIAL_MAIN_PRICE + i, 0);
        StyleStatic(m_mainPriceLabels[i], L"", COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

        RECT salePriceRect = {x + 24, 357, 325, 20};
        m_salePriceLabels[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), salePriceRect, ID_SPECIAL_SALE_PRICE + i, 0);
        StyleStatic(m_salePriceLabels[i], L"", COLOR_ACTIVE, CTextBoard::JUSTIFY_CENTER);

        RECT buyRect = {x + 130, 389, 112, 32};
        m_buyButtons[i] = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), buyRect, i == 0 ? ID_SPECIAL_BUY_LEFT : ID_SPECIAL_BUY_RIGHT, 0);
        StyleButton(m_buyButtons[i], KmtGetText(L"UIIT_KMT_PURCHASE"), "clientlibrary\\mall\\mall_pre_big_button.ddj");
    }

    RECT prevRect = {120, 440, 12, 42};
    m_prevButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), prevRect, ID_SPECIAL_PREV, 0);
    if (m_prevButton) {
        m_prevButton->TB_Func_13("clientlibrary\\mall\\mall_pre_left_button.ddj", 1, 1);
        m_prevButton->ShowGWnd(true);
    }

    RECT pageRect = {144, 449, 76, 24};
    m_pageLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), pageRect, ID_SPECIAL_PAGE, 0);
    StyleStatic(m_pageLabel, L"1 / 1", COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);

    RECT nextRect = {232, 440, 12, 42};
    m_nextButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), nextRect, ID_SPECIAL_NEXT, 0);
    if (m_nextButton) {
        m_nextButton->TB_Func_13("clientlibrary\\mall\\mall_pre_right_button.ddj", 1, 1);
        m_nextButton->ShowGWnd(true);
    }

    RECT statusRect = {286, 449, 474, 24};
    m_statusLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_SPECIAL_STATUS, 0);
    StyleStatic(m_statusLabel, KmtGetText(L"UIIT_KMT_READY"), COLOR_MUTED, CTextBoard::JUSTIFY_CENTER);

    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(COLOR_LABEL);
        m_pTitleText->BringToFront();
    }
    if (m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
        m_pCloseBtn->SetGWndSize(28, 32);
        m_pCloseBtn->MoveGWnd(WINDOW_WIDTH - 36, 2);
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }
    UpdateWindowPos();
    ShowGWnd(false);
    return true;
}

void CIFSpecialOffersWnd::ShowGWnd(bool bVisible) {
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        RequestOffers();
        RefreshOffers();
        BringToFront();
    }
}

void CIFSpecialOffersWnd::OnUpdate() {
    CIFMainFrame::OnUpdate();

    if (m_waitingForPurchase && GetTickCount() - m_requestTick > 7000) {
        m_waitingForPurchase = false;
        m_pendingOfferId = 0;
        if (m_statusLabel) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_PURCHASE_REQUEST_TIMED_OUT_PLEASE_TRY_AGAIN"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_WARNING);
        }
        RefreshOffers();
    }
}

undefined1 CIFSpecialOffersWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CIFSpecialOffersWnd::UpdateWindowPos() {
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int y = ((height - GetSize().height) / 2) + 35;
    if (y < 70) {
        y = 70;
    }
    if (y + GetSize().height > height - 20) {
        y = height - GetSize().height - 20;
    }
    MoveGWnd((width - GetSize().width) / 2, y);
}

void CIFSpecialOffersWnd::RequestOffers() {
    CMsgStreamBuffer request(0x169A);
    request << byte(31);
    SendMsg(request);
}

void CIFSpecialOffersWnd::OnPrevPage() {
    if (m_currentPage > 0) {
        --m_currentPage;
        RefreshOffers();
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}

void CIFSpecialOffersWnd::OnNextPage() {
    const int count = (int)m_CustomDataManager->SpecialOffers.size();
    int pageCount = (count + OFFERS_PER_PAGE - 1) / OFFERS_PER_PAGE;
    if (pageCount < 1) {
        pageCount = 1;
    }

    if (m_currentPage + 1 < pageCount) {
        ++m_currentPage;
        RefreshOffers();
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}

void CIFSpecialOffersWnd::OnBuyLeft() {
    BuyOfferAtCard(0);
}

void CIFSpecialOffersWnd::OnBuyRight() {
    BuyOfferAtCard(1);
}

void CIFSpecialOffersWnd::BuyOfferAtCard(int cardIndex) {
    if (m_waitingForPurchase || !m_Settings->EnableSpecialOffers) {
        return;
    }

    const int offerIndex = (m_currentPage * OFFERS_PER_PAGE) + cardIndex;
    if (offerIndex < 0 || offerIndex >= (int)m_CustomDataManager->SpecialOffers.size()) {
        return;
    }

    const CustomDataManager::SpecialOfferItem& offer = m_CustomDataManager->SpecialOffers[offerIndex];
    m_waitingForPurchase = true;
    m_pendingOfferId = offer.ID;
    m_requestTick = GetTickCount();

    if (m_statusLabel) {
        m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_SENDING_PURCHASE_REQUEST"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    if (m_buyButtons[0]) m_buyButtons[0]->SetEnabledState(false);
    if (m_buyButtons[1]) m_buyButtons[1]->SetEnabledState(false);

    CMsgStreamBuffer packet(0x169A);
    packet << byte(32);
    packet << INT32(offer.ID);
    SendMsg(packet);
}

void CIFSpecialOffersWnd::RefreshOffers() {
    const int count = (int)m_CustomDataManager->SpecialOffers.size();
    int pageCount = (count + OFFERS_PER_PAGE - 1) / OFFERS_PER_PAGE;
    if (pageCount < 1) {
        pageCount = 1;
    }
    if (m_currentPage >= pageCount) {
        m_currentPage = pageCount - 1;
    }
    if (m_currentPage < 0) {
        m_currentPage = 0;
    }

    for (int i = 0; i < 2; ++i) {
        RenderCard(i, (m_currentPage * OFFERS_PER_PAGE) + i);
    }

    if (m_pageLabel) {
        wchar_t page[32];
        swprintf(page, 32, L"%d / %d", m_currentPage + 1, pageCount);
        m_pageLabel->SetText(page);
    }

    if (m_prevButton) m_prevButton->SetEnabledState(m_currentPage > 0);
    if (m_nextButton) m_nextButton->SetEnabledState(m_currentPage + 1 < pageCount);

    if (m_statusLabel && !m_waitingForPurchase) {
        if (!m_Settings->EnableSpecialOffers) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_SPECIAL_OFFERS_IS_CURRENTLY_DISABLED"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_WARNING);
        } else if (count == 0) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_NO_SPECIAL_OFFERS_CONFIGURED_YET"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
        } else {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_READY"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_MUTED);
        }
    }
}

void CIFSpecialOffersWnd::ClearCard(int cardIndex) {
    if (cardIndex < 0 || cardIndex >= 2) {
        return;
    }

    if (m_cardBackgrounds[cardIndex]) m_cardBackgrounds[cardIndex]->ShowGWnd(false);
    if (m_cardRankLabels[cardIndex]) m_cardRankLabels[cardIndex]->ShowGWnd(false);
    if (m_cardTitleLabels[cardIndex]) m_cardTitleLabels[cardIndex]->ShowGWnd(false);
    if (m_previewBackgrounds[cardIndex]) m_previewBackgrounds[cardIndex]->ShowGWnd(false);
    if (m_renderPreviews[cardIndex]) {
        m_renderPreviews[cardIndex]->Clear();
        m_renderPreviews[cardIndex]->ShowGWnd(false);
    }
    if (m_previews[cardIndex]) m_previews[cardIndex]->ShowGWnd(false);
    if (m_itemSlotBackgrounds[cardIndex]) m_itemSlotBackgrounds[cardIndex]->ShowGWnd(false);
    if (m_itemSlots[cardIndex]) m_itemSlots[cardIndex]->ShowGWnd(false);
    if (m_mainPriceLabels[cardIndex]) m_mainPriceLabels[cardIndex]->ShowGWnd(false);
    if (m_salePriceLabels[cardIndex]) m_salePriceLabels[cardIndex]->ShowGWnd(false);
    if (m_buyButtons[cardIndex]) m_buyButtons[cardIndex]->ShowGWnd(false);
    ClearSlot(m_itemSlots[cardIndex]);
}

void CIFSpecialOffersWnd::RenderCard(int cardIndex, int offerIndex) {
    if (cardIndex < 0 || cardIndex >= 2) {
        return;
    }

    if (offerIndex < 0 || offerIndex >= (int)m_CustomDataManager->SpecialOffers.size()) {
        ClearCard(cardIndex);
        return;
    }

    const CustomDataManager::SpecialOfferItem& offer = m_CustomDataManager->SpecialOffers[offerIndex];
    if (m_cardBackgrounds[cardIndex]) m_cardBackgrounds[cardIndex]->ShowGWnd(true);
    if (m_cardRankLabels[cardIndex]) {
        wchar_t rank[32];
        swprintf(rank, 32, L"#%d", offerIndex + 1);
        m_cardRankLabels[cardIndex]->SetText(rank);
        m_cardRankLabels[cardIndex]->ShowGWnd(true);
    }
    if (m_cardTitleLabels[cardIndex]) {
        m_cardTitleLabels[cardIndex]->SetText(offer.Title.size() > 0 ? offer.Title.c_str() : KmtGetText(L"UIIT_KMT_SPECIAL_OFFER"));
        m_cardTitleLabels[cardIndex]->ShowGWnd(true);
    }
    if (m_previewBackgrounds[cardIndex]) m_previewBackgrounds[cardIndex]->ShowGWnd(true);
    if (m_itemSlotBackgrounds[cardIndex]) m_itemSlotBackgrounds[cardIndex]->ShowGWnd(true);
    if (m_itemSlots[cardIndex]) m_itemSlots[cardIndex]->ShowGWnd(true);
    if (m_mainPriceLabels[cardIndex]) m_mainPriceLabels[cardIndex]->ShowGWnd(true);
    if (m_salePriceLabels[cardIndex]) m_salePriceLabels[cardIndex]->ShowGWnd(true);
    if (m_buyButtons[cardIndex]) {
        m_buyButtons[cardIndex]->ShowGWnd(true);
        m_buyButtons[cardIndex]->SetEnabledState(!m_waitingForPurchase && m_Settings->EnableSpecialOffers);
    }

    FillSlot(m_itemSlots[cardIndex], offer.ItemID, offer.ItemCount);

    const SItemData* data = &g_CGlobalDataManager->GetItemData(offer.ItemID);
    bool renderPreviewShown = false;
    if (m_renderPreviews[cardIndex]) {
        m_renderPreviews[cardIndex]->ShowGWnd(false);
        renderPreviewShown = RenderItemMallPreview(m_renderPreviews[cardIndex], offer, data);
    }

    if (m_previews[cardIndex]) {
        std::n_string previewPath = offer.PreviewImagePath;
        if (previewPath.size() == 0 && data) {
            previewPath = data->AssocFileIcon;
        }
        if (previewPath.size() > 0) {
            m_previews[cardIndex]->TB_Func_13(previewPath.c_str(), 0, 0);
        }
        m_previews[cardIndex]->ShowGWnd(!renderPreviewShown);
        if (!renderPreviewShown) {
            m_previews[cardIndex]->BringToFront();
        }
    }

    SetFormattedText(m_mainPriceLabels[cardIndex], COLOR_MUTED, KmtGetText(L"UIIT_KMT_REGULAR_PRICE_VALUE"), offer.MainPrice);

    if (m_salePriceLabels[cardIndex]) {
        const wchar_t* currency = offer.PaymentType == 1
            ? KmtGetText(L"UIIT_KMT_GOLD")
            : KmtGetText(L"UIIT_KMT_SILK");
        wchar_t price[128];
        swprintf(price, 128, KmtGetText(L"UIIT_KMT_OFFER_PRICE_VALUE_TEXT"), offer.SalePrice, currency);
        m_salePriceLabels[cardIndex]->SetText(price);
        m_salePriceLabels[cardIndex]->m_FontTexture.SetColor(COLOR_ACTIVE);
    }
}

void CIFSpecialOffersWnd::FillSlot(CIFSlotWithHelp* slot, int refObjId, int amount) {
    if (!slot || refObjId <= 0) {
        return;
    }

    const SItemData* data = &g_CGlobalDataManager->GetItemData(refObjId);
    if (!data) {
        return;
    }

    CMsgStreamBuffer packet(0xB034);
    packet << INT32(0) << INT32(refObjId);
    const u_short typeID2 = data->m_typeId.getTypeID2();
    const u_short typeID3 = data->m_typeId.getTypeID3();
    const u_short typeID4 = data->m_typeId.getTypeID4();
    if (typeID2 == 1) {
        packet << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
    } else if (typeID2 == 2) {
        if (typeID3 == 1) packet << UINT8(1);
        else if (typeID3 == 2) packet << UINT32(0);
        else if (typeID4 == 3) packet << UINT32(1);
    } else if (typeID2 == 3) {
        packet << UINT16(1);
        if (typeID3 == 11 && (typeID4 == 1 || typeID4 == 2)) packet << UINT8(0);
    }

    CSOItem* item = new CSOItem();
    item->ReadFromPacket(&packet, 1);
    item->SetEnabled(true);
    item->m_quantity = amount;
    item->m_OptLevel = 0;
    slot->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);
    slot->ItemInfo = item;
    slot->SetType(19);
    slot->ShowGWnd(true);
}

void CIFSpecialOffersWnd::ClearSlot(CIFSlotWithHelp* slot) {
    if (!slot) {
        return;
    }
    slot->SetSlotData(0);
    slot->ItemInfo = 0;
}

void CIFSpecialOffersWnd::HandlePurchaseResult(bool success, int offerId, const wchar_t* message) {
    m_waitingForPurchase = false;
    m_pendingOfferId = 0;

    if (m_statusLabel) {
        m_statusLabel->SetText(message ? message : L"");
        m_statusLabel->m_FontTexture.SetColor(success
            ? COLOR_ACTIVE
            : COLOR_WARNING);
    }

    RefreshOffers();
    CGEffSoundBody::get()->PlaySound(success ? L"snd_window_open" : L"snd_window_close");
}

bool CIFSpecialOffersGuide::OnCreate(long ln) {
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_special_offers_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_special_offers_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_SPECIAL_OFFERS"));
        label->SetStyleThingy(TOOLTIP);
    }
    return true;
}

int CIFSpecialOffersGuide::OnMouseLeftUp(int a1, int x, int y) {
    if (!g_pCGInterface) {
        return 0;
    }

    CIFSpecialOffersWnd* window = g_pCGInterface->GetGuiFromList<CIFSpecialOffersWnd>(SPECIAL_OFFERS_WINDOW_ID);
    if (!window) {
        return 0;
    }

    window->ShowGWnd(!window->IsVisible());
    CGEffSoundBody::get()->PlaySound(window->IsVisible() ? L"snd_window_open" : L"snd_window_close");
    return 0;
}

void CIFSpecialOffersGuide::OnCIFReady() {
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
