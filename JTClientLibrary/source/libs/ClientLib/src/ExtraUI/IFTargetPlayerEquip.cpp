//
// Created by YUMBUL on 6.01.2025.
//

#include "IFTargetPlayerEquip.h"

#include <Game.h>
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <BSLib/multibyte.h>
#include <CustomData/CustomSettingManager.h>
#include <IFFrame.h>
#include <IFNormalTile.h>
#include <IFRenderStatic.h>
#include <IFStretchWnd.h>
#include <ICUser.h>
#include <IFTargetWindowPlayer.h>



GFX_IMPLEMENT_DYNCREATE(CIFTargetPlayerEquip, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFTargetPlayerEquip, CIFMainFrame)
                   // ONG_COMMAND(11, &On_BtnClick)
//                    ONG_COMMAND(1000, &On_BtnClick)

GFX_END_MESSAGE_MAP()

#define WeaponSlot 106
#define ShieldSlot 107

namespace {
    const int WINDOW_WIDTH = 260;
    const int WINDOW_HEIGHT = 410;
    const int TARGET_NAME_ID = 7;
    const int PREVIEW_FRAME_ID = 8;
    const int NAME_BORDER_ID = 9;
    const int NAME_TILE_ID = 10;
    const int EQUIPMENT_SLOT_IDS[] = {
        100, 101, 102, 103, 104, 105,
        106, 107, 109, 110, 111, 112
    };
    const int EQUIPMENT_SLOT_COUNT =
        sizeof(EQUIPMENT_SLOT_IDS) / sizeof(EQUIPMENT_SLOT_IDS[0]);
    const int EQUIPMENT_DECORATION_IDS[] = {
        1000, 1001, 1002, 1003, 1004, 1005,
        1066, 1077, 1009, 1010, 1011, 1012
    };
    const int EQUIPMENT_DECORATION_COUNT =
        sizeof(EQUIPMENT_DECORATION_IDS) /
        sizeof(EQUIPMENT_DECORATION_IDS[0]);

    const D3DCOLOR COLOR_TITLE = D3DCOLOR_ARGB(255, 255, 255, 255);
}

CIFTargetPlayerEquip::CIFTargetPlayerEquip(void){
    TargetCharName = std::n_string();
    pObject = NULL;
}
CIFTargetPlayerEquip::~CIFTargetPlayerEquip(void){

}
bool CIFTargetPlayerEquip::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iftargetplayerequipment.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetGWndSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetText(KmtGetText(L"UIIT_KMT_LIVE_EQUIPMENT_CHAMBER"));

    if (m_pTitleText != NULL)
    {
        m_pTitleText->m_FontTexture.SetColor(COLOR_TITLE);
        m_pTitleText->BringToFront();
    }

    if (m_pCloseBtn != NULL)
    {
        m_pCloseBtn->TB_Func_13(
            "interface\\ifcommon\\com_windowclose.ddj", 0, 0);
        m_pCloseBtn->SetGWndSize(16, 16);
        m_pCloseBtn->MoveGWnd(
            GetPos().x + WINDOW_WIDTH - 26,
            GetPos().y + 9);
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }

    CIFFrame* contentFrame = m_IRM.GetResObj<CIFFrame>(4, 1);
    if (contentFrame != NULL)
        contentFrame->SetClickable(false);

    CIFNormalTile* panelBackground = m_IRM.GetResObj<CIFNormalTile>(5, 1);
    if (panelBackground != NULL)
        panelBackground->SetClickable(false);

    CIFNormalTile* previewBackground = m_IRM.GetResObj<CIFNormalTile>(6, 1);
    if (previewBackground != NULL)
        previewBackground->SetClickable(false);

    CIFFrame* previewFrame = m_IRM.GetResObj<CIFFrame>(PREVIEW_FRAME_ID, 1);
    if (previewFrame != NULL)
        previewFrame->SetClickable(false);

    CIFStretchWnd* nameBorder =
        m_IRM.GetResObj<CIFStretchWnd>(NAME_BORDER_ID, 1);
    if (nameBorder != NULL)
        nameBorder->SetClickable(false);

    CIFNormalTile* nameTile =
        m_IRM.GetResObj<CIFNormalTile>(NAME_TILE_ID, 1);
    if (nameTile != NULL)
        nameTile->SetClickable(false);

    CIFStatic* targetName = m_IRM.GetResObj<CIFStatic>(TARGET_NAME_ID, 1);
    if (targetName != NULL)
    {
        targetName->SetClickable(false);
        targetName->m_FontTexture.SetColor(COLOR_TITLE);
        targetName->SetText(TO_NWSTRING(TargetCharName).c_str());
        targetName->BringToFront();
    }

    CIFRenderStatic* render = m_IRM.GetResObj<CIFRenderStatic>(500, 1);
    if (render != NULL)
        render->SetClickable(false);

    for (int i = 0; i < EQUIPMENT_DECORATION_COUNT; ++i)
    {
        CIFStatic* decoration =
            m_IRM.GetResObj<CIFStatic>(EQUIPMENT_DECORATION_IDS[i], 1);
        if (decoration != NULL)
            decoration->SetClickable(false);
    }

    for (int i = 0; i < EQUIPMENT_SLOT_COUNT; ++i)
    {
        CIFSlotWithHelp* slot =
            m_IRM.GetResObj<CIFSlotWithHelp>(EQUIPMENT_SLOT_IDS[i], 1);
        if (slot == NULL)
            continue;

        slot->SetClickable(false);
        slot->SetType(19);
    }

    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}

