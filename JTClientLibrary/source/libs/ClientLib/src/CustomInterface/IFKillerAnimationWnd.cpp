#include "IFKillerAnimationWnd.h"

#include "ClientNet/MsgStreamBuffer.h"
#include "CustomData/CustomDataManager.h"
#include "GInterface.h"
#include "Game.h"
#include "ICPlayer.h"
#include "IFEquipment.h"
#include "IFMainPopup.h"
#include "KillerAnimationPlayer.h"
#include <Windows.h>
#include <cstdio>

#define ID_KILLER_BG 10
#define ID_KILLER_LIST_BG 11
#define ID_KILLER_PREVIEW_BG 12
#define ID_KILLER_HEADER_BG 13
#define ID_KILLER_LIST_TITLE 14
#define ID_KILLER_PREVIEW_SECTION_TITLE 15
#define ID_KILLER_ROW_0 20
#define ID_KILLER_ROW_STATUS 40
#define ID_KILLER_ROW_PRICE 60
#define ID_KILLER_RENDER 80
#define ID_KILLER_PREVIEW_TITLE 81
#define ID_KILLER_STATUS 82
#define ID_KILLER_PREV 83
#define ID_KILLER_NEXT 84
#define ID_KILLER_PAGE 85
#define ID_KILLER_PREVIEW_BTN 86
#define ID_KILLER_BUY 87
#define ID_KILLER_ACTIVATE 88
#define TIMER_KILLER_PREVIEW_BUILD 13960

namespace {
const int ROWS_PER_PAGE = 6;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 500;
const unsigned int INVALID_PREVIEW_OBJECT = 0xFFFFFFFFu;
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

bool PrepareKillerRenderPreview(CIFRenderStatic* render)
{
    if (!render || !g_pMyPlayerObj || !g_pMyPlayerObj->m_pCCObjAnimation) {
        return false;
    }

    render->TB_Func_13("interface\\mall\\mall_charac_frame.ddj", 1, 0);
    render->Clear();
    render->ShowGWnd(true);
    render->BringToFront();
    return true;
}

bool PlayKillerRenderPreview(CIFRenderStatic* render, unsigned int previewObject, int animationId)
{
    if (!render || previewObject == INVALID_PREVIEW_OBJECT ||
        !KillerAnimationPlayer::IsValidAnimationId(animationId)) {
        return false;
    }

    CCObjCharacter* animationObject = render->GetCharacterAnimationObject(previewObject);
    return animationObject &&
        KillerAnimationPlayer::PlayOnAnimationObject(animationObject, animationId, 0, 900);
}

bool BuildKillerRenderPreview(CIFRenderStatic* render, int animationId, unsigned int* previewObject)
{
    if (!render || !g_pMyPlayerObj || !g_pMyPlayerObj->GetCommonData() || !g_pMyPlayerObj->m_pCCObjAnimation) {
        return false;
    }

    unsigned int object = 0;
    CCObjCharacter* sourceAnimation = g_pMyPlayerObj->m_pCCObjAnimation;

    __try {
        object = render->GetCharacterObj(g_pMyPlayerObj->GetCommonData()->RefObjectId, sourceAnimation);
        render->FUN_005602c0(object, 0);
        render->Test1();

        undefined4 renderArg = 0;
        render->Test(&renderArg);
        render->Test2(&renderArg, 0x420d1000);
        render->field_0x430 = 0x43fa0000;
        render->Test3(0, 0);
        render->yukariasagi = 15.000f;
        render->N00000609 = 1;
        render->ShowGWnd(true);
        render->BringToFront();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!render->GetCharacterAnimationObject(object)) {
        return false;
    }

    if (previewObject) {
        *previewObject = object;
    }
    return true;
}

const wchar_t* PaymentName(byte paymentType)
{
    return paymentType == 1
        ? KmtGetText(L"UIIT_KMT_GOLD")
        : KmtGetText(L"UIIT_KMT_SILK");
}
}

