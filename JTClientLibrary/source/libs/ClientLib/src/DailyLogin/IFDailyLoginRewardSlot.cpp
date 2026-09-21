//
// Created by YUMBUL on 10.07.2024.
//

#include <IFSlotWithHelp.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <IFTextBox.h>
#include <GlobalHelpersThatHaveNoHomeYet.h>
#include <GInterface.h>
#include <BSLib/multibyte.h>
#include "IFDailyLoginRewardSlot.h"

#define GDR_RECEIVE 13
GFX_IMPLEMENT_DYNCREATE(CIFDailyLoginRewardSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFDailyLoginRewardSlot, CIFWnd)
                    ONG_COMMAND(GDR_RECEIVE, &CIFDailyLoginRewardSlot::Receive)
GFX_END_MESSAGE_MAP()
CIFDailyLoginRewardSlot::CIFDailyLoginRewardSlot(void)
{
    RefRewardID = 999999999;
    DItemID = 0;
    ItemCodeName = std::n_wstring();
}
CIFDailyLoginRewardSlot::~CIFDailyLoginRewardSlot(void)
{

}


bool CIFDailyLoginRewardSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdailyloginrewardslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_IRM.GetResObj<CIFButton>(GDR_RECEIVE, 1)->SetEnabledState(false);
    m_IRM.GetResObj<CIFSlotWithHelp>(12, 1)->SetClickable(false);

    m_itemInfo = m_IRM.GetResObj<CIFSlotWithHelp>(12, 1);

    this->m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_RECEIVE"));



    return true;
}
void CIFDailyLoginRewardSlot::OnUpdate() {

}
void CIFDailyLoginRewardSlot::ClearSlot()
{

    m_IRM.GetResObj(11, 1)->SetText(L"");
    m_itemInfo->ClearSlot();
    m_itemInfo->TB_Func_13("", 0, 0);
}
void CIFDailyLoginRewardSlot::OpenReceiveButton(bool value)
{
    m_IRM.GetResObj<CIFButton>(GDR_RECEIVE, 1)->SetEnabledState(value);
}
void CIFDailyLoginRewardSlot::FillTheSlot(int itemID, int itemCount, int OptLevel, int day, int RefRewardIDx) {
    const SItemData* data = &g_CGlobalDataManager->GetItemData(itemID);

    if (data != NULL) {
        m_itemInfo->TB_Func_13(data->AssocFileIcon.c_str(), 0, 0);
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
            if (tempItemInfo->GetItemData()->IsNasrun())
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


        ItemCodeName = data->CodeName;


        wchar_t pageText[16];
        swprintf(pageText, KmtGetText(L"UIIT_KMT_VALUE_DAYS"), day);

        m_IRM.GetResObj(11, 1)->SetText(pageText);

        RefRewardID = RefRewardIDx;
        DItemID = itemID;
    }
}
void CIFDailyLoginRewardSlot::Receive(){
    m_IRM.GetResObj<CIFButton>(GDR_RECEIVE, 1)->SetEnabledState(false);
    CMsgStreamBuffer buf(0x169A);
    std::n_string Code = TO_NSTRING(ItemCodeName);
    buf << byte(10);
    buf << RefRewardID;
    SendMsg(buf);
}

