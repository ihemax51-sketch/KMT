#pragma once
#include <SRIFLib/IFSlotWithHelpEx.h>
#include "IFWnd.h"
#include "unsorted.h"
class CIFItemLockSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFItemLockSlot)
GFX_DECLARE_MESSAGE_MAP(CIFItemLockSlot)
public:
    CIFItemLockSlot(void);

    ~CIFItemLockSlot(void);

    bool OnCreate(long ln) override;
    void ClearSlot();
    void OnUpdate() override;
    void SendPacketMoveItem(BYTE SLot, BYTE SlotTo);

    void Func_26(int a1) override;


    int OnMouseLeftUp(int a1, int x, int y) override;
    int OnMouseLeftDown(int a1, int x, int y) override;

    bool Func_28(CGWnd* a1, int a2, int a3) override;

    void RenderMyself() override;

    void UpdatehgWndSlot(CIFSlotWithHelp *pSlot);


    void LoadSlot(int SlotSetq, int SlotType, int Data);
    //bool Func_25(int a1) override;
    void UseItem(int Slot);

public:
    CIFSlotWithHelpEx* m_pMySlot;
    CIFSlotWithHelp *copyslot;
    int m_nMagicCubeSlotGID;


};