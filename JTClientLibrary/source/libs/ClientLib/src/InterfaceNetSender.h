#pragma once

#include "IFItemMallShopSlot.h"

#define g_pCInterfaceNetSender (*(CInterfaceNetSender**)0x00eecbf4)

class CInterfaceNetSender {
public:


    void BuyItemMallItem(int nRefPackageItemId, int nShopId, unsigned char btTab, unsigned char btIndex, unsigned short usCount, int a6, int a7, int a8, CStringData strCodeName);
};

