#include <GInterface.h>
#include <GlobalDataManager.h>
#include <BSLib/multibyte.h>
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomSettingManager.h>
#include "IFMacroMenuAutoHuntPartySlot.h"
#include "IFMacroMenu.h"

#define GDR_TAKE_BTN 71
GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoHuntPartySlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoHuntPartySlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFMacroMenuAutoHuntPartySlot::CIFMacroMenuAutoHuntPartySlot(void) {
}

CIFMacroMenuAutoHuntPartySlot::~CIFMacroMenuAutoHuntPartySlot(void)
{
    //printf("> " __FUNCTION__ "\n");
}

bool CIFMacroMenuAutoHuntPartySlot::OnCreate(long ln)
{
    //printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacromenuautoskillpartyslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_CharnamePtr = this->m_IRM.GetResObj<CIFStatic>(4, 1);
    this->m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();
    wnd_pos x =  this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetPos();


    this->ShowGWnd(false);
    return true;
}

void CIFMacroMenuAutoHuntPartySlot::LoadItems(std::n_wstring CharName) {
    this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(CharName.c_str());
}

void CIFMacroMenuAutoHuntPartySlot::ClearDDJ()
{
    this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
int CIFMacroMenuAutoHuntPartySlot::OnMouseLeftUp(int a1, int x, int y) {
    if(!this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetNText().empty())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->ClearPartySlotDDJ();
        // printf("%d \n", ItemDBID);


        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->SelectedPartyMemberName = this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetNText();
        this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);

    }

    return true;
}
void CIFMacroMenuAutoHuntPartySlot::SetBarSize(int widht, int height){
    this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->SetGWndSize(widht, height);
    this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetGWndSize(widht, height);
}
