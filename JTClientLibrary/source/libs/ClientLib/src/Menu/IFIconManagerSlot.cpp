#include <GInterface.h>
#include <GlobalDataManager.h>
#include <BSLib/multibyte.h>
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include <CustomData/CustomSettingManager.h>
#include "IFIconManagerSlot.h"
#include "IFIconManager.h"
#define GDR_TAKE_BTN 71
GFX_IMPLEMENT_DYNCREATE(CIFIconManagerSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFIconManagerSlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFIconManagerSlot::CIFIconManagerSlot(void) {
    //printf("> " __FUNCTION__ "\n");
    ItemDBID = -1;
    SideValue = -1;
}

CIFIconManagerSlot::~CIFIconManagerSlot(void)
{
    //printf("> " __FUNCTION__ "\n");
}

bool CIFIconManagerSlot::OnCreate(long ln)
{
    //printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ificonmanagerslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    this->m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();
    wnd_pos x =  this->m_IRM.GetResObj<CIFStatic>(4, 1)->GetPos();
    if(!m_Settings->IconManagerRight)
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->MoveGWnd(x.x + 8, x.y);
    }
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->BringToFront();
    this->m_IRM.GetResObj<CIFStatic>(5, 1)->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
    this->ShowGWnd(false);
    return true;
}

void CIFIconManagerSlot::LoadItems(byte DBID, std::n_wstring ItemName, int side) {

    ItemDBID = DBID;

    if(ItemName == L"dummy")
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("", 1, 1);
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_NO_ICON"));
    }
    else if(ItemName == L"<No Icon>")
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_NO_ICON"));
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13(TO_NSTRING(ItemName), 1, 1);
    }
    if(m_Settings->IconManagerRight)
    {
        if(side == 0)
        {
            this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"L");
        }
        else if(side == 1)
        {
            this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"R");
        }
    }
    SideValue = side;

}
void CIFIconManagerSlot::Clear()
{
    ItemDBID = -1;
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(L"");
    m_IRM.GetResObj<CIFStatic>(4, 1)->TB_Func_13("", 0, 0);

    m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"");
    m_IRM.GetResObj<CIFStatic>(5, 1)->TB_Func_13("", 0, 0);

    SideValue = -1;

}
void CIFIconManagerSlot::ClearDDJ()
{
    this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
int CIFIconManagerSlot::OnMouseLeftUp(int a1, int x, int y) {

    if(ItemDBID != -1)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ClearDDJ();
        // printf("%d \n", ItemDBID);

        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->m_SelectedItemId = ItemDBID;
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->m_SelectedSide = SideValue;

        this->m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);


    }

    return true;
}