#pragma once

#include "IFWnd.h"
#include "IFMainFrame.h"
#include "IFMessageBox.h"

class CIFItemMall : public CIFMainFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CIFItemMall, 0xEEC2E8)
    void SetBuyItemCount(int nItemCount);

    int GetBuyItemCount() const;
    void UpdateMenuSize();
    bool OnCreateIMPL(long ln);
    undefined1 OnCloseWnd_IMPL();

    int sub_7C0490(int a2, int a3);
private:
    char pad_07BC[0x58];    //0x07BC
public:
    CIFMessageBox* m_pMessageBox; //0x0814
    bool m_bIsMessageBoxCreated; //0x0818
private:
    char pad_0819[0x03]; //0x0819
    int m_nBuyItemCount;    //0x081C

};
