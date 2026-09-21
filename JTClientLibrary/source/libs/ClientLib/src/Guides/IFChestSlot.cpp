
#include <Data/ItemData.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <GInterface.h>
#include <CustomData/CustomDataManager.h>
#include <GFXMainFrame/Controler.h>
#include "SOItem.h"
#include "IFChest.h"

#define CHEST_ITEM_LINK_SLOT 13437

GFX_IMPLEMENT_DYNCREATE(CIFChestSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFChestSlot, CIFWnd)
                    ONG_COMMAND(10, &TakeBtn)
GFX_END_MESSAGE_MAP()

static CSOItem* CreateChestPreviewItem(int itemId, int quantity, byte plus)
{
    const CItemData* itemData = g_CGlobalDataManager->GetItem(itemId);
    if (itemData == NULL) {
        return NULL;
    }
    const SItemData* data = &itemData->GetData();

    CMsgStreamBuffer buf(0xB034);
    buf << INT32(0) << INT32(itemId);

    const u_short typeID2 = data->m_typeId.getTypeID2();
    const u_short typeID3 = data->m_typeId.getTypeID3();
    const u_short typeID4 = data->m_typeId.getTypeID4();
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
                    if (typeID4 == 3)
                        buf << UINT32(0x01);
                    break;
            }
            break;
        case 3:
            buf << UINT16(0x01);
            if (typeID3 == 11) {
                if (typeID4 == 1 || typeID4 == 2)
                    buf << UINT8(0x00);
            } else if (typeID3 == 14 && typeID4 == 2) {
                buf << UINT8(0x00);
            }
            break;
    }

    CSOItem* itemInfo = new CSOItem();
    itemInfo->ReadFromPacket(&buf, 1);
    itemInfo->SetEnabled(true);
    itemInfo->m_quantity = quantity;
    itemInfo->m_OptLevel = plus;
    return itemInfo;
}

CIFChestSlot::CIFChestSlot(void)
{
    DatabaseID = 0;
    m_itemIcon = NULL;
    m_itemId = 0;
    m_quantity = 0;
    m_plus = 0;
}
CIFChestSlot::~CIFChestSlot(void)
{
}
bool CIFChestSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifchestslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->m_IRM.GetResObj<CIFButton>(10, 1)->SetText(KmtGetText(L"UIIT_KMT_TAKE"));
    this->m_IRM.GetResObj<CIFButton>(10, 1)->ShowGWnd(false);

    RECT itemIconRect = {36, 2, 20, 20};
    m_itemIcon = (CIFStatic*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), itemIconRect, 12, 0);
    if (m_itemIcon != NULL) {
        m_itemIcon->ShowGWnd(false);
    }
    return true;
}
void CIFChestSlot::OnUpdate() {

}

int CIFChestSlot::OnMouseLeftUp(int a1, int x, int y)
{
    if (DatabaseID != 0 && m_itemId > 0) {
        const wnd_rect bounds = GetBounds();
        const int localX = x - bounds.left();

        if (localX < 520) {
            ShowItemLink();
            return 0;
        }
    }

    return CIFWnd::OnMouseLeftUp(a1, x, y);
}

