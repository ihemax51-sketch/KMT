//
// Created by Kurama on 2/2/2023.
//
#include "NIFWnd.h"
#include "GNFontChar.h"

#include "Game.h"

#include "GFX3DFunction/RStateMgr.h"
#include "SimpleViewer/VBDynamic.h"

#define GDR_STATIC_TOOLTIP_ID 100000

GFX_IMPLEMENT_DYNAMIC_EXISTING(CNIFWnd, 0x0115dea0)

bool CNIFWnd::OnCreate(long ln) { return true; }

bool CNIFWnd::OnRelease() { return true; }

void CNIFWnd::UpdateFontCoordinates() {}

void CNIFWnd::Func_45() {}

void CNIFWnd::DisableMouseActions(DWORD dwState) {
    m_dwEnableMouseActions = dwState;
}

void CNIFWnd::SetTooltipVerticalMode(DWORD dwMode) {
    m_dwTooltipValignTop = dwMode;
}

void CNIFWnd::Func_49() {}

void CNIFWnd::SetTooltipOwner(CNIFWnd *pOwner) {
#if 0
    g_pNewInterfaceToolTips->m_pOwnerWnd = pOwner;
#endif
}

GFX_BEGIN_MESSAGE_MAP(CNIFWnd, CObjChild)
                    ONG_CREATE()
                    ONG_MOVE()
GFX_END_MESSAGE_MAP()

CNIFWnd::CNIFWnd() {
    m_fAniFadeCurrentTime = 0.0f;
    m_bMineAlphaScaleUse = false;
    m_fAniFadeTime = 0.0f;
    m_btCurrentTextureAlpha = 255;

    m_dwEnableMouseActions = TRUE;

    m_dwTextureBGColor = D3DCOLOR_RGBA(0, 0, 0, 0);
    m_dwTextureFGColor = D3DCOLOR_RGBA(0, 0, 0, 0);

    m_dwStyleOptionBit = 0;

    void *pFontData = g_sNFontDataVec[0];
    m_pFontTextData = pFontData;
    m_NFontTexture.m_pFontData = pFontData;
    CNIFWnd::SetText(L"");

    m_pRenderTexture = NULL;
    m_NFontTexture.SetColor(D3DCOLOR_RGBA(255, 255, 255, 255));
    m_fRenderColorRight = 1.0f;
    m_fRenderColorDown = 1.0f;

    m_dwIsAniFadeIn = FALSE;

    ///       \Y
    //        â†‘
    //        |
    // (0, 1) |  ______________________ (1, 1)
    //        | |                      |
    //        | |                      |
    //        | |                      |
    //        | |                      |
    //        | |                      |
    //        | |                      |
    // (0, 0) | |______________________| (1, 0)
    ///       -------------------------------------â†’  \X

    m_vTextureUV[0].x = 0.0f;
    m_dwTooltipValignTop = FALSE;
    m_vTextureUV[0].y = 0.0f;
    m_rcTextTexturePos.bottom = 0;
    m_vTextureUV[1].y = 0.0f;
    m_rcTextTexturePos.left = 0;
    m_vTextureUV[3].x = 0.0f;
    m_rcTextTexturePos.right = 0;
    m_rcTextTexturePos.top = 0;
    m_vTextureUV[1].x = 1.0f;
    m_vTextureUV[2].x = 1.0f;
    m_vTextureUV[2].y = 1.0f;
    m_vTextureUV[3].y = 1.0f;
    m_vUnkCords_3.z = 1.0f;
    OptimizeTextureHeadPos();

    m_blWndDragMode = false;
    m_bEdgesHelperVisibleState = false;
    m_dwWndType = -1;
    m_pParentOwner = NULL;
    field_0344 = 0;

    m_strParentsId = "";
    m_cdwParentsId = 0;

   // reinterpret_cast<CNIFWnd*(__thiscall*)(CNIFWnd*)>(0x0046c3d0)(this);
}

CNIFWnd::~CNIFWnd() {
    EraseEdgesHelper();

    if (g_pNewInterfaceToolTips != NULL) {
        g_pNewInterfaceToolTips->EraseWindowObj();
        g_pNewInterfaceToolTips = NULL;
    }
  //  reinterpret_cast<CNIFWnd*(__thiscall*)(CNIFWnd*)>(0x0046c620)(this);
}

int CNIFWnd::OnCreatedInstance(UINT lParam, UINT wParam) {
    UpdateTexturesCoordinates();
    return 1;
}

