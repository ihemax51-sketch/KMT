//
// Created by YUMBUL on 15.06.2024.
//

#include "InterfaceNetSender.h"



void CInterfaceNetSender::BuyItemMallItem(int nRefPackageItemId, int nShopId, BYTE btTab, BYTE btIndex, USHORT usCount,
                                          int a6, int a7, int a8, CStringData strCodeName) {

    reinterpret_cast<void(__thiscall *)(CInterfaceNetSender *, int, int, BYTE, BYTE, USHORT, int, int, int, CStringData)>(0x00826460)(this,
                                                                                                                                      nRefPackageItemId, nShopId, btTab, btIndex, usCount, a6, a7, a8, strCodeName);
}
