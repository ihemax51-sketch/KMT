
#include <Data/ItemData.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <GInterface.h>
#include <CustomData/CustomDataManager.h>
#include "IFAlchemyMacroBlueSlot.h"
#include "IFAlchemyMacro.h"


GFX_IMPLEMENT_DYNCREATE(CIFAlchemyMacroBlueSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFAlchemyMacroBlueSlot, CIFWnd)

GFX_END_MESSAGE_MAP()
CIFAlchemyMacroBlueSlot::CIFAlchemyMacroBlueSlot(void)
{
    Param1 = -1;
    Code = std::wstring();

}
CIFAlchemyMacroBlueSlot::~CIFAlchemyMacroBlueSlot(void)
{
}
bool CIFAlchemyMacroBlueSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifalchemymacroblueslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();
    return true;
}
void CIFAlchemyMacroBlueSlot::OnUpdate() {

}
void CIFAlchemyMacroBlueSlot::Clear()
{
    Param1 = -1;
    Code.clear();
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(L"");
}
void CIFAlchemyMacroBlueSlot::LoadItems(int Param1x, const wchar_t * Name)
{
    Param1 = Param1x;
    //printf("%ls\n", Name);
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(Name);
}
void CIFAlchemyMacroBlueSlot::LoadItems2(const wchar_t * Codex, const wchar_t * Name)
{
    Code = Codex;
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(Name);
}
void CIFAlchemyMacroBlueSlot::ClearDDJ()
{
    m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
int CIFAlchemyMacroBlueSlot::OnMouseLeftUp(int a1, int x, int y) {
    CIFAlchemyMacro * macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
    macro->ClearDDJList();
    // printf("%d \n", ItemDBID);

    if(Param1 != -1 || !Code.empty())
    {
        m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);

        if(macro->CurrentPage == 2)
        {
            macro->SelectedAttrCode = Code;
        }
    }


    return true;
}