int CNIFWnd::OnWindowPosChanged(UINT lParam, UINT wParam) {
    UpdateTexturesCoordinates();
    return 1;
}

bool CNIFWnd::OnWndMessage(const Event3D *pEventData) {
    // if there is a slot in our mouse and right click is down
    if (pEventData->Msg == WM_RBUTTONDOWN) {
        SetDraggedGWnd(NULL);
        return true;
    }

    if (pEventData->Msg != WM_LBUTTONDOWN)
        return false;

    if (g_pCurrentEventCtrl != NULL) {
        if (g_pCurrentEventCtrl == g_pOnMouseDownClickCtrl)
            return false;

        CGWnd *pGWnd = (CGWnd *) g_pCurrentEventCtrl;
        for (int i = 0; pGWnd != NULL; i++) {
            if (pGWnd->Func_28(0, 0, i))
                break;

            pGWnd = (CGWnd *) pGWnd->GetParentControl();
        }
    }
    SetDraggedGWnd(NULL);
    return true;
}

bool CNIFWnd::OnMouseEvent(Event3D *pEventData) {
    int nReturn = FALSE;
    switch (pEventData->Msg) {
        case WM_MOUSEMOVE:
            nReturn = this->OnMouseMove(0, pEventData->lParam, pEventData->wParam);
            break;
        case WM_LBUTTONDOWN:
            nReturn = this->OnMouseLeftDown(0, pEventData->lParam, pEventData->wParam);
            break;
        case WM_LBUTTONUP:
            if ((GetKeyState(VK_CONTROL) > -1) && (GetKeyState(VK_SHIFT) > -1) && (GetKeyState(VK_MENU) > -1) &&
                this->IsDragable()) {
                if (g_pOnMouseDownClickCtrl == NULL)
                    SetDraggedGWnd(this);
                return true;
            }
            nReturn = this->OnMouseLeftUp(0, pEventData->lParam, pEventData->wParam);
            break;
        case WM_LBUTTONDBLCLK:
            nReturn = this->OnMouseDbLeftUp(0, pEventData->lParam, pEventData->wParam);
            break;
        case WM_RBUTTONDOWN:
            nReturn = this->OnMouseRightDown(0, pEventData->lParam, pEventData->wParam);
            break;
        case WM_RBUTTONUP:
            nReturn = this->OnMouseRightUp(0, pEventData->lParam, pEventData->wParam);
            break;
        default:
            break;
    }

    if (m_dwEnableMouseActions != FALSE)
        nReturn = TRUE;
    return (nReturn != FALSE);
}

void CNIFWnd::OnGWndDimensionChange() {
    UpdateTexturesCoordinates();
}

BYTE CNIFWnd::GetAlphaScale() {
    if (GetParentControl()->IsKindOf(GFX_RUNTIME_CLASS(CNIFWnd)) && m_bMineAlphaScaleUse)
        return ((((CNIFWnd *) GetParentControl())->GetAlphaScale() * m_btCurrentTextureAlpha) / 255);

    return m_btCurrentTextureAlpha;
}

void CNIFWnd::StartAlphaAnimation(BYTE btAlphaStart, BYTE btAlphaMax, float fTime, float fStartupTime, bool bFadeIn) {
    SetAniFadeData(btAlphaStart, btAlphaMax, fTime, fStartupTime);

    m_dwIsAniFadeIn = bFadeIn;
    if (bFadeIn && btAlphaMax != 0)
        if (!this->IsVisible())
            this->ShowGWnd(true);
}

bool CNIFWnd::IsInside(int nX, int nY) {
    wnd_rect m_rcWnd = this->GetBounds();
    if ((m_rcWnd.pos.x <= nX) && (nY <= m_rcWnd.size.width + m_rcWnd.pos.x)) {
        if (m_rcWnd.pos.y <= nY) {
            return (nY <= m_rcWnd.size.height + m_rcWnd.pos.y);
        }
    }
    return false;
}

bool CNIFWnd::IsOnWindow(wnd_pos posMouse, wnd_rect rcWnd) {
    if ((rcWnd.pos.x <= posMouse.y) && (posMouse.y <= rcWnd.size.width + rcWnd.pos.x)) {
        if (rcWnd.pos.y <= posMouse.y) {
            return (posMouse.y <= rcWnd.size.height + rcWnd.pos.y);
        }
    }
    return false;
}

void CNIFWnd::SetTextPosBounds(const RECT &rc) {
    m_rcTextTexturePos = rc;
}

