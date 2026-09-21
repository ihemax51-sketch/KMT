
#include <Data/ItemData.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <GInterface.h>
#include <CustomData/CustomDataManager.h>
#include "IFAlchemyMacroAttrSlot.h"
#include "IFAlchemyMacro.h"


GFX_IMPLEMENT_DYNCREATE(CIFAlchemyMacroAttrSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFAlchemyMacroAttrSlot, CIFWnd)

GFX_END_MESSAGE_MAP()
CIFAlchemyMacroAttrSlot::CIFAlchemyMacroAttrSlot(void)
{
    Code = std::wstring();

}
CIFAlchemyMacroAttrSlot::~CIFAlchemyMacroAttrSlot(void)
{
}
bool CIFAlchemyMacroAttrSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifalchemymacroblueslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();
    return true;
}
void CIFAlchemyMacroAttrSlot::OnUpdate() {

}
void CIFAlchemyMacroAttrSlot::Clear()
{
    Code.clear();
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(L"");
}
void CIFAlchemyMacroAttrSlot::ClearDDJ()
{
    m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
int CIFAlchemyMacroAttrSlot::OnMouseLeftUp(int a1, int x, int y) {
    CIFAlchemyMacro* macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
    macro->ClearDDJListAttr();
    // printf("%d \n", ItemDBID);

    if(!Code.empty())
    {
        m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);
        if(macro->CurrentPage == 2)
        {
            macro->SelectedRemoveAttrCode = Code;
        }

    }
    return true;
}
void CIFAlchemyMacroAttrSlot::LoadItems2(int Target, const wchar_t *Codex, const wchar_t *Name){
    Code = Codex;

    wchar_t Priceb[255];
    swprintf_s(Priceb, L"%ls (%d%%)", Name, Target);

    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(Priceb);

}
