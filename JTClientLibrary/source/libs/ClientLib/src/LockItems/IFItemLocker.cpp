#include <Game.h>
#include "IFItemLocker.h"
#include <NetProcessIn.h>
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <CustomData/CustomCICPlayer.h>

#define GDR_CERT_BUTTON 10

#define GDR_LOCK_BUTTON 14
#define GDR_CANCEL_BUTTON 15

GFX_IMPLEMENT_DYNCREATE(CIFItemLocker, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFItemLocker, CIFMainFrame)
                    ONG_COMMAND(GDR_LOCK_BUTTON, &CIFItemLocker::On_BtnClick_1)
                    ONG_COMMAND(GDR_CANCEL_BUTTON, &CIFItemLocker::On_BtnClick_2)
                    ONG_COMMAND(GDR_CERT_BUTTON, &CIFItemLocker::OnBtnCert)
GFX_END_MESSAGE_MAP()

CIFItemLocker::CIFItemLocker(void) {
    ok = 0;
    cancel = 0;
    write = 0;
    ItemSlotID = -1;
    ItemTypeID = -1;
    //UniqueID = 0;
    //TeleportType = 0;
    //targetTeleport = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFItemLocker::~CIFItemLocker(void) {
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFItemLocker::OnCreate(long ln) {
    //	printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    // TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifitemlocker.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->m_pTitleText->SetText(KmtGetText(L"UIIT_KMT_LOCK_ITEM"));

    wnd_rect sz;

    m_IRM.GetResObj(9, 1)->SetText(KmtGetText(L"UIIT_KMT_E_MAIL"));
    m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_AUTHENTICATE"));
    m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_REMAINING_TIME"));
    m_IRM.GetResObj(14, 1)->SetText(KmtGetText(L"UIIT_KMT_LOCK_ITEM"));
    m_IRM.GetResObj(15, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));
    m_IRM.GetResObj(16, 1)->SetText(KmtGetText(L"UIIT_KMT_WHEN_AN_ITEM_IS_LOCKED_IT_CANNOT_BE_TRADED_DROPPED_STORED_SOLD_AT_A_SHOP_OR_RE_73EB38A7"));
    sz.pos.x= 161;
    sz.pos.y = 46;
    sz.size.width = 32;
    sz.size.height = 32;
    ItemSlot = m_IRM.GetResObj<CIFItemLockSlot>(500, 1);
    ItemSlot->m_pMySlot->m_pSlot->SetGWndSize(43, 42);
    ItemSlot->m_pMySlot->m_pSlot->SetType(0xC);
    ItemSlot->m_pMySlot->m_pSlot->SetSlot(600);
    ItemSlot->m_pMySlot->m_pSlot->BringToFront();

    this->m_IRM.GetResObj<CIFVerticalScroll>(17, 1)->m_arrowUpButton->SetGWndSize(18, 16);
    this->m_IRM.GetResObj<CIFVerticalScroll>(17, 1)->m_arrowDownButton->SetGWndSize(18, 16);
    this->m_IRM.GetResObj<CIFVerticalScroll>(17, 1)->m_scrollButton->SetGWndSize(18, 16);

    Pass1 = m_IRM.GetResObj<CIFEdit>(19, 1);
    Pass1->AddValue_404(4);
    Pass1->SetValue_404(1);



    /*


    m_IRM.GetResObj<CIFTextBox>(7, 1)->SetText(L"Locking Items\nWhen an item is locked, it cannot be traded, dropped, stored, sold at a shop, or registered at a street shop.  It can only be unlocked with a key.");


    sz.pos.x= 12;
    sz.pos.y = 224;
    sz.size.width = 90;
    sz.size.height = 24;

    m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_PASSWORD"));


    Pass1 = m_IRM.GetResObj<CIFEdit>(9, 1);
    Pass1->AddValue_404(4);
    Pass1->SetValue_404(1);
    Pass1->SetMaxLength(6);

    m_IRM.GetResObj(11, 1)->SetText(KmtGetText(L"UIIT_KMT_R_PASSWORD"));


    Pass2 = m_IRM.GetResObj<CIFEdit>(10, 1);
    Pass2->AddValue_404(4);
    Pass2->SetValue_404(1);
    Pass2->SetMaxLength(6);



    sz.pos.x = 76;
    sz.pos.y = 334;
    sz.size.width = 96;
    sz.size.height = 28;
    ok = m_IRM.GetResObj<CIFButton>(GDR_OK_BTN, 1);
    ok->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));






    sz.pos.x = 179;
    sz.pos.y = 334;
    sz.size.width = 98;
    sz.size.height = 28;

    cancel = m_IRM.GetResObj<CIFButton>(GDR_CANCEL_BTN, 1);

    cancel->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));



*/

    this->ShowGWnd(false);
    UpdateMenuSize();


    return true;
}