CGWnd::wnd_rect CNIFWnd::GetFontBounds() const {
    wnd_rect rcFrame = this->GetBounds();

    rcFrame.pos.x += this->m_rcTextTexturePos.left;
    rcFrame.size.width -= (this->m_rcTextTexturePos.right + this->m_rcTextTexturePos.left);
    rcFrame.pos.y += (this->m_rcTextTexturePos).top;
    rcFrame.size.height -= (this->m_rcTextTexturePos.bottom + this->m_rcTextTexturePos.top);

    return rcFrame;
}

bool CNIFWnd::SetText(const wchar_t *pText) {
    // We have SortTextLetters is __usercall
    return reinterpret_cast<bool (__thiscall *)(CNIFWnd *, const wchar_t *)>(0x0046cc80)(this, pText);

#if 0
    if (pText == NULL)
        return false;

    m_wstrInnerText = pText;
    CNIFTextBoard::InstantiateFontText(m_wstrInnerText.c_str());
    return true;
#endif // if 0
}

const wchar_t *CNIFWnd::GetText() {
    return m_wstrInnerText.c_str();
}

void CNIFWnd::OnAniFadeRelease() {
    this->SendMessageToParentDispatcher(GFX_WM_FADERELEASING, 0, 0);

    if (m_dwIsAniFadeIn == TRUE && m_cAniFadeAlphaMax == 0)
        if (this->IsVisible())
            this->ShowGWnd(false);
}

void CNIFWnd::BringToFront() {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *)>(0x0046cd60)(this);

#if 0
    CGWndBase::BringToFront();
    GetNewWndFactoreyPtr()->BringElementsToFront(this);
#endif // if 0
}

void CNIFWnd::OnUpdate() {
    OnAniFading(theApp.GetTimeBase()->AnimationSpeedCorrection);
}

void CNIFWnd::RenderMyself() {
    if (IsStyleThingyShift(INVISIBLE_S))
        return;

    if (m_dwTextureBGColor != D3DCOLOR_RGBA(0, 0, 0, 0))
        TextureGroundColorRender(m_dwTextureBGColor);

    OnRender();

    if (m_dwTextureFGColor != D3DCOLOR_RGBA(0, 0, 0, 0))
        TextureGroundColorRender(m_dwTextureFGColor);
}

void CNIFWnd::EnableToolTipsCtrl() {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *)>(0x0046cdf0)(this);

#if 0
    // Let create it first if we dont have the tooltip
    if (g_pNewInterfaceToolTips == NULL) {
        wnd_rect rcBubbleWnd;
        rcBubbleWnd.size.width = 120;
        rcBubbleWnd.size.height = 120;
        rcBubbleWnd.pos.x = 0;
        rcBubbleWnd.pos.y = 0;

        g_pNewInterfaceToolTips = (CNIFHelperBubbleWindow *) CreateInstance(
                ((CGWnd *) m_pParentOwner->GetParentControl()), GFX_RUNTIME_CLASS(CNIFHelperBubbleWindow), rcBubbleWnd,
                GDR_STATIC_TOOLTIP_ID, 0);
    }

    g_pNewInterfaceToolTips->ShowGWnd(true);
    g_pNewInterfaceToolTips->SetGWndSize(200, 1);
    g_pNewInterfaceToolTips->m_pOwnerWnd = this;
    g_pNewInterfaceToolTips->m_pTextBox->EraseTextBoxLinesList();
    g_pNewInterfaceToolTips->m_pTextBox->EraseTextBoxLinesText();
    g_pNewInterfaceToolTips->SetTooltipText(&m_wstrTooltipText);
    g_pNewInterfaceToolTips->UpdateTooltipSize();
    g_pNewInterfaceToolTips->SetTooltipPos(&(this->GetBounds()), m_dwTooltipValignTop);
    g_pNewInterfaceToolTips->BringToFront();
#endif // if 0
}

void CNIFWnd::OnGfxMessage(Event3D *pEvent) {
    if ((pEvent->Msg == GFX_WM_MOUSEHOVER_DOWN) && IsStyleThingyShift(TOOLTIP_S)) {
        EnableToolTipsCtrl();
        return;
    } else if ((pEvent->Msg == GFX_WM_MOUSEHOVER_UP) && IsStyleThingyShift(TOOLTIP_S)) {
        if (g_pNewInterfaceToolTips == NULL)
            return;

        g_pNewInterfaceToolTips->ShowGWnd(false);
        return;
    }

    GetParentControl()->OnGfxMessage(pEvent);
}

