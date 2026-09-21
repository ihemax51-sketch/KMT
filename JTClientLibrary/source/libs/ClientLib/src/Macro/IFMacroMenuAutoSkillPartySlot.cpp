#include <GInterface.h>
#include <GlobalDataManager.h>
#include <BSLib/multibyte.h>
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomSettingManager.h>
#include "IFMacroMenuAutoSkillPartySlot.h"
#include "IFMacroMenu.h"

#define GDR_TAKE_BTN 71
GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoSkillPartySlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoSkillPartySlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFMacroMenuAutoSkillPartySlot::CIFMacroMenuAutoSkillPartySlot(void) {
}

CIFMacroMenuAutoSkillPartySlot::~CIFMacroMenuAutoSkillPartySlot(void)
{
    //printf("> " __FUNCTION__ "\n");
}

bool CIFMacroMenuAutoSkillPartySlot::OnCreate(long ln)
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

void CIFMacroMenuAutoSkillPartySlot::LoadItems(std::n_wstring CharName) {
    this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(CharName.c_str());
}

void CIFMacroMenuAutoSkillPartySlot::ClearDDJ()
{
    this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
int CIFMacroMenuAutoSkillPartySlot::OnMouseLeftUp(int a1, int x, int y) {
    if(!this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetNText().empty())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->ClearPartySlotDDJ();
        // printf("%d \n", ItemDBID);


        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SelectedPartyMemberName = this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetNText();
        this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);

        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SelectedPartyBuffs();
    }


    return true;
}