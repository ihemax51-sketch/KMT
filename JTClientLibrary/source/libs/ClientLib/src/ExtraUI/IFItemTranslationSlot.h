#pragma once
#include <SRIFLib/IFSlotWithHelpEx.h>
#include "IFWnd.h"
#include "unsorted.h"
class CIFItemTranslationSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFItemTranslationSlot)
GFX_DECLARE_MESSAGE_MAP(CIFItemTranslationSlot)
public:
    CIFItemTranslationSlot(void);

    ~CIFItemTranslationSlot(void);

    bool OnCreate(long ln) override;
    void ClearSlot();
    void OnUpdate() override;


    void Func_26(int a1) override;

    bool Func_28(CGWnd* a1, int a2, int a3) override;

    void RenderMyself() override;

    void UpdatehgWndSlot(CIFSlotWithHelp *pSlot);


    void LoadSlot(int SlotSetq, int SlotType, int Data);
    //bool Func_25(int a1) override;


public:
    CIFSlotWithHelpEx* m_pMySlot;
    CIFSlotWithHelp* copyslot;
    int m_nMagicCubeSlotGID;
bool IsChineseWeapon(TypeId TID);
};