GFX_IMPLEMENT_DYNCREATE(CIFKillerAnimationWnd, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFKillerAnimationWnd, CIFMainFrame)
                    ONG_COMMAND(ID_KILLER_ROW_0, &CIFKillerAnimationWnd::OnRow0)
                    ONG_COMMAND(ID_KILLER_ROW_0 + 1, &CIFKillerAnimationWnd::OnRow1)
                    ONG_COMMAND(ID_KILLER_ROW_0 + 2, &CIFKillerAnimationWnd::OnRow2)
                    ONG_COMMAND(ID_KILLER_ROW_0 + 3, &CIFKillerAnimationWnd::OnRow3)
                    ONG_COMMAND(ID_KILLER_ROW_0 + 4, &CIFKillerAnimationWnd::OnRow4)
                    ONG_COMMAND(ID_KILLER_ROW_0 + 5, &CIFKillerAnimationWnd::OnRow5)
                    ONG_COMMAND(ID_KILLER_PREV, &CIFKillerAnimationWnd::OnPrevPage)
                    ONG_COMMAND(ID_KILLER_NEXT, &CIFKillerAnimationWnd::OnNextPage)
                    ONG_COMMAND(ID_KILLER_PREVIEW_BTN, &CIFKillerAnimationWnd::OnPreview)
                    ONG_COMMAND(ID_KILLER_BUY, &CIFKillerAnimationWnd::OnBuy)
                    ONG_COMMAND(ID_KILLER_ACTIVATE, &CIFKillerAnimationWnd::OnActivate)
GFX_END_MESSAGE_MAP()

GFX_IMPLEMENT_DYNCREATE(CIFKillerAnimationGuide, CIFDecoratedStatic)

CIFKillerAnimationWnd::CIFKillerAnimationWnd()
    : m_background(0), m_headerBackground(0), m_listBackground(0), m_previewBackground(0),
      m_listTitle(0), m_previewSectionTitle(0), m_renderPreview(0),
      m_previewTitle(0), m_statusLabel(0), m_pageLabel(0), m_prevButton(0), m_nextButton(0),
      m_previewButton(0), m_buyButton(0), m_activateButton(0), m_currentPage(0),
      m_selectedIndex(-1), m_pendingAnimationId(0), m_previewAnimationId(0),
      m_previewObject(INVALID_PREVIEW_OBJECT),
      m_requestTick(0), m_lastPreviewTick(0),
      m_waitingForResult(false)
{
    for (int i = 0; i < ROWS_PER_PAGE; ++i) {
        m_rows[i] = 0;
        m_rowStatus[i] = 0;
        m_rowPrice[i] = 0;
    }
}

CIFKillerAnimationWnd::~CIFKillerAnimationWnd() {
}

