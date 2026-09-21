#pragma once


#include <Data/ItemMallData.h>
#include "IFWnd.h"
#include "IFItemMall.h"
#include "IFMessageBox.h"

class CIFItemMallZzim : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFItemMallZzim, 0xEEC4A8)
public:
    void OnBuyAll_BtnClick();
    void RefreshReservedItems();
    void OnBuyAllCallBack();

private:
    char pad_036C[0xBC];
    CIFMessageBox* m_pMessageBox; //0x0428
};
