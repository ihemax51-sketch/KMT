///******************************************************************************
/// \File IFExtQuickSlotSlot.cpp
///
/// \Desc
///
/// \Author kyuubi09 on 2/16/2022.
///
/// \Copyright Copyright © 2023 SRO_DevKit.
///
///******************************************************************************

#include "IFExtQuickSlotSlot.h"
#include "BSLib/Debug.h"
#include "unsorted.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFExtQuickSlotSlot, 0x00ee9a68)

GFX_IMPLEMENT_DYNCREATE_FN(CIFExtQuickSlotSlot, CIFWnd)

CIFExtQuickSlotSlot::CIFExtQuickSlotSlot() :
        m_nMagicCubeSlotGID(0) {
    SetDragable(true);
}

bool CIFExtQuickSlotSlot::OnCreate(long ln) {
    return true;
}

void CIFExtQuickSlotSlot::RenderMyself() {
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

bool CIFExtQuickSlotSlot::Func_28(CGWnd *a1, int a2, int a3) {
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        return pWnd->Func_28(a1, a2, a3);;

    return this->Func_28(a1, a2, a3);
}

void CIFExtQuickSlotSlot::OnGfxMessage(Event3D *pEvent) {
    reinterpret_cast<void (__thiscall *)(CIFExtQuickSlotSlot *, Event3D *)>(0x0065fb90)(this, pEvent);
}

bool CIFExtQuickSlotSlot::Func_25(int a1) {
    return reinterpret_cast<bool (__thiscall *)(CIFExtQuickSlotSlot *, int)>(0x0065fbf0)(this, a1);
}

int CIFExtQuickSlotSlot::OnMouseRightDown(int a1, int x, int y) {
    return reinterpret_cast<bool (__thiscall *)(CIFExtQuickSlotSlot *, int, int, int)>(0x0065fc10)(this, a1, x, y);
}

int CIFExtQuickSlotSlot::OnMouseRightUp(int a1, int x, int y) {
    return reinterpret_cast<bool (__thiscall *)(CIFExtQuickSlotSlot *, int, int, int)>(0x0065fc20)(this, a1, x, y);
}

void CIFExtQuickSlotSlot::UpdatehgWndSlot(CIFWnd *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(false);
}

CIFSlotWithHelp *CIFExtQuickSlotSlot::GetSlot() const {
    return (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
}
