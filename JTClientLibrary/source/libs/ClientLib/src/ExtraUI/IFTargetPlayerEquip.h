#pragma once
#include <SRIFLib/IFSlotWithHelpEx.h>
#include <ICUser.h>
#include "IFWnd.h"
#include "unsorted.h"
class CIFTargetPlayerEquip : public CIFMainFrame {
GFX_DECLARE_DYNCREATE(CIFTargetPlayerEquip)
GFX_DECLARE_MESSAGE_MAP(CIFTargetPlayerEquip)
public:
    CIFTargetPlayerEquip(void);

    ~CIFTargetPlayerEquip(void);

    bool OnCreate(long ln) override;
    undefined1 OnCloseWnd() override;
void UpdateMenuSize();
    void UpdateItemSlot(int ItemSlot, CSOItem* tempItemInfo, std::n_string CN16);
    void UpdateTargetName(const std::n_string& characterName);
    std::n_string TargetCharName;
void ClearSlots();
void UpdateRenderStatic(CICUser * pObject);
    CICUser* pObject;
void OnTimer(int timerId) override;
};