bool CIFKillerAnimationWnd::OnCreate(long ln)
{
    CIFMainFrame::OnCreate(ln);

    TB_Func_13("interface\\frame\\mall_sub_wnd04_", 0, 0);
    SetText(KmtGetText(L"UIIT_KMT_KILLER_ANIMATION_STUDIO"));

    RECT bgRect = {0, 31, WINDOW_WIDTH, WINDOW_HEIGHT - 31};
    m_background = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), bgRect, ID_KILLER_BG, 0);
    if (m_background) {
        m_background->TB_Func_13("clientlibrary\\mall\\win_bg.ddj", 0, 0);
        m_background->SetClickable(false);
        m_background->ShowGWnd(true);
    }

    RECT headerRect = {0, 31, WINDOW_WIDTH, 45};
    m_headerBackground = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), headerRect, ID_KILLER_HEADER_BG, 0);
    if (m_headerBackground) {
        m_headerBackground->TB_Func_13("clientlibrary\\mall\\header.ddj", 0, 0);
        m_headerBackground->SetClickable(false);
        m_headerBackground->ShowGWnd(true);
    }

    RECT listRect = {20, 82, 334, 350};
    m_listBackground = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), listRect, ID_KILLER_LIST_BG, 0);
    if (m_listBackground) {
        m_listBackground->TB_Func_13("clientlibrary\\mall\\leftbg.ddj", 0, 0);
        m_listBackground->SetClickable(false);
        m_listBackground->ShowGWnd(true);
    }

    RECT listTitleRect = {36, 88, 300, 24};
    m_listTitle = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), listTitleRect, ID_KILLER_LIST_TITLE, 0);
    StyleStatic(m_listTitle, KmtGetText(L"UIIT_KMT_ANIMATION_COLLECTION"), COLOR_ACCENT, CTextBoard::JUSTIFY_LEFT);

    for (int i = 0; i < ROWS_PER_PAGE; ++i) {
        const int y = 119 + (i * 49);
        RECT rowRect = {36, y, 206, 40};
        m_rows[i] = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), rowRect, ID_KILLER_ROW_0 + i, 0);
        StyleButton(m_rows[i], L"", "clientlibrary\\title\\title_list.ddj");

        RECT statusRect = {250, y + 3, 88, 16};
        m_rowStatus[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_KILLER_ROW_STATUS + i, 0);
        StyleStatic(m_rowStatus[i], L"", COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);

        RECT priceRect = {250, y + 21, 88, 16};
        m_rowPrice[i] = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), priceRect, ID_KILLER_ROW_PRICE + i, 0);
        StyleStatic(m_rowPrice[i], L"", COLOR_ACCENT, CTextBoard::JUSTIFY_CENTER);
    }

    RECT previewBgRect = {366, 82, 414, 350};
    m_previewBackground = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), previewBgRect, ID_KILLER_PREVIEW_BG, 0);
    if (m_previewBackground) {
        m_previewBackground->TB_Func_13("clientlibrary\\mall\\mall_pre_start.ddj", 0, 0);
        m_previewBackground->SetClickable(false);
        m_previewBackground->ShowGWnd(true);
    }

    RECT previewSectionRect = {386, 88, 374, 18};
    m_previewSectionTitle = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), previewSectionRect, ID_KILLER_PREVIEW_SECTION_TITLE, 0);
    StyleStatic(m_previewSectionTitle, KmtGetText(L"UIIT_KMT_LIVE_CHARACTER_PREVIEW"), COLOR_ACCENT, CTextBoard::JUSTIFY_CENTER);

    RECT previewTitleRect = {392, 108, 362, 22};
    m_previewTitle = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), previewTitleRect, ID_KILLER_PREVIEW_TITLE, 0);
    StyleStatic(m_previewTitle, KmtGetText(L"UIIT_KMT_SELECT_AN_ANIMATION"), COLOR_SELECTED, CTextBoard::JUSTIFY_CENTER);

    RECT renderRect = {392, 132, 362, 242};
    m_renderPreview = (CIFRenderStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFRenderStatic), renderRect, ID_KILLER_RENDER, 0);
    if (m_renderPreview) {
        m_renderPreview->SetClickable(false);
        m_renderPreview->TB_Func_13("interface\\mall\\mall_charac_frame.ddj", 1, 0);
        m_renderPreview->ShowGWnd(false);
    }

    RECT previewBtnRect = {388, 389, 112, 32};
    m_previewButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), previewBtnRect, ID_KILLER_PREVIEW_BTN, 0);
    StyleButton(m_previewButton, KmtGetText(L"UIIT_KMT_PREVIEW"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    RECT buyRect = {517, 389, 112, 32};
    m_buyButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), buyRect, ID_KILLER_BUY, 0);
    StyleButton(m_buyButton, KmtGetText(L"UIIT_KMT_PURCHASE"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    RECT activateRect = {646, 389, 112, 32};
    m_activateButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), activateRect, ID_KILLER_ACTIVATE, 0);
    StyleButton(m_activateButton, KmtGetText(L"UIIT_KMT_ACTIVATE"), "clientlibrary\\mall\\mall_pre_big_button.ddj");

    RECT prevRect = {120, 440, 12, 42};
    m_prevButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), prevRect, ID_KILLER_PREV, 0);
    if (m_prevButton) {
        m_prevButton->TB_Func_13("clientlibrary\\mall\\mall_pre_left_button.ddj", 1, 1);
        m_prevButton->ShowGWnd(true);
    }

    RECT pageRect = {144, 449, 76, 24};
    m_pageLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), pageRect, ID_KILLER_PAGE, 0);
    StyleStatic(m_pageLabel, L"1 / 1", COLOR_LABEL, CTextBoard::JUSTIFY_CENTER);

    RECT nextRect = {232, 440, 12, 42};
    m_nextButton = (CIFButton*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), nextRect, ID_KILLER_NEXT, 0);
    if (m_nextButton) {
        m_nextButton->TB_Func_13("clientlibrary\\mall\\mall_pre_right_button.ddj", 1, 1);
        m_nextButton->ShowGWnd(true);
    }

    RECT statusRect = {286, 449, 474, 24};
    m_statusLabel = (CIFStatic*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), statusRect, ID_KILLER_STATUS, 0);
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

