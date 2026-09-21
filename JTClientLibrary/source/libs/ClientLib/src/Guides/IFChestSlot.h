#pragma once
#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>
#include "IFSlotWithHelp.h"

class CIFChestSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFChestSlot)
GFX_DECLARE_MESSAGE_MAP(CIFChestSlot)
public:
    CIFChestSlot(void);
    ~CIFChestSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void SetName(int FakeNum, int DbID, int ItemID, int Quantity, const wchar_t * Date, const wchar_t * Type, byte plus);
    void TakeBtn();
public:
    int DatabaseID;
    CIFStatic* m_itemIcon;
    void Clear();
private:
    void ShowItemLink();
    int m_itemId;
    int m_quantity;
    byte m_plus;
};
