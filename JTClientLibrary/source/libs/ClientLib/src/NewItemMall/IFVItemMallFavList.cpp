#include <Game.h>
#include "IFVItemMallFavList.h"
#include <NetProcessIn.h>
#include <BSLib/multibyte.h>
#include <GInterface.h>
#include <sstream>

#include <BSLib/Debug.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include "IFVItemMall.h"
#include "IFVItemMallBuyItem.h"
#define GDR_STATIC1 6

#define GDR_PURCHASE 11
#define GDR_CANCEL_BTN 12


GFX_IMPLEMENT_DYNCREATE(CIFVItemMallFavList, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFVItemMallFavList, CIFWnd)
                    ONG_COMMAND(GDR_PURCHASE, &CIFVItemMallFavList::On_BtnClick_1)
                    ONG_COMMAND(GDR_CANCEL_BTN, &CIFVItemMallFavList::On_BtnClick_2)

GFX_END_MESSAGE_MAP()

CIFVItemMallFavList::CIFVItemMallFavList(void) {

    IconID = 0;

    Quan = 0;
    Pricex = 0;

    MallID = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVItemMallFavList::~CIFVItemMallFavList(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}


bool CIFVItemMallFavList::OnCreate(long ln)
{
    //	printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvitemmallfavlist.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));


    m_itemInfo = m_IRM.GetResObj<CIFSlotWithHelp>(4, 1);
    m_itemInfo->ShowGWnd(true);
    m_itemInfo->SetClickable(false);
    // m_itemInfo->TB_Func_14("item\\etc\\icon_edge_rare.ddj", 1,0);


    m_IRM.GetResObj(11, 1)->SetText(KmtGetText(L"UIIT_KMT_PURCHASE"));






    //ItemSlotPic->ShowGWnd(false);
    m_itemInfo->BringToFront();
    m_itemInfo->SetType(19);

    this->Quan = 1;
    this->Pricex = 31;


    return true;
}

void CIFVItemMallFavList::OnUpdate()
{
    //if(!g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(1353, 1)->IsVisible() || g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(1352, 1)->IsVisible())
    //{
    //    this->ShowGWnd(false);
    //}
}

void CIFVItemMallFavList::On_BtnClick_1()
{
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->MallID = 1;
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->IconID = IconID;

    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->m_itemInfo->SetSlotData(m_itemInfo->ItemInfo);

    std::wstring itemname = g_CTextStringManager->GetString2(m_itemInfo->ItemInfo->GetItemData()->NameStrID.c_str())->c_str();
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->IconName->SetText(itemname.c_str());


    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->Quan = 1;
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->Pricex = Pricex;

    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->ShowGWnd(true);
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->BringToFront();

}

void CIFVItemMallFavList::On_BtnClick_2()
{

    if(g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->FavItemList.find(this->IconID) != g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->FavItemList.end())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->FavItemList.erase(this->IconID);
        g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->ClearFav();
        g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->UpdateFavList();
    }
}

void CIFVItemMallFavList::AddItem(int IconIDx, int ItemID, int ItemCount, int Price){
    IconID = IconIDx;

    FillTheSlot(ItemID,ItemCount);

    wchar_t Priceb[255];

    std::wstring test = Insert(Price);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());

    m_IRM.GetResObj(7, 1)->SetText(Priceb);

    Pricex = Price;

    if(m_itemInfo->ItemInfo == NULL)
    {
        this->ShowGWnd(false);
    }
}

void CIFVItemMallFavList::FillTheSlot(int itemID, int itemCount) {
    const SItemData* data = &g_CGlobalDataManager->GetItemData(itemID);
    m_itemInfo->ShowGWnd(true);
    if (data != NULL) {
        m_itemInfo->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);

        std::wstring itemname = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();
        if(itemname.empty())
        {
            m_IRM.GetResObj<CIFTextBox>(5, 1)->SetText(KmtGetText(L"UIIT_KMT_UNKNOWN"));
        }
        m_IRM.GetResObj<CIFTextBox>(5, 1)->SetText(itemname.c_str());

        CMsgStreamBuffer buf(0xB034);
        buf << INT32(0) << INT32(itemID);
        u_short typeID2 = data->m_typeId.getTypeID2();
        u_short typeID3 = data->m_typeId.getTypeID3();
        u_short typeID4 = data->m_typeId.getTypeID4();
        switch (typeID2) {
            case 1:
                buf << UINT8(0) << UINT64(0) << UINT32(1) << UINT8(0) << UINT8(1) << UINT8(0) << UINT8(2) << UINT8(0);
                break;
            case 2:
                switch (typeID3) {
                    case 1:
                        buf << UINT8(0x01);
                        break;
                    case 2:
                        buf << UINT32(0x00);
                        break;
                    default:
                        if (typeID4 == 3)//Magic cube
                            buf << UINT32(0x01);
                        break;
                }
                break;
            case 3:
                buf << UINT16(0x01);
                if (typeID3 == 11)
                    if (typeID4 == 1 || typeID4 == 2)
                        buf << UINT8(0x00);
                    else if (typeID3 == 14 && typeID4 == 2)
                        buf << UINT8(0x00);
                break;
        }

        CSOItem* tempItemInfo = new CSOItem();
        tempItemInfo->ReadFromPacket(&buf, 1);
        tempItemInfo->SetEnabled(true);
        if (tempItemInfo) {

            //  m_itemInfo->ItemInfo = tempItemInfo;
            /* if(tempItemInfo->GetItemData()->IsNasrun())
             {
                 m_itemInfo->ItemInfo = tempItemInfo;
             }
             else
             {
                 m_itemInfo->SetSlotData(tempItemInfo);
             }*/
            m_itemInfo->SetSlotData(tempItemInfo);
            if(itemCount > 1)
            {
                m_itemInfo->ItemInfo->m_quantity = itemCount;
            }
            m_itemInfo->ItemInfo->m_quantity = itemCount;
            m_itemInfo->ItemInfo->m_OptLevel = 0;
            m_itemInfo->SetType(19);
            m_itemInfo->ShowGWnd(true);
        }
    }
}