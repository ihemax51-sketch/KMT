#include <GInterface.h>
#include "IFVAvatarMallBuyItemListSlot.h"
#include "IFVAvatarMallBuyItemList.h"
#include <GlobalDataManager.h>
#include <BSLib/multibyte.h>
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <BSLib/Debug.h>
#include <TextStringManager.h>
#include <sstream>

#define GDR_SLOT 5
#define GDR_TEXTBOX 6
#define GDR_STATIC1 7
#define GDR_STATIC2 8
#define GDR_TAKE_BTN 9
GFX_IMPLEMENT_DYNCREATE(CIFVAvatarMallBuyItemListSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFVAvatarMallBuyItemListSlot, CIFWnd)
                    ONG_COMMAND(GDR_TAKE_BTN, &TakeBtn)
GFX_END_MESSAGE_MAP()

CIFVAvatarMallBuyItemListSlot::CIFVAvatarMallBuyItemListSlot(void) {
    //printf("> " __FUNCTION__ "\n");
    m_itemInfo = 0;
    ItemDBID = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFVAvatarMallBuyItemListSlot::~CIFVAvatarMallBuyItemListSlot(void)
{
    //printf("> " __FUNCTION__ "\n");
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFVAvatarMallBuyItemListSlot::OnCreate(long ln)
{
    //printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvavatarmallbuyslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    wnd_rect sz;

    sz.pos.x = 22;
    sz.pos.y = 5;
    sz.size.width = 32;
    sz.size.height = 32;
    m_itemInfo = m_IRM.GetResObj<CIFSlotWithHelp>(4, 1);
    m_itemInfo->ShowGWnd(false);
    m_itemInfo->SetClickable(false);




    m_itemInfo->BringToFront();

    this->ShowGWnd(false);
    return true;
}

void CIFVAvatarMallBuyItemListSlot::SetName(int ID, int ItemId, int Price) {

    FillTheSlot(ItemId, 1, 0);
    ItemDBID = ID;
    this->m_IRM.GetResObj<CIFButton>(9, 1)->ShowGWnd(true);
    m_IRM.GetResObj<CIFStatic>(6, 1)->ShowGWnd(true);
    wchar_t Priceb[255];
    std::wstring test = Insert(Price);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());


    m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Priceb);
    m_IRM.GetResObj<CIFStatic>(7, 1)->ShowGWnd(true);
}

void CIFVAvatarMallBuyItemListSlot::ClearSlot()
{
    m_itemInfo->ClearSlot();
    m_itemInfo->SetSlotData(NULL);
    m_itemInfo->TB_Func_13("", 0, 0);
    m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(L"");

    m_IRM.GetResObj<CIFStatic>(6, 1)->ShowGWnd(false);
    m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(L"");
    m_IRM.GetResObj<CIFStatic>(7, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFButton>(9, 1)->ShowGWnd(false);

}
void CIFVAvatarMallBuyItemListSlot::TakeBtn()
{
    if(g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.find(m_itemInfo->ItemInfo->GetItemData()->m_typeId.m_type_id_value)
       != g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.end())
    {
        // g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList[newh->m_typeId.m_type_id_value];
        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->ItemList.erase(m_itemInfo->ItemInfo->GetItemData()->m_typeId.m_type_id_value);
        g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1)->UpdateList();
    }

}

void CIFVAvatarMallBuyItemListSlot::FillTheSlot(int itemID, int itemCount, int OptLevel) {
    const SItemData *data = &g_CGlobalDataManager->GetItemData(itemID);

    if (data != NULL) {
        m_itemInfo->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);
        std::wstring itemname = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();
        m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(itemname.c_str());

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

        CSOItem *tempItemInfo = new CSOItem();
        tempItemInfo->ReadFromPacket(&buf, 1);
        tempItemInfo->SetEnabled(true);
        if (tempItemInfo) {

            /*m_itemInfo->ItemInfo = tempItemInfo;*/
            if(tempItemInfo->GetItemData()->IsNasrun())
            {
                m_itemInfo->ItemInfo = tempItemInfo;
            }
            else
            {
                m_itemInfo->SetSlotData(tempItemInfo);
            }

            m_itemInfo->ItemInfo->m_quantity = itemCount;
            m_itemInfo->ItemInfo->m_OptLevel = OptLevel;
            m_itemInfo->SetType(19);
            m_itemInfo->ShowGWnd(true);
        }
    }
}


