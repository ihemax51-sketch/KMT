#pragma once


#include <Data/SOItemPackage.h>
#include <Data/ItemMallData.h>
#include "IFFrame.h"

class CIFItemMallConfirmBuy : public CIFFrame {
public:
    void OnBuy_BtnClick();
    CSOItemPackage *GetPackageItem() const;
    int GetRealItemIndex(int nShopId, int nTabIndex, CSOItemPackage* pPackageItem);

private:
    char pad_07B0[0x8];        //0x07B0
    unsigned __int16 m_usCount;//0x07B8
    char pad_07BA[0x2E];       //0x07BA
    int m_nField7E8;
    int m_nField7EC;
    char pad_07F0[0x3C];             //0x07F0
    CSOItemPackage *m_pSOPackageItem;//0x082C
};
