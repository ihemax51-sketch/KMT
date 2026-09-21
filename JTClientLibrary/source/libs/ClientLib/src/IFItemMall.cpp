#include "IFItemMall.h"
#include "Game.h"
#include "GInterface.h"
#include <CustomData/CustomSettingManager.h>

undefined1 CIFItemMall::OnCloseWnd_IMPL() {
    reinterpret_cast<int(__thiscall *)(CIFItemMall *)>(0x007BD530)(this);
    if (m_Settings->EnableOldItemMall && g_pCGInterface) {
        g_pCGInterface->OnItemMallSectionControl(false);
    }

    return 1;
}

bool CIFItemMall::OnCreateIMPL(long ln) {
    return reinterpret_cast<bool (__thiscall *)(CIFItemMall *, long)>(0x007c09e0)(this, ln);
}

void CIFItemMall::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFItemMall::SetBuyItemCount(int nItemCount){
    m_nBuyItemCount = nItemCount;
}
int CIFItemMall::GetBuyItemCount() const{
    return m_nBuyItemCount;
}