undefined1 CIFTargetPlayerEquip::OnCloseWnd()
{
    CGWndBase* parent = GetParentControl();
    if (parent != NULL &&
        parent->IsSame(GFX_RUNTIME_CLASS(CIFTargetWindowPlayer)))
    {
        static_cast<CIFTargetWindowPlayer*>(parent)->Hide();
    }

    return CIFWnd::OnCloseWnd();
}

void CIFTargetPlayerEquip::ClearSlots()
{
    for (int i = 0; i < EQUIPMENT_SLOT_COUNT; ++i)
    {
        CIFSlotWithHelp* slot =
            m_IRM.GetResObj<CIFSlotWithHelp>(EQUIPMENT_SLOT_IDS[i], 1);
        if (slot == NULL)
            continue;

        slot->SetSlotData(NULL);
        slot->SetSlotType(0);
        slot->SetInventorySlotType(0);
        slot->TB_Func_13("", 0, 0);
    }
}
void CIFTargetPlayerEquip::UpdateItemSlot(int ItemSlot, CSOItem* tempItemInfo, std::n_string CN16)
{
    if (ItemSlot < 0 || ItemSlot > 12 || ItemSlot == 8 || tempItemInfo == NULL)
        return;

    CIFSlotWithHelp* slot =
        m_IRM.GetResObj<CIFSlotWithHelp>(100 + ItemSlot, 1);
    if (slot != NULL)
        slot->SetSlotData(tempItemInfo);

}

void CIFTargetPlayerEquip::UpdateTargetName(const std::n_string& characterName)
{
    TargetCharName = characterName;

    CIFStatic* targetName = m_IRM.GetResObj<CIFStatic>(TARGET_NAME_ID, 1);
    if (targetName != NULL)
        targetName->SetText(TO_NWSTRING(TargetCharName).c_str());
}
void CIFTargetPlayerEquip::UpdateMenuSize()
{

}
void CIFTargetPlayerEquip::UpdateRenderStatic(CICUser* pObjectx)
{
    pObject = pObjectx;
    CIFRenderStatic* render = m_IRM.GetResObj<CIFRenderStatic>(500, 1);
    if (render == NULL || pObject == NULL)
        return;

    render->Clear();
    this->StartTimer(1, 250);

}

void CIFTargetPlayerEquip::OnTimer(int timerId)
{
    if(timerId == 1)
    {
        this->KillTimer(1);
        CIFRenderStatic* render = m_IRM.GetResObj<CIFRenderStatic>(500, 1);
        if (render == NULL || pObject == NULL || pObject->GetCommonData() == NULL)
            return;

        render->FUN_005602c0(
            render->GetCharacterObj(
                pObject->GetCommonData()->RefObjectId,
                pObject->m_pCCObjAnimation),
            0);
        render->Test1();

        undefined4 uStack12;
        uStack12 = 0;
        render->Test(&uStack12);
        render->Test2(&uStack12, 0x420d1000);

        render->field_0x430 = 0x43fa0000;
        render->Test3(0, 0);

        render->yukariasagi = 15.000;
        render->N00000609 = 1;

    }
}