void CIFItemLocker::OnUpdate() {
}

void CIFItemLocker::SetItemInfo(int ItemSlotIDs, unsigned short ItemTypeIDs) {
    ItemSlotID = ItemSlotIDs;
    ItemTypeID = ItemTypeIDs;
}
void CIFItemLocker::UpdateMenuSize() {
    m_IRM.GetResObj(18, 1)->SetText(m_Player->MailAddress.c_str());
    USHORT PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();

}
#define Gerisayim 1
void CIFItemLocker::Clear()
{
    Pass1->SetText(L"");
    m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_REMAINING_TIME"));
    this->KillTimer(Gerisayim);
    ItemSlotID = -1;
    ItemTypeID = -1;
    ItemSlot->ClearSlot();
    this->m_IRM.GetResObj<CIFButton>(GDR_CERT_BUTTON, 1)->SetEnabledState(true);
    this->m_IRM.GetResObj<CIFButton>(GDR_LOCK_BUTTON, 1)->SetEnabledState(true);
}
void CIFItemLocker::EnableStateLockButton()
{
    this->m_IRM.GetResObj<CIFButton>(GDR_LOCK_BUTTON, 1)->SetEnabledState(true);
}
void CIFItemLocker::On_BtnClick_1() {
    if(Pass1->GetCurrentText().size() > 5)
    {
            CIFMainPopup *popup = g_pCGInterface->GetMainPopup();
            CIFInventory *inventory = popup->GetInventory();
            if (ItemSlotID >= 0 && ItemSlotID <= 95) {
                if(this->ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
                {
                    CSOItem *item = inventory->GetItemBySlot(ItemSlotID);

                    if (item->m_blValid == 0) {
                        this->OnCloseWnd();
                        return;
                    }

                    if (item->GetItemData()->m_typeId.m_type_id_value == 0xCEED) {

                        CMsgStreamBuffer buf(0x210A);


                        buf << item->GetItemData()->RefObjectId << ItemSlotID+13 << ItemTypeID << this->ItemSlot->m_pMySlot->m_pSlot->GetInventorySlotType()+13

                            << Pass1->GetCurrentText();

                        SendMsg(buf);
                        this->m_IRM.GetResObj<CIFButton>(GDR_LOCK_BUTTON, 1)->SetEnabledState(false);

                    }
                }
            }
    }
    else
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THE_CODE_MUST_CONSIST_OF_MINIMUM_6_NUMBERS"));
        g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_THE_CODE_MUST_CONSIST_MINIMUM_6_NUMBERS"));

    }
    //this->OnCloseWnd();
}

void CIFItemLocker::On_BtnClick_2() {
    Clear();
    this->OnCloseWnd();

}
undefined1 CIFItemLocker::OnCloseWnd(){
    Clear();

    return CIFWnd::OnCloseWnd();
}

void CIFItemLocker::OnBtnCert()
{
    if(this->ItemSlot->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        CMsgStreamBuffer buf(0x169A);
        buf << byte(22);
        buf << byte(0); // LOCK TYPE
        SendMsg(buf);
        this->m_IRM.GetResObj<CIFButton>(GDR_CERT_BUTTON, 1)->SetEnabledState(false);
    }
}

void CIFItemLocker::StartRemainingTime()
{
    wchar_t buffer[255]; std::swprintf(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_REMAINING_TIME_03_00"));
    endTime = std::time(NULL) + 3 * 60;
    this->StartTimer(Gerisayim, 1000);
}
void CIFItemLocker::OnTimer(int timerId) {
    if (timerId == Gerisayim)
    {
        std::time_t currentTime = std::time(NULL);
        int remainingTime = static_cast<int>(endTime - currentTime);
        if (remainingTime <= 0)
        {
            m_IRM.GetResObj(13, 1)->SetText(KmtGetText(L"UIIT_KMT_TIME_S_UP"));
            this->KillTimer(Gerisayim);
        }
        else
        {
            wchar_t buffer[255]; std::swprintf(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_REMAINING_TIME_02D_02D"), remainingTime / 60, remainingTime % 60);
            m_IRM.GetResObj(13, 1)->SetText(buffer);
        }
    }
}