void CNIFWnd::Func_22(int nX, int nY) {
    // ???????
    this->SetGWndSize(nX, nY);
}

void CNIFWnd::SetGWndSize(int nWidth, int nHeight) {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *, int, int)>(0x0046cf70)(this, nWidth, nHeight);

#if 0
    CGWndBase::SetGWndSize(nWidth, nHeight);
    UpdateTexturesCoordinates();
    UpdateEdgesHelperPos();
#endif // if 0
}

void CNIFWnd::MoveGWnd2(CGWndBase::wnd_pos sPos) {
    MoveGWnd(sPos.x, sPos.y);
}

void CNIFWnd::MoveGWnd(int nX, int nY) {
    SetPosition(nX, nY);
    UpdateTexturesCoordinates();
}

void CNIFWnd::UpdateTexturesCoordinates() {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *)>(0x0046cff0)(this);

#if 0
    wnd_rect rcWnd = GetBounds();
    // GG unknown variables name :)
    float fVar5 = (float) rcWnd.pos.x - 0.5f;
    float fVar1 = ((m_fRenderColorRight * ((float) rcWnd.size.width)) - 0.5f);
    float fVar2 = (float) rcWnd.pos.y - 0.5f;
    float fVar3 = ((m_fRenderColorDown * ((float) rcWnd.size.height)) - 0.5f);

    m_vTexturedRenderCords[0].x = fVar5;
    m_vTexturedRenderCords[0].y = fVar2;
    m_vTexturedRenderCords[2].x = fVar1;
    m_vTexturedRenderCords[4].x = fVar1;
    m_vTexturedRenderCords[2].y = fVar2;
    m_vTexturedRenderCords[4].y = fVar3;
    m_vTexturedRenderCords[6].y = fVar3;
    m_vUnkCords_1.y = fVar3;
    m_vTextureColorRenderCords[4].y = fVar3;
    m_vTextureColorRenderCords[6].y = fVar3;
    m_vUnkCords_3.y = fVar3;
    m_vTexturedRenderCords[6].x = fVar5;
    m_vUnkCords_1.x = fVar5;
    m_vTextureColorRenderCords[0].x = fVar5;
    m_vTextureColorRenderCords[6].x = fVar5;
    m_vUnkCords_3.x = fVar5;
    m_vTextureColorRenderCords[0].y = fVar2;
    m_vTextureColorRenderCords[2].y = fVar2;
    m_vTextureColorRenderCords[2].x = fVar1;
    m_vTextureColorRenderCords[4].x = fVar1;

    UpdateFontCoordinates();
#endif // if 0
}

void CNIFWnd::SetTextureHeadPos(D3DXVECTOR2 vTopLeft, D3DXVECTOR2 vTopRight,
                                D3DXVECTOR2 vBottomRight, D3DXVECTOR2 vBottomLeft) {
    m_vTextureUV[0] = vTopLeft;
    m_vTextureUV[1] = vTopRight;
    m_vTextureUV[2] = vBottomLeft;
    m_vTextureUV[3] = vBottomRight;

    OptimizeTextureHeadPos();
}

void CNIFWnd::TextureRender() {
    g_RStateMgr.SetDeviceFVFState(0x104);
    g_RStateMgr.SetTextureForStage(0, m_pRenderTexture);

    int local_158 = 0;
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vTexturedRenderCords, sizeof(m_vTexturedRenderCords), &local_158) !=
        0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_158, 2);
    }
}

void CNIFWnd::FontTextureRender() {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *)>(0x0046d250)(this);

#if 0
    wnd_pos posWnd = GetPos();
    BYTE btAlphaScale = GetAlphaScale();

    m_NFontTexture.RenderAllText(posWnd.x, posWnd.y, btAlphaScale);
#endif // if 0
}

void CNIFWnd::TextureGroundColorRender(DWORD dwColor) {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *, DWORD)>(0x0046d2a0)(this, dwColor);

#if 0
    wnd_rect recBounds = GetBounds();

