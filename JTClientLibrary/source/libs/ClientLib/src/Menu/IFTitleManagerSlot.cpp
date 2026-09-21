#include <GInterface.h>
#include <GlobalDataManager.h>
#include <BSLib/multibyte.h>
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <TextStringManager.h>
#include "IFTitleManagerSlot.h"
#include "IFTitleManager.h"
#define GDR_TAKE_BTN 71
GFX_IMPLEMENT_DYNCREATE(CIFTitleManagerSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFTitleManagerSlot, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFTitleManagerSlot::CIFTitleManagerSlot(void) {
    //printf("> " __FUNCTION__ "\n");
    ItemDBID = -1;
}

CIFTitleManagerSlot::~CIFTitleManagerSlot(void)
{
    //printf("> " __FUNCTION__ "\n");
}

bool CIFTitleManagerSlot::OnCreate(long ln)
{
    //printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFWnd::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iftitlemanagerslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_IRM.GetResObj<CIFStatic>(4, 1)->ShowGWnd(true);
    m_IRM.GetResObj<CIFStatic>(4, 1)->BringToFront();
    this->ShowGWnd(false);
    return true;
}

void CIFTitleManagerSlot::LoadItems(byte DBID, const wchar_t * ItemName) {

    ItemDBID = DBID;

    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(ItemName);
    m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(0xFFFFFF);
}

void CIFTitleManagerSlot::LoadItemsColors(byte DBID, std::wstring ItemName, unsigned __int32 Color) {

    ItemDBID = DBID;
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(ItemName.c_str());
    m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(Color);
    //printf("%ls \n", ItemName.c_str());
}

void CIFTitleManagerSlot::ClearDDJ()
{
    m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01_", 0, 0);
}
void CIFTitleManagerSlot::Clear()
{
    ItemDBID = -1;
    m_IRM.GetResObj<CIFStatic>(4, 1)->SetText(L"");
    m_IRM.GetResObj<CIFStatic>(4, 1)->m_FontTexture.SetColor(0xFFFFFF);
}
int CIFTitleManagerSlot::OnMouseLeftUp(int a1, int x, int y) {
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ClearDDJ();
    // printf("%d \n", ItemDBID);
    if(ItemDBID != -1)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->m_SelectedItemId = ItemDBID;

        m_IRM.GetResObj<CIFBarWnd>(3, 1)->TB_Func_13("interface\\ifcommon\\com_bar01select_", 1, 1);

    }


    return true;
}