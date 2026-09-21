#include <ExtraUI/IFTargetPlayerEquip.h>
#include <BSLib/multibyte.h>
#include <CustomData/CustomDataManager.h>
#include "IFTargetWindowPlayer.h"
#include "IFStatic.h"
#include "unsorted.h"
#include "TextStringManager.h"
#include "ICPlayer.h"
#include "Game.h"

#include "support/MemberFunctionHook.h"
#include "GInterface.h"
#include "IFRenderStatic.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFTargetWindowPlayer, 0x00eea5dc)

GFX_IMPLEMENT_DYNCREATE_FN(CIFTargetWindowPlayer, CIFWnd)

enum {
    GDR_TW_KINDRED_MARK = 10, // CIFStatic
    GDR_TWP_TEXT_NAME = 1, // CIFStatic
    GDR_TARGET_ITEM_INFO_BTN = 111
};

GFX_BEGIN_MESSAGE_MAP(CIFTargetWindowPlayer, CIFWnd)
                    ONG_COMMAND(GDR_TARGET_ITEM_INFO_BTN, &GetTargetPlayerInfo)
GFX_END_MESSAGE_MAP()

void CIFTargetWindowPlayer::ShowTargetWnd()
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    CIFButton* toggleButton =
        m_IRM.GetResObj<CIFButton>(GDR_TARGET_ITEM_INFO_BTN, 1);
    if (equipmentWindow == NULL || toggleButton == NULL)
        return;

    if(!equipmentWindow->IsVisible())
    {
        wnd_pos anchor = toggleButton->GetPos();
        wnd_size panelSize = equipmentWindow->GetSize();
        wnd_size buttonSize = toggleButton->GetSize();

        int screenWidth = 800;
        int screenHeight = 600;
        if (g_CGame != NULL && g_CGame->GetRes().res != NULL)
        {
            screenWidth = g_CGame->GetRes().res->width;
            screenHeight = g_CGame->GetRes().res->height;
        }

        int panelX = anchor.x;
        int panelY = anchor.y;
        const int maximumX = screenWidth - panelSize.width - buttonSize.width;
        const int maximumY = screenHeight - panelSize.height;
        if (panelX > maximumX)
            panelX = maximumX;
        if (panelY > maximumY)
            panelY = maximumY;
        if (panelX < 0)
            panelX = 0;
        if (panelY < 0)
            panelY = 0;

        equipmentWindow->MoveGWnd(panelX, panelY);
        equipmentWindow->ShowGWnd(true);

        toggleButton->MoveGWnd(panelX + panelSize.width, panelY);
        toggleButton->TB_Func_13(
            "interface\\quick_slot\\qsl_hclose_button.ddj", 1, 1);
    }
}
void CIFTargetWindowPlayer::UpdateRenderStatic()
{
    CICUser* pObject = (CICUser*)GetCharacterObjectByID_MAYBE(m_objectId);
    if(pObject != NULL) {
        CIFTargetPlayerEquip * objWnd = m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
        if (objWnd != NULL)
            objWnd->UpdateRenderStatic(pObject);

    }

}
void CIFTargetWindowPlayer::GetTargetPlayerInfo()
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    if (equipmentWindow == NULL)
        return;

    if(!equipmentWindow->IsVisible())
    {
        CICUser* pObject = (CICUser*)GetCharacterObjectByID_MAYBE(m_objectId);
        if(pObject != NULL)
        {
            CMsgStreamBuffer buf(0x169A);
            buf << byte(23);
            buf << pObject->GetName();
            SendMsg(buf);
        }
    }
    else
    {
        Hide();
    }


}
void CIFTargetWindowPlayer::Hide()
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    CIFButton* toggleButton =
        m_IRM.GetResObj<CIFButton>(GDR_TARGET_ITEM_INFO_BTN, 1);

    if (equipmentWindow != NULL)
        equipmentWindow->ShowGWnd(false);
    if (toggleButton != NULL)
    {
        toggleButton->MoveGWnd(this->GetPos().x + 236, this->GetPos().y);
        toggleButton->TB_Func_13(
            "interface\\quick_slot\\qsl_hopen_button.ddj", 1, 1);
    }
}
bool CIFTargetWindowPlayer::OnCreate(long ln) {
    //printf("%s\n", __FUNCTION__);
    //return reinterpret_cast<bool (__thiscall *)(const CIFTargetWindowPlayer *, long)>(0x0069b180)(this, ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iftw_player.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    if (equipmentWindow != NULL)
        equipmentWindow->ShowGWnd(false);
    return true;
}