//    this->m_vTextureColorRenderCords[0].x = (float)extraout_ST1;
//    this->m_vTextureColorRenderCords[0].y = (float)extraout_ST0;
//
//    // Extract the alpha scale from the color
//    this->m_btCurrentTextureAlpha = (dwColor >> 26);
//
//    this->m_vTextureColorRenderCords[2].x = (float)iVar1;
//    this->m_vTextureColorRenderCords[4].x = (float)iVar1;
//    this->m_vTextureColorRenderCords[2].y = (float)extraout_ST0;
//    this->m_vTextureColorRenderCords[4].y = (float)iVar2;
//    this->m_vTextureColorRenderCords[6].y = (float)iVar2;
//    this->m_vTextureColorRenderCords[6].x = (float)extraout_ST1;
//    (this->m_vUnkCords_3).x = (float)extraout_ST1;
//    (this->m_vUnkCords_3).y = (float)extraout_ST0;

    g_RStateMgr.SetDeviceFVFState(0x104);
    g_RStateMgr.SetTextureForStage(0, NULL);

    EnableTextureAlphaOperations(dwColor);
    g_RStateMgr.SetStageOperator<0, D3DTSS_COLOROP>(D3DTOP_SELECTARG1);
    g_RStateMgr.SetStageOperator<0, D3DTSS_COLORARG1>(D3DTA_TFACTOR);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAOP>(D3DTOP_MODULATE);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG1>(D3DTA_TFACTOR);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG2>(D3DTA_DIFFUSE);

    int local_158 = 0;
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vTextureColorRenderCords, sizeof(m_vTextureColorRenderCords),
                                                  &local_158) != 0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_158, 2);
    }
    g_RStateMgr.SetStageOperator<0, D3DTSS_COLOROP>(D3DTOP_SELECTARG1);
    g_RStateMgr.SetStageOperator<0, D3DTSS_COLORARG1>(D3DTA_TEXTURE);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAOP>(D3DTOP_SELECTARG1);
    g_RStateMgr.SetStageOperator<0, D3DTSS_ALPHAARG1>(D3DTA_TEXTURE);
    DefaultTextureAlphaOperations();

    this->m_btCurrentTextureAlpha = (dwColor >> 26);
#endif // if 0
}

void CNIFWnd::OptimizeTextureHeadPos() {
    // what is the pixel doing here?
    m_vTexturedRenderCords[0].z = 1.0f;

    m_vTexturedRenderCords[1].x = 0.1f;
    m_vTexturedRenderCords[3].x = 0.1f;
    m_vTexturedRenderCords[5].x = 0.1f;
    m_vTexturedRenderCords[7].x = 0.1f;
    m_vUnkCords_2.x = 0.1f;

    // weird what is the image pixels doing here?
    m_vTexturedRenderCords[2].z = 1.0f;
    m_vTexturedRenderCords[4].z = 1.0f;
    m_vTexturedRenderCords[6].z = 1.0f;
    m_vUnkCords_1.z = 1.0f;

    m_vTexturedRenderCords[1].y = m_vTextureUV[0].x;
    m_vTexturedRenderCords[1].z = m_vTextureUV[0].y;
    m_vTexturedRenderCords[3].y = m_vTextureUV[1].x;
    m_vTexturedRenderCords[3].z = m_vTextureUV[1].y;
    m_vTexturedRenderCords[5].y = m_vTextureUV[2].x;
    m_vTexturedRenderCords[5].z = m_vTextureUV[2].y;
    m_vTexturedRenderCords[7].y = m_vTextureUV[3].x;
    m_vTexturedRenderCords[7].z = m_vTextureUV[3].y;
    m_vUnkCords_2.y = m_vTextureUV[0].x;
    m_vUnkCords_2.z = m_vTextureUV[0].y;
}

void CNIFWnd::MoveTextureRightVertices(float fX) {
    m_vTexturedRenderCords[3].y = (fX * m_vTextureUV[1].x);
    m_vTexturedRenderCords[3].z = m_vTextureUV[1].y;
    m_vTexturedRenderCords[5].y = (fX * m_vTextureUV[2].x);
    m_vTexturedRenderCords[5].z = m_vTextureUV[2].y;
}

void CNIFWnd::OnGWndShow() {
    this->ShowGWnd(!IsVisible());
}

void CNIFWnd::ShowGWnd(bool bVisible) {
    reinterpret_cast<void (__thiscall *)(CNIFWnd *, bool)>(0x0046d760)(this, bVisible);

#if 0
    if (IsVisible() == bVisible)
        return;

    CGWndBase::ShowGWnd(bVisible);
    UpdateTexturesCoordinates();
    GetNewWndFactoreyPtr()->ShowElements(this);
#endif // if 0
}

void CNIFWnd::SetTextureColorUV(D3DXVECTOR2 vUV) {
    m_fRenderColorRight = vUV.x;
    m_fRenderColorDown = vUV.y;
}

bool CNIFWnd::OnCloseWnd() {
    ShowGWnd(false);
    return true;
}

