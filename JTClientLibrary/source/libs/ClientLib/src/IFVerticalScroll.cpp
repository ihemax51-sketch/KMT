#include "IFVerticalScroll.h"
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFVerticalScroll, 0x00ee9968)
void CIFVerticalScroll::sub_65A390(RECT a2) {
    m_field_0x3a8 = 0;

    FUN_006590e0(a2.left);

    FUN_006592a0(a2.top, a2.right, a2.bottom);

    wnd_rect rectThis = GetBounds();
    wnd_rect scrollRect = m_scrollButton->GetBounds();
    m_scrollButton->MoveGWnd(scrollRect.pos.x, rectThis.pos.y);

    m_field_0x37c = rectThis.pos.y;
    m_arrowDownButton->MoveGWnd(rectThis.pos.x, a2.left + 16 + rectThis.pos.y);
    OnCIFReady();
}

void CIFVerticalScroll::sub_65A500(short a2) {
    m_field_0x3a8 = 0;
    FUN_006590e0(a2);
    wnd_rect rectThis = this->GetBounds();
    wnd_rect rectHandle = m_scrollButton->GetBounds();
    m_scrollButton->MoveGWnd(rectHandle.pos.x, rectThis.pos.y);

    m_field_0x37c = rectThis.pos.y;
    m_arrowDownButton->MoveGWnd(rectThis.pos.x, a2 + 16 + rectThis.pos.y);
    OnCIFReady();
}

void CIFVerticalScroll::sub_65A5A0(int a2) {
    if (a2 == 0) {
        a2 = m_field_0x3a0;
    }
    FUN_00659150(m_CurrentIndex - a2);
}

void CIFVerticalScroll::sub_65A5C0(int a2) {
    if (a2 == 0) {
        a2 = m_field_0x3a0;
    }
    FUN_00659150(m_CurrentIndex + a2);
}