HOOK_ORIGINAL_MEMBER(0x0069b0d0, &CIFTargetWindowPlayer::FUN_0069b0d0);
void CIFTargetWindowPlayer::FUN_0069b0d0(int objectId) {
    m_objectId = objectId;
    CIGIDObject* pObject = GetCharacterObjectByID_MAYBE(m_objectId);
    if(pObject != NULL) {
        CIFStatic* nameControl = m_IRM.GetResObj<CIFStatic>(GDR_TWP_TEXT_NAME, 1);
        CIFStatic* kindredControl = m_IRM.GetResObj<CIFStatic>(GDR_TW_KINDRED_MARK, 1);
        CIFTargetPlayerEquip* equipmentWindow =
            m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
        CIFButton* toggleButton =
            m_IRM.GetResObj<CIFButton>(GDR_TARGET_ITEM_INFO_BTN, 1);

        if (nameControl != NULL)
            nameControl->SetText(pObject->GetName().c_str());
        if (kindredControl != NULL && pObject->GetCommonData() != NULL)
            kindredControl->TB_Func_13(
                GetKindredTextureFilePath(pObject->GetCommonData()->Country, 0),
                0,
                0);
        if (equipmentWindow != NULL)
            equipmentWindow->ShowGWnd(false);
        if (toggleButton != NULL)
        {
            toggleButton->ShowGWnd(true);
            toggleButton->MoveGWnd(this->GetPos().x + 236, this->GetPos().y);
            toggleButton->TB_Func_13(
                "interface\\quick_slot\\qsl_hopen_button.ddj", 1, 1);
        }

        if(pObject->IsSame(GFX_RUNTIME_CLASS(CICUser)))
        {
            CICUser* pUser = (CICUser*)pObject;
            if(m_CustomDataManager->m_EventMapSettings.find(pObject->GetRegion().r) != m_CustomDataManager->m_EventMapSettings.end())
            {
                if(m_CustomDataManager->m_EventMapSettings[pObject->GetRegion().r].EventSuit)
                {
                    if(g_pMyPlayerObj != NULL)
                    {
                        byte MineCapeType = g_pMyPlayerObj->m_PVPCapeState;
                        if(MineCapeType != pUser->m_PVPCapeState)
                        {
                            if(pUser->m_PVPCapeState == 4)
                            {
                                if(pObject->GetName() != g_pMyPlayerObj->GetCharName())
                                {
                                    m_IRM.GetResObj<CIFStatic>(GDR_TWP_TEXT_NAME, 1)->SetText(KmtGetText(L"UIIT_KMT_WHITE_TEAM"));
                                    return;
                                }
                            }
                            else if(pUser->m_PVPCapeState == 3)
                            {
                                if(pObject->GetName() != g_pMyPlayerObj->GetCharName())
                                {
                                    m_IRM.GetResObj<CIFStatic>(GDR_TWP_TEXT_NAME, 1)->SetText(KmtGetText(L"UIIT_KMT_BLUE_TEAM"));
                                    return;
                                }
                            }
                            else if(pUser->m_PVPCapeState == 2)
                            {
                                if(pObject->GetName() != g_pMyPlayerObj->GetCharName())
                                {
                                    m_IRM.GetResObj<CIFStatic>(GDR_TWP_TEXT_NAME, 1)->SetText(KmtGetText(L"UIIT_KMT_GREEN_TEAM"));
                                    return;
                                }
                            }
                            else if(pUser->m_PVPCapeState == 1)
                            {
                                if(pObject->GetName() != g_pMyPlayerObj->GetCharName())
                                {
                                    m_IRM.GetResObj<CIFStatic>(GDR_TWP_TEXT_NAME, 1)->SetText(KmtGetText(L"UIIT_KMT_RED_TEAM"));
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
void CIFTargetWindowPlayer::UpdateItemSlot(int ItemSlot, CSOItem* tempItemInfo, std::n_string CN16)
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    if (equipmentWindow != NULL)
        equipmentWindow->UpdateItemSlot(ItemSlot, tempItemInfo, CN16);
}
void CIFTargetWindowPlayer::UpdateCharName(std::n_string CN16)
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    if (equipmentWindow == NULL)
        return;

    if (equipmentWindow->TargetCharName != "" &&
        equipmentWindow->TargetCharName != CN16)
    {
        equipmentWindow->ClearSlots();
    }

    equipmentWindow->UpdateTargetName(CN16);
}

void CIFTargetWindowPlayer::ClearTargetEquipment()
{
    CIFTargetPlayerEquip* equipmentWindow =
        m_IRM.GetResObj<CIFTargetPlayerEquip>(1951, 1);
    if (equipmentWindow != NULL)
        equipmentWindow->ClearSlots();
}