void CNIFWnd::SetTooltipText(const std::n_wstring *pWStr) {
    m_wstrTooltipText = *pWStr;

    if (IsParentOf(g_CurrentIfUnderCursor) && IsStyleThingyShift(TOOLTIP_S))
        EnableToolTipsCtrl();
}

const std::n_wstring *CNIFWnd::GetTooltipText() const {
    return &m_wstrTooltipText;
}

size_t CNIFWnd::GetTooltipTextLength() const {
    return m_wstrTooltipText.length();
}

DWORD CNIFWnd::GetStyleOption() const {
    return m_dwStyleOptionBit;
}

void CNIFWnd::SetStyleThingy(StyleOptions eOptions) {
    m_dwStyleOptionBit = eOptions;
    UpdateStyle();
}

DWORD CNIFWnd::AddStyleThingy(StyleOptions eOptions) {
    m_dwStyleOptionBit |= eOptions;
    UpdateStyle();

    return m_dwStyleOptionBit;
}

DWORD CNIFWnd::RemoveStyleThingy(StyleOptions eOptions) {
    m_dwStyleOptionBit &= (~eOptions);
    UpdateStyle();

    return m_dwStyleOptionBit;
}

bool CNIFWnd::IsStyleThingy(StyleOptions eStyle) const {
    return ((m_dwStyleOptionBit & eStyle) != FALSE);
}

void CNIFWnd::UseCurrentAlphaScale() {
    m_bMineAlphaScaleUse = true;
}

void CNIFWnd::SetRenderViewport(wnd_rect *pRCWnd) {
    D3DVIEWPORT9 vViewPort;
    vViewPort.MinZ = 0.0f;
    vViewPort.MaxZ = 1.0f;
    vViewPort.X = ((pRCWnd->pos.x <= 0) ? 0 : pRCWnd->pos.x);
    vViewPort.Y = ((pRCWnd->pos.y <= 0) ? 0 : pRCWnd->pos.y);
    vViewPort.Width = (((pRCWnd->pos.x >= 0) ? 0 : pRCWnd->pos.x) + pRCWnd->size.width);
    vViewPort.Height = (((pRCWnd->pos.y >= 0) ? 0 : pRCWnd->pos.y) + pRCWnd->size.height);

    g_RStateMgr.m_pDevice->SetViewport(&vViewPort);
}

#if 0
void CNIFWnd::DefaultRenderViewport() {
    D3DVIEWPORT9 vViewPort;
    vViewPort.MinZ = 0.0f;
    vViewPort.X = 0;
    vViewPort.Y = 0;
    vViewPort.MaxZ = 1.0f;
    vViewPort.Width = g_dwCurrentWindowSize.width;
    vViewPort.Height = g_dwCurrentWindowSize.height;

    g_RStateMgr.m_pDevice->SetViewport(&vViewPort);
}
#endif // if 0

void CNIFWnd::SetTextureBkColor(D3DCOLOR d3dColor) {
    m_dwTextureBGColor = d3dColor;
}

void CNIFWnd::SetTextureFgColor(D3DCOLOR d3dColor) {
    m_dwTextureFGColor = d3dColor;
}

void CNIFWnd::Func_40() {}

void CNIFWnd::SetEdgesHelperColor() {
    std::n_vector<CNIFWnd *>::const_iterator it = m_vecEdgesTextureHelper.begin();
    for (; it != m_vecEdgesTextureHelper.end(); it++)
        (*it)->m_dwTextureBGColor = D3DCOLOR_RGBA(255, 128, 192, 255);
}

void CNIFWnd::InitEdgesHelper() {
    wnd_rect rcEdges;
    rcEdges.size.height = 6;
    rcEdges.size.width = 6;
    rcEdges.pos.y = 0;
    rcEdges.pos.x = 0;

    // 8 Edges
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));
    m_vecEdgesTextureHelper.push_back((CNIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CNIFWnd), rcEdges, 0, 0));

    SetEdgesHelperColor();
#if 0
    UpdateEdgesHelperPos();
    ShowEdgesHelper();
#endif // if 0
}

#if 0
// Alot of mess here
void CNIFWnd::UpdateEdgesHelperPos() {

}
#endif // if 0

void CNIFWnd::EraseEdgesHelper() {
    std::n_vector<CNIFWnd *>::const_iterator it = m_vecEdgesTextureHelper.begin();
    for (; it != m_vecEdgesTextureHelper.end(); it++)
        (*it)->EraseWindowObj();
}

#if 0
void CNIFWnd::ShowEdgesHelper() {

}
#endif // if 0

