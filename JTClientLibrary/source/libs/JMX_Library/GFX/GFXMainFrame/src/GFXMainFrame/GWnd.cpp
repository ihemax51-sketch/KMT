#include "GWnd.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CGWnd, 0x0110F94C)

void CGWnd::Func_24(int a1, int a2) {
    reinterpret_cast<void (__thiscall *)(CGWnd *, int, int)>(0x00B9D810)(this, a1, a2);
}

bool CGWnd::Func_25(int a1) {
    return true;
}

void CGWnd::Func_26(int a1) {
}

bool CGWnd::Func_27() {
    return true;
}

bool CGWnd::Func_28(CGWnd *a1, int a2, int a3) {
    return false;
}

int CGWnd::OnMouseLeftUp(int a1, int x, int y) {
    return 0;
}

int CGWnd::OnMouseLeftDown(int a1, int x, int y) {
    return 0;
}

int CGWnd::OnMouseDbLeftUp(int a1, int nPos, int nDownVK) {
    return 0;
}

int CGWnd::OnMouseRightDown(int a1, int x, int y) {
    return 0;
}

int CGWnd::OnMouseRightUp(int a1, int x, int y) {
    return 0;
}

int CGWnd::Func_34(int a1, int a2, int a3) {
    return 0;
}

int CGWnd::OnMouseMove(int a1, int x, int y) {
    return 0;
}

int CGWnd::Func_36(int a1, short action, int a3, int a4) {
    return 0;
}

void CGWnd::OnGWndDimensionChange() {
    return;
}

CGWnd::CGWnd() {
    m_bVisible = true;
    m_bClickable = true;
    N0000074F = true;
    m_bDragable = false;

    sub_B9DA70(false);
}

CGWnd::~CGWnd() {
    if (g_pOnMouseDownClickCtrl == this)
        g_pOnMouseDownClickCtrl = NULL;

    if (g_pMouseHoldingSlot == this)
        g_pMouseHoldingSlot = NULL;

    if (g_pUnderFocusCtrl == this)
        g_pUnderFocusCtrl = NULL;
}

void CGWnd::OnUpdate() {
    // empty
}

void CGWnd::RenderMyself() {
    if (!m_bVisible)
        return;
    // NOTE: This can trigger an endless recursion if RenderMyself is not properly overwritten
    CGWnd::Render();
}

void CGWnd::RenderMyChildren() {
    reinterpret_cast<void (__thiscall *)(CGWnd *)>(0x00BA0300)(this);
}

void CGWnd::Render() {
    RenderMyself();
    RenderMyChildren();
    Func_15();
}

void CGWnd::Func_15() {
    // empty
}

bool CGWnd::OnMouseEvent(Event3D *pEvent) {
    return false;
}

int CGWnd::GetCurrentEventMsgCtrlId() {
    if (g_pCurrentEventCtrl) {
        return g_pCurrentEventCtrl->UniqueID();
    } else {
        return -1;
    }
}

void CGWnd::EraseWindowObj() {
    reinterpret_cast<void(*)(CGWnd *)>(0x00ba0e40)(this);
}
BOOL CGWnd::EraseWindowObj2(CGWnd *pGWnd) {
    return reinterpret_cast<BOOL (*)(CGWnd *, CGWnd *)>(0x00ba0e40)(this, pGWnd);
}

CGWnd *CGWnd::GetDraggedGWnd() {
    return g_pMouseHoldingSlot;
}

void CGWnd::SetDraggedGWnd(CGWnd *wnd) {
    // Do We already holding the same slot?
    if (g_pMouseHoldingSlot == wnd)
        return;
    // OnClear?
    if (g_pMouseHoldingSlot != (CGWnd *) NULL)
        g_pMouseHoldingSlot->Func_26(0);

    g_pMouseHoldingSlot = wnd;

    if (wnd != NULL && !wnd->Func_25(0))
        g_pMouseHoldingSlot = NULL;
}