void CIFChestSlot::SetName(int FakeNum, int DbID, int ItemID, int Quantity, const wchar_t *Date, const wchar_t *Type, byte plus){
    DatabaseID = DbID;
    m_itemId = ItemID;
    m_quantity = Quantity;
    m_plus = plus;

    wchar_t buffer3[253];
    swprintf_s(buffer3, L"%d.", FakeNum);

    this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(buffer3);


    std::n_wstring Name;
    const CItemData* itemData = g_CGlobalDataManager->GetItem(ItemID);

    if (itemData != NULL)
    {
        const SItemData* data = &itemData->GetData();
        const std::n_wstring* translatedName = g_CTextStringManager->GetString2(data->NameStrID.c_str());
        Name = translatedName ? *translatedName : KmtGetText(L"UIIT_KMT_UNKNOWN");
        if (m_itemIcon != NULL) {
            m_itemIcon->TB_Func_13(data->AssocFileIcon.c_str(), 1, 1);
            m_itemIcon->ShowGWnd(true);
            m_itemIcon->BringToFront();
        }
    }
    else
    {
        Name = KmtGetText(L"UIIT_KMT_UNKNOWN");
        if (m_itemIcon != NULL) {
            m_itemIcon->ShowGWnd(false);
        }
    }
    std::n_wstring displayName = Name;
    if (displayName.length() > 24)
        displayName = displayName.substr(0, 21) + L"...";

    wchar_t buffer1[255];
    if (plus > 0)
        swprintf_s(
            buffer1,
            KmtGetText(L"UIIT_KMT_TEXT_PLUS_VALUE"),
            displayName.c_str(),
            plus);
    else
        swprintf_s(buffer1, L"%ls", displayName.c_str());
    CIFStatic* nameControl = this->m_IRM.GetResObj<CIFStatic>(5, 1);
    if (nameControl) {
        nameControl->SetText(buffer1);
        nameControl->SetTooltip(Name);
        nameControl->SetStyleThingy(TOOLTIP);
    }

    wchar_t buffer2[255];
    swprintf_s(buffer2, L"%d", Quantity);
    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(buffer2);


    this->m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Date);

    std::n_wstring source = Type ? Type : L"";
    std::n_wstring displaySource = source;
    if (displaySource.length() > 15)
        displaySource = displaySource.substr(0, 12) + L"...";
    CIFStatic* sourceControl = this->m_IRM.GetResObj<CIFStatic>(8, 1);
    if (sourceControl) {
        sourceControl->SetText(displaySource.c_str());
        sourceControl->SetTooltip(source);
        sourceControl->SetStyleThingy(TOOLTIP);
    }

    CIFButton* takeButton = this->m_IRM.GetResObj<CIFButton>(10, 1);
    if (takeButton) {
        takeButton->SetEnabledState(true);
        takeButton->ShowGWnd(true);
    }
}
void CIFChestSlot::Clear()
{
    DatabaseID = 0;
    m_itemId = 0;
    m_quantity = 0;
    m_plus = 0;
    this->m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(L"");



    this->m_IRM.GetResObj<CIFStatic>(5, 1)->SetText(L"");

    this->m_IRM.GetResObj<CIFStatic>(6, 1)->SetText(L"");


    this->m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(L"");
    this->m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(L"");
    if (m_itemIcon != NULL) {
        m_itemIcon->ShowGWnd(false);
    }
    this->m_IRM.GetResObj<CIFButton>(10, 1)->ShowGWnd(false);
}

void CIFChestSlot::ShowItemLink()
{
    if (!g_pCGInterface || !g_pCGInterface->m_helperWindow || m_itemId <= 0) {
        return;
    }

    CSOItem* itemInfo = CreateChestPreviewItem(m_itemId, m_quantity, m_plus);
    if (itemInfo == NULL) {
        return;
    }

    CIFSlotWithHelp* itemLinkSlot = g_pCGInterface->GetGuiFromList<CIFSlotWithHelp>(CHEST_ITEM_LINK_SLOT);
    if (itemLinkSlot == NULL) {
        RECT rect = {0, 0, 0, 0};
        itemLinkSlot = (CIFSlotWithHelp*)g_pCGInterface->CreateInstance(
            g_pCGInterface, GFX_RUNTIME_CLASS(CIFSlotWithHelp), rect, CHEST_ITEM_LINK_SLOT, 0);
        if (itemLinkSlot != NULL) {
            itemLinkSlot->ShowGWnd(false);
        }
    }

    if (itemLinkSlot == NULL) {
        delete itemInfo;
        return;
    }

    g_pCGInterface->m_helperWindow->Reset();
    g_pCGInterface->m_helperWindow->ShowGWnd(true);
    itemLinkSlot->ShowGWnd(false);
    itemLinkSlot->ItemInfo = itemInfo;
    itemLinkSlot->SetType(70);
    itemLinkSlot->MoveGWnd(g_Controler->m_CursorPos.x + 25, g_Controler->m_CursorPos.y);
    itemLinkSlot->sub_686DB0();
}

void CIFChestSlot::TakeBtn()
{
    if (!g_pCGInterface)
        return;

    CIFChest* chest = g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1);
    if (!chest || chest->TakeAllWorking || DatabaseID <= 0)
        return;

    if (chest->RequestTake(DatabaseID)) {
        CIFButton* takeButton = this->m_IRM.GetResObj<CIFButton>(10, 1);
        if (takeButton)
            takeButton->SetEnabledState(false);
    }
}