void CIFKillerAnimationWnd::ShowGWnd(bool bVisible)
{
    CIFMainFrame::ShowGWnd(bVisible);
    if (bVisible) {
        UpdateWindowPos();
        RequestAnimations();
        RefreshAnimations();
        if (GetSelectedAnimationIndex() >= 0) {
            RefreshPreview(true);
        }
        BringToFront();
    }
}

void CIFKillerAnimationWnd::OnUpdate()
{
    CIFMainFrame::OnUpdate();

    if (m_waitingForResult && GetTickCount() - m_requestTick > 7000) {
        m_waitingForResult = false;
        m_pendingAnimationId = 0;
        if (m_statusLabel) {
            m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_REQUEST_TIMED_OUT_PLEASE_TRY_AGAIN"));
            m_statusLabel->m_FontTexture.SetColor(COLOR_WARNING);
        }
        RefreshButtons();
    }

}

void CIFKillerAnimationWnd::OnTimer(int timerId)
{
    if (timerId == TIMER_KILLER_PREVIEW_BUILD) {
        KillTimer(TIMER_KILLER_PREVIEW_BUILD);
        m_previewObject = INVALID_PREVIEW_OBJECT;
        if (IsVisible() && BuildKillerRenderPreview(m_renderPreview, m_previewAnimationId, &m_previewObject)) {
            PlayKillerRenderPreview(m_renderPreview, m_previewObject, m_previewAnimationId);
            m_lastPreviewTick = GetTickCount();
        }
        return;
    }

    CIFMainFrame::OnTimer(timerId);
}

undefined1 CIFKillerAnimationWnd::OnCloseWnd()
{
    KillTimer(TIMER_KILLER_PREVIEW_BUILD);
    ShowGWnd(false);
    return true;
}

void CIFKillerAnimationWnd::UpdateWindowPos()
{
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

void CIFKillerAnimationWnd::RequestAnimations()
{
    CMsgStreamBuffer request(0x169A);
    request << byte(37);
    SendMsg(request);
}

void CIFKillerAnimationWnd::RefreshAnimations()
{
    const int count = (int)m_CustomDataManager->KillerAnimations.size();
    int pageCount = (count + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
    if (pageCount < 1) {
        pageCount = 1;
    }
    if (m_currentPage >= pageCount) {
        m_currentPage = pageCount - 1;
    }

    for (int i = 0; i < ROWS_PER_PAGE; ++i) {
        const int index = (m_currentPage * ROWS_PER_PAGE) + i;
        if (index >= 0 && index < count) {
            const CustomDataManager::KillerAnimation& animation = m_CustomDataManager->KillerAnimations[index];
            if (m_rows[i]) {
                wchar_t name[128];
                swprintf(name, 128, L"%ls", animation.DisplayName.c_str());
                m_rows[i]->SetText(name);
                m_rows[i]->m_FontTexture.SetColor(index == m_selectedIndex ? COLOR_SELECTED : COLOR_TEXT);
                m_rows[i]->SetEnabledState(!m_waitingForResult);
                m_rows[i]->ShowGWnd(true);
            }
            if (m_rowStatus[i]) {
                m_rowStatus[i]->SetText(animation.IsActive ? KmtGetText(L"UIIT_KMT_ACTIVE") : (animation.Owned ? KmtGetText(L"UIIT_KMT_OWNED") : KmtGetText(L"UIIT_KMT_FOR_SALE")));
                m_rowStatus[i]->m_FontTexture.SetColor(animation.IsActive
                    ? COLOR_ACTIVE
                    : (animation.Owned ? COLOR_LABEL : COLOR_MUTED));
                m_rowStatus[i]->ShowGWnd(true);
            }
            if (m_rowPrice[i]) {
                wchar_t price[64];
                if (animation.Owned) {
                    swprintf(price, 64, L"");
                } else {
                    swprintf(
                        price,
                        64,
                        KmtGetText(L"UIIT_KMT_VALUE_TEXT"),
                        animation.Price,
                        PaymentName(animation.PaymentType));
                }
                m_rowPrice[i]->SetText(price);
                m_rowPrice[i]->m_FontTexture.SetColor(index == m_selectedIndex ? COLOR_ACCENT : COLOR_LABEL);
                m_rowPrice[i]->ShowGWnd(true);
            }
        } else {
            if (m_rows[i]) {
                m_rows[i]->SetText(L"");
                m_rows[i]->SetEnabledState(false);
                m_rows[i]->ShowGWnd(false);
            }
            if (m_rowStatus[i]) {
                m_rowStatus[i]->SetText(L"");
                m_rowStatus[i]->ShowGWnd(false);
            }
            if (m_rowPrice[i]) {
                m_rowPrice[i]->SetText(L"");
                m_rowPrice[i]->ShowGWnd(false);
            }
        }
    }

    if (m_pageLabel) {
        wchar_t page[64];
        swprintf(page, 64, L"%d / %d", m_currentPage + 1, pageCount);
        m_pageLabel->SetText(page);
    }
    if (m_prevButton) m_prevButton->SetEnabledState(m_currentPage > 0 && !m_waitingForResult);
    if (m_nextButton) m_nextButton->SetEnabledState(m_currentPage + 1 < pageCount && !m_waitingForResult);

    if (m_selectedIndex < 0 && count > 0) {
        m_selectedIndex = 0;
        RefreshPreview(true);
    } else if (m_selectedIndex >= count) {
        m_selectedIndex = count - 1;
        RefreshPreview(true);
    }

    RefreshButtons();
}

void CIFKillerAnimationWnd::SelectRow(int row)
{
    int index = (m_currentPage * ROWS_PER_PAGE) + row;
    if (index < 0 || index >= (int)m_CustomDataManager->KillerAnimations.size()) {
        return;
    }

    m_selectedIndex = index;
    RefreshAnimations();
    RefreshPreview(true);
    CGEffSoundBody::get()->PlaySound(L"snd_window_open");
}

void CIFKillerAnimationWnd::OnRow0() { SelectRow(0); }
void CIFKillerAnimationWnd::OnRow1() { SelectRow(1); }
void CIFKillerAnimationWnd::OnRow2() { SelectRow(2); }
void CIFKillerAnimationWnd::OnRow3() { SelectRow(3); }
void CIFKillerAnimationWnd::OnRow4() { SelectRow(4); }
void CIFKillerAnimationWnd::OnRow5() { SelectRow(5); }

void CIFKillerAnimationWnd::OnPrevPage()
{
    if (m_currentPage > 0) {
        --m_currentPage;
        RefreshAnimations();
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}

void CIFKillerAnimationWnd::OnNextPage()
{
    const int count = (int)m_CustomDataManager->KillerAnimations.size();
    int pageCount = (count + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
    if (pageCount < 1) {
        pageCount = 1;
    }
    if (m_currentPage + 1 < pageCount) {
        ++m_currentPage;
        RefreshAnimations();
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
    }
}

void CIFKillerAnimationWnd::OnPreview()
{
    RefreshPreview(true);
}

void CIFKillerAnimationWnd::OnBuy()
{
    int animationId = GetSelectedAnimationId();
    if (animationId <= 0 || m_waitingForResult) {
        return;
    }

    m_waitingForResult = true;
    m_pendingAnimationId = animationId;
    m_requestTick = GetTickCount();
    if (m_statusLabel) {
        m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_SENDING_PURCHASE_REQUEST"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    RefreshButtons();

    CMsgStreamBuffer packet(0x169A);
    packet << byte(38);
    packet << INT32(animationId);
    SendMsg(packet);
}

void CIFKillerAnimationWnd::OnActivate()
{
    int animationId = GetSelectedAnimationId();
    if (animationId <= 0 || m_waitingForResult) {
        return;
    }

    m_waitingForResult = true;
    m_pendingAnimationId = animationId;
    m_requestTick = GetTickCount();
    if (m_statusLabel) {
        m_statusLabel->SetText(KmtGetText(L"UIIT_KMT_ACTIVATING_ANIMATION"));
        m_statusLabel->m_FontTexture.SetColor(COLOR_ACCENT);
    }
    RefreshButtons();

    CMsgStreamBuffer packet(0x169A);
    packet << byte(39);
    packet << INT32(animationId);
    SendMsg(packet);
}

void CIFKillerAnimationWnd::RefreshPreview(bool restartAnimation)
{
    const int index = GetSelectedAnimationIndex();
    if (index < 0) {
        return;
    }

    const CustomDataManager::KillerAnimation& animation = m_CustomDataManager->KillerAnimations[index];
    if (m_previewTitle) {
        m_previewTitle->SetText(animation.DisplayName.c_str());
    }

    m_previewAnimationId = restartAnimation ? animation.AnimationID : 0;
    if (restartAnimation && PlayKillerRenderPreview(m_renderPreview, m_previewObject, m_previewAnimationId)) {
        m_renderPreview->ShowGWnd(true);
        m_renderPreview->BringToFront();
        m_lastPreviewTick = GetTickCount();
        return;
    }

    m_previewObject = INVALID_PREVIEW_OBJECT;
    if (PrepareKillerRenderPreview(m_renderPreview)) {
        m_lastPreviewTick = GetTickCount();
        KillTimer(TIMER_KILLER_PREVIEW_BUILD);
        StartTimer(TIMER_KILLER_PREVIEW_BUILD, 250);
    }
}

void CIFKillerAnimationWnd::RefreshButtons()
{
    const int index = GetSelectedAnimationIndex();
    const bool hasSelection = index >= 0;
    bool owned = false;
    bool active = false;
    if (hasSelection) {
        const CustomDataManager::KillerAnimation& animation = m_CustomDataManager->KillerAnimations[index];
        owned = animation.Owned;
        active = animation.IsActive;
    }

    if (m_previewButton) {
        m_previewButton->SetText(KmtGetText(L"UIIT_KMT_PREVIEW"));
        m_previewButton->SetEnabledState(hasSelection && !m_waitingForResult);
    }
    if (m_buyButton) {
        m_buyButton->SetText(owned ? KmtGetText(L"UIIT_KMT_OWNED") : KmtGetText(L"UIIT_KMT_PURCHASE"));
        m_buyButton->SetEnabledState(hasSelection && !owned && !m_waitingForResult);
    }
    if (m_activateButton) {
        m_activateButton->SetText(active ? KmtGetText(L"UIIT_KMT_ACTIVE") : KmtGetText(L"UIIT_KMT_ACTIVATE"));
        m_activateButton->SetEnabledState(hasSelection && owned && !active && !m_waitingForResult);
    }
}

int CIFKillerAnimationWnd::GetSelectedAnimationIndex() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_CustomDataManager->KillerAnimations.size()) {
        return -1;
    }
    return m_selectedIndex;
}

int CIFKillerAnimationWnd::GetSelectedAnimationId() const
{
    const int index = GetSelectedAnimationIndex();
    if (index < 0) {
        return 0;
    }
    return m_CustomDataManager->KillerAnimations[index].ID;
}

void CIFKillerAnimationWnd::HandleActionResult(bool success, byte action, int animationId, const wchar_t* message)
{
    m_waitingForResult = false;
    m_pendingAnimationId = 0;

    if (m_statusLabel) {
        m_statusLabel->SetText(message ? message : L"");
        m_statusLabel->m_FontTexture.SetColor(success
            ? COLOR_ACTIVE
            : COLOR_WARNING);
    }

    RequestAnimations();
    RefreshAnimations();
    CGEffSoundBody::get()->PlaySound(success ? L"snd_window_open" : L"snd_window_close");
}

bool CIFKillerAnimationGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\kmt_killer_animation_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\kmt_killer_animation_2.ddj");
    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFStatic* label = m_IRM.GetResObj<CIFStatic>(1, 0);
    if (label) {
        label->SetTooltip(KmtGetText(L"UIIT_KMT_KILLER_ANIMATIONS"));
        label->SetStyleThingy(TOOLTIP);
    }
    return true;
}

int CIFKillerAnimationGuide::OnMouseLeftUp(int a1, int x, int y)
{
    if (!g_pCGInterface) {
        return 0;
    }

    CIFKillerAnimationWnd* window = g_pCGInterface->GetGuiFromList<CIFKillerAnimationWnd>(KILLER_ANIMATION_WINDOW_ID);
    if (!window) {
        return 0;
    }

    window->ShowGWnd(!window->IsVisible());
    CGEffSoundBody::get()->PlaySound(window->IsVisible() ? L"snd_window_open" : L"snd_window_close");
    return 0;
}

void CIFKillerAnimationGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}
