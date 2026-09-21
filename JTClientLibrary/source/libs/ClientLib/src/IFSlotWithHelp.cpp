///******************************************************************************
/// \File IFSlotWithHelp.cpp
///
/// \Desc
///
/// \Author kyuubi09 on 6/21/2023.
///
/// \Copyright Copyright © 2023 SRO_DevKit.
///
///******************************************************************************

#include <GFX3DFunction/RStateMgr.h>
#include <GFX3DFunction/RTLoading.h>
#include <SimpleViewer/VBDynamic.h>
#include <Macro/IFMacroMenu.h>
#include <algorithm>
#include <GlobalItemLinking/GlobalItemLinking.h>
#include <sstream>
#include <BSLib/multibyte.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomData/CustomSettingManager.h>
#include <LockItems/IFNewMsgBox.h>
#include <LockItems/IFItemLocker.h>
#include <LockItems/IFItemUnlocker.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include <MacroAlchemy/IFMacroAlchemySlot.h>

#include "IFSlotWithHelp.h"
#include "GInterface.h"
#include "IFWholeChat.h"
#include "TextStringManager.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"
#include "IFPlayerMiniInfo.h"
#include "IFPetMiniInfo.h"
#include "ICCos.h"
#include "GEffSoundBody.h"
#include "IFChatViewer.h"
#include "IFEquipment.h"

namespace {
int GetBestSlotEquipmentIndex(unsigned int tid) {
    switch (tid) {
        case 0x132C:
        case 0x1B2C:
        case 0x232C:
        case 0x2B2C:
        case 0x332C:
        case 0x6B2C:
        case 0x3B2C:
        case 0x432C:
        case 0x4B2C:
        case 0x632C:
        case 0x532C:
        case 0x5B2C:
        case 0x732C:
        case 0x7B2C:
            return 6;
        case 0x0A2C:
        case 0x122C:
            return 7;
        case 0x0AAC:
        case 0x0E2C:
            return 9;
        case 0x12AC:
        case 0x162C:
            return 10;
        case 0x1AAC:
        case 0x1E2C:
            return 11;
        case 0x0D2C:
        case 0x092C:
        case 0x0CAC:
        case 0x0DAC:
        case 0x08AC:
        case 0x09AC:
            return 0;
        case 0x15AC:
        case 0x10AC:
        case 0x152C:
        case 0x11AC:
        case 0x112C:
        case 0x14AC:
            return 2;
        case 0x1CAC:
        case 0x192C:
        case 0x18AC:
        case 0x1DAC:
        case 0x19AC:
        case 0x1D2C:
            return 1;
        case 0x25AC:
        case 0x21AC:
        case 0x252C:
        case 0x212C:
        case 0x24AC:
        case 0x20AC:
            return 4;
        case 0x2CAC:
        case 0x292C:
        case 0x28AC:
        case 0x2DAC:
        case 0x2D2C:
        case 0x29AC:
            return 3;
        case 0x30AC:
        case 0x312C:
        case 0x31AC:
        case 0x34AC:
        case 0x352C:
        case 0x35AC:
            return 5;
        default:
            return -1;
    }
}

int GetBestSlotItemPoint(CSOItem *item) {
    if (!item || !item->GetObjectData())
        return 0;

    ObjectData *data = item->GetObjectData();
    return item->m_OptLevel + data->ReqLevel1 + (data->Rarity * 3);
}

void RenderSlotOverlay(const std::n_string &path, void *vertices) {
    const IDirect3DBaseTexture9 *texture = Fun_CacheTexture_Create(path);
    g_RStateMgr.SetTextureForStage(0, texture);

    int local_159 = 0;
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(vertices, 0x60, &local_159) != 0) {
        IDirect3DVertexBuffer9 *vertexBuffer = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, vertexBuffer, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_159, 2);
    }
}
}


void CIFSlotWithHelp::SetSlotData(CSOItem *pItemSocket) {
    reinterpret_cast<void (__thiscall *)(CIFSlotWithHelp *, CSOItem *)>(0x006871d0)(this, pItemSocket);
}

int CIFSlotWithHelp::GetItemSourceParentWindowId() const {
    return m_itemSourceParentWindowId;
}

const CSOItem &CIFSlotWithHelp::GetMyItem() const {
    return m_myItem;
}

CSOItem *CIFSlotWithHelp::GetItem() const {
    return ItemInfo;
}
void CIFSlotWithHelp::CopySlot(CIFSlotWithHelp* pSlot) {
    reinterpret_cast<int(__thiscall *)(CIFSlotWithHelp *, CIFSlotWithHelp *)>(0x00687a10)(this, pSlot);
}
void CIFSlotWithHelp::UseItem() {
    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *)>(0x00676570)(this);
}
void CIFSlotWithHelp::SetSlotVisual(undefined4 pSlot) {
    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, undefined4)>(0x00667ac0)(this, pSlot);
}
void CIFSlotWithHelp::ClearSlotEmpty() {

}
void CIFSlotWithHelp::GetSkillInfo(int SkillID) {
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*, int)>(0x0066c3c0)(this, SkillID);
}
int CIFSlotWithHelp::GetType() {
    return this->Type;
}
int CIFSlotWithHelp::GetParentWindowId() {
    return reinterpret_cast<int(__thiscall*)(CIFSlotWithHelp*)>(0x00666700)(this);
}
void CIFSlotWithHelp::ClearSlot()
{
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*)>(0x0067ece0)(this);
}
void CIFSlotWithHelp::FUN_00682040() {
    //  printf("Use Item Type : %d | Slot : %d | SlotType : %d SkillID : %d ItemID : %d \n", this->Type, this->Slot, this->SlotType, this->m_SkillID);
    if(this->UniqueID() == SkillListSlot || this->UniqueID() == 103777 || this->UniqueID() == 103977 || this->UniqueID() == 104177
       || this->UniqueID() == 104377 || this->UniqueID() == 104577 || this->UniqueID() == 100050)
    {
        return;
    }
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*)>(0x00682040)(this);
}
int CIFSlotWithHelp::GetInventorySlotIndex() const {
    return m_inventorySlotIndex;
}

int CIFSlotWithHelp::GetSlot() {
    return this->SlotNum;
}
void CIFSlotWithHelp::GetIconSlot(int Type, int a1) {
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*, int, int)>(0x00687bc0)(this, Type, a1);
}
int CIFSlotWithHelp::GetSlotType() {
    return reinterpret_cast<int(__thiscall*)(CIFSlotWithHelp*)>(0x00666720)(this);
}
int CIFSlotWithHelp::GetInventorySlotType() {
    return reinterpret_cast<int(__thiscall*)(CIFSlotWithHelp*)>(0x00666800)(this);
}

void CIFSlotWithHelp::FUN_006809a00()
{
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*)>(0x006809a0)(this);
}

int CIFSlotWithHelp::GetSkillSlotInDex() {
    return reinterpret_cast<int(__thiscall*)(CIFSlotWithHelp*)>(0x00666730)(this);
}

void CIFSlotWithHelp::SetSlot(int Slot) {
    this->SlotNum = Slot;
}
void CIFSlotWithHelp::SetType(int value) {
    this->Type = value;
}


void CIFSlotWithHelp::SetInventorySlotType(int SlotType) {
    //  printf("Slot Type : %d\n", SlotType);
    reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*, int)>(0x006667f0)(this, SlotType);

}
void CIFSlotWithHelp::SetSlotType(int Type) {
    return reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*, int)>(0x00666710)(this, Type);
}
void CIFSlotWithHelp::WriteItemType5(class CIFHelperBubbleWindow *window) {

    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = L"\nCharacter-bound\n";
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }


    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x00682ba0)(this, window);
}
void CIFSlotWithHelp::WriteItemType4(class CIFHelperBubbleWindow *window) {

    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = L"\nCharacter-bound\n";
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }


    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x00685f9a)(this, window);
}
void CIFSlotWithHelp::WriteItemType3(class CIFHelperBubbleWindow *window) {

    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = L"\nCharacter-bound\n";
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }


    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x00682410)(this, window);
}
void CIFSlotWithHelp::WriteItemType2(class CIFHelperBubbleWindow *window) {

    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = L"\nCharacter-bound\n";
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }


    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x006821f0)(this, window);
}
void CIFSlotWithHelp::WriteItemType(class CIFHelperBubbleWindow *window) {


    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = KmtGetText(L"UIIT_KMT_CHARACTER_BOUND");
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }
    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x0066f850)(this, window);



}
int CIFSlotWithHelp::OnMouseRightUPIMPL(int a1, int x, int y) {
    CIFSlotWithHelp *slotptr = (CIFSlotWithHelp *) g_CurrentIfUnderCursor;
    //printf("%d %d\n", slotptr->CIFEquipmentSlotNum, (slotptr->UniqueID() - 87));
    if (slotptr)
    {
        if (slotptr->ItemInfo != NULL) {
            // printf("%hu \n",slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value);
            int slotId = (slotptr->UniqueID() - 87);
            if ((slotId >= 13 && slotId <= 108)) {
                if (slotptr->ItemInfo->GetItemData() != NULL) {
                    if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xC6ED) {
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->UpdateMenuSize();
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->SetItemInfo(slotId - 13, 0xC6ED);

                        g_pCGInterface->GetGuiFromList<CIFNewMsgBox>(1394)->ShowGWnd(true);
                        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
                    }
                    else if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xCEED) {
                        if (g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->IsVisible()) {
                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->ShowGWnd(false);
                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->Clear();
                            CGEffSoundBody::get()->PlaySound(L"snd_window_close");
                        }
                        if(!g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->IsVisible())
                        {
                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->UpdateMenuSize();
                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->Clear();
                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->SetItemInfo(slotId - 13, 0xCEED);

                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->ShowGWnd(true);
                            CGEffSoundBody::get()->PlaySound(L"snd_window_open");
                        }


                    }
                    else if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xD6ED) {
                        if (g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->IsVisible())
                        {
                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->ShowGWnd(false);
                            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->Clear();
                            CGEffSoundBody::get()->PlaySound(L"snd_window_close");
                        }
                        if(!g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->IsVisible())
                        {
                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->UpdateMenuSize();
                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->Clear();
                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->SetItemInfo(slotId - 13, 0xD6ED);

                            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->ShowGWnd(true);
                        }
                    }
                }

            } else if (slotptr->m_inventorySlotIndex >= 0 && slotptr->m_inventorySlotIndex <= 95) {
                if (slotptr->ItemInfo->GetItemData() != NULL) {
                    if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xC6ED) {
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->UpdateMenuSize();
                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->SetItemInfo(slotptr->m_inventorySlotIndex, 0xC6ED);

                        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->ShowGWnd(true);
                    } else if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xCEED) {
                        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->UpdateMenuSize();
                        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->SetItemInfo(slotId - 13, 0xCEED);

                        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->ShowGWnd(true);
                    } else if (slotptr->ItemInfo->GetItemData()->m_typeId.m_type_id_value == 0xD6ED) {
                      g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->UpdateMenuSize();
                      g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->Clear();
                      g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->SetItemInfo(slotId - 13, 0xD6ED);

                      g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->ShowGWnd(true);
                    }
                }
            }
        }
    }

    return reinterpret_cast<int(__thiscall *)(CIFSlotWithHelp *, int, int, int)>(0x00685ee0)(this, a1, x, y);
}

void CIFSlotWithHelp::FUN_006821f02(CIFHelperBubbleWindow *window) {

    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x0067a970)(this, window);
    for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
        it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
    {
        CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
        if (pUser != NULL) {
            static const CCharacterData *data = NULL;
            data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
            if(data)
            {
                std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                {
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_1) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_1);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_1)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_2) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_2);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_2)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_3) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_3);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_3)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_4) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_4);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_4)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_5) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_5);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_5)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_1) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_Active_Level_1);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_Active_Level_1)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                    if (this->m_SkillID == m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_2) {
                        wchar_t buffer123[255];
                        swprintf_s(buffer123, KmtGetText(L"UIIT_KMT_REQUIRED_PET_LEVEL_IS_VALUE"), m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_Active_Level_2);

                        std::wstring mymsg22312321;
                        mymsg22312321.assign(buffer123);
                        if(data->GetData().Level < m_CustomDataManager->m_RefFellowPetSystem[NameStr].SelfSkill_Active_Level_2)
                        {
                            window->WriteLine(mymsg22312321, 0, 0xff4a4a, 0, 1);
                        }
                        else
                        {
                            window->WriteLine(mymsg22312321, 0, 0xFFFFFF, 0, 1);
                        }
                        break;
                    }
                }
            }
        }
    }
}

void CIFSlotWithHelp::WriteItemClass(class CIFHelperBubbleWindow *window) {

    /*CSOItem *myitem = this->ItemInfo;
   */


    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x0066f0c0)(this, window);
    CSOItem *myitem = this->ItemInfo;

    if (myitem->GetItemData()->m_param15 == 1)
    {
        switch (myitem->GetObjectData()->m_typeId.m_type_id_value)
        {

            case 0x132C: //ch sword
            case 0x1B2C: //ch blade
            case 0x232C: //ch spear
            case 0x2B2C: //ch glaive
            case 0x332C: //ch bow
            case 0x6B2C: //eu dagger
            case 0x3B2C: //eu 1h
            case 0x432C: //eu 2h
            case 0x4B2C: //eu axe
            case 0x632C: //eu crossbow
            case 0x532C: //eu warlock
            case 0x5B2C: //eu 2hstaff
            case 0x732C: //eu harp
            case 0x7B2C: //eu staff
            case 0x0A2C: //ch shield
            case 0x122C: //eu shield
            {
                wchar_t buffer2[45];
                std::wstring mymsg2;
                swprintf_s(buffer2, sizeof(buffer2), L"%s: %s", KmtGetText(L"UIIT_KMT_GLOW_TYPE"), myitem->GetItemData()->m_desc15_128.c_str());
                mymsg2.assign(buffer2);
                window->WriteLine(mymsg2, 0xff, 0xffefdaa4, 0, 1);

                break;
            }
        }
        if (myitem->GetItemData()->m_param16 == 1)
        {
            switch (myitem->GetObjectData()->m_typeId.m_type_id_value)
            {
                case 0x132C: //ch sword
                case 0x1B2C: //ch blade
                case 0x232C: //ch spear
                case 0x2B2C: //ch glaive
                case 0x332C: //ch bow
                case 0x6B2C: //eu dagger
                case 0x3B2C: //eu 1h
                case 0x432C: //eu 2h
                case 0x4B2C: //eu axe
                case 0x632C: //eu crossbow
                case 0x532C: //eu warlock
                case 0x5B2C: //eu 2hstaff
                case 0x732C: //eu harp
                case 0x7B2C: //eu staff
                case 0x0A2C: //ch shield
                case 0x122C: //eu shield
                {
                    wchar_t buffer2[45];
                    std::wstring mymsg2;
                    swprintf_s(buffer2, sizeof(buffer2), L"%s: %s", KmtGetText(L"UIIT_KMT_MODEL_TYPE"), myitem->GetItemData()->m_desc16_128.c_str());
                    mymsg2.assign(buffer2);
                    window->WriteLine(mymsg2, 0xff, 0xffefdaa4, 0, 1);

                    break;
                }
            }
        }
    }
    else if (myitem->GetItemData()->m_param16 == 1)
    {
        switch (myitem->GetObjectData()->m_typeId.m_type_id_value)
        {
            case 0x132C: //ch sword
            case 0x1B2C: //ch blade
            case 0x232C: //ch spear
            case 0x2B2C: //ch glaive
            case 0x332C: //ch bow
            case 0x6B2C: //eu dagger
            case 0x3B2C: //eu 1h
            case 0x432C: //eu 2h
            case 0x4B2C: //eu axe
            case 0x632C: //eu crossbow
            case 0x532C: //eu warlock
            case 0x5B2C: //eu 2hstaff
            case 0x732C: //eu harp
            case 0x7B2C: //eu staff
            case 0x0A2C: //ch shield
            case 0x122C: //eu shield
            {
                wchar_t buffer2[45];
                std::wstring mymsg2;
                swprintf_s(buffer2, sizeof(buffer2), L"%s: %s", KmtGetText(L"UIIT_KMT_MODEL_TYPE"), myitem->GetItemData()->m_desc16_128.c_str());
                mymsg2.assign(buffer2);
                window->WriteLine(mymsg2, 0xff, 0xffefdaa4, 0, 1);

                break;
            }
        }
    }

  /*  if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = KmtGetText(L"UIIT_KMT_CHARACTER_BOUND");
            window->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }
*/


}
void CIFSlotWithHelp::SetRareName(class CIFHelperBubbleWindow* window)
{

    CSOItem* myitem = this->ItemInfo;

    bool isFoundAndUpdated = false;

    std::wstring NewSealName;
    std::wstring NewSealName2;
        std::wstring GlowType = KmtGetText(L"UIIT_KMT_DEFAULT");
        std::wstring ModelType = KmtGetText(L"UIIT_KMT_DEFAULT");
    unsigned __int32 NewSealHex;
    unsigned __int32 NewSealHex2;

    if (myitem->GetItemData()->m_param11 == 1)
    {
        if (myitem->GetItemData()->m_param11 == 1)
        {
            NewSealName = myitem->GetItemData()->m_desc11_128.c_str();

            if (myitem->GetItemData()->m_param12 == 1)
            {
                std::wstringstream buffers(myitem->GetItemData()->m_desc12_128.c_str());
                buffers >> std::hex >> NewSealHex;
            }
            else
            {
                NewSealHex = 0xffffd953;
            }

            window->WriteLine(NewSealName, 0, NewSealHex, 0, 2);
        }

        if (myitem->GetItemData()->m_param13 == 1)
        {
            NewSealName2 = myitem->GetItemData()->m_desc13_128.c_str();

            if (myitem->GetItemData()->m_param14 == 1)
            {
                std::wstringstream buffers1(myitem->GetItemData()->m_desc14_128.c_str());
                buffers1 >> std::hex >> NewSealHex2;
            }
            else
            {
                NewSealHex2 = 0xffffd953;
            }
            window->WriteLine(NewSealName2, 0, NewSealHex2, 0, 2);
        }
    }
    else
    {

        reinterpret_cast<void(__thiscall*)(CIFSlotWithHelp*, CIFHelperBubbleWindow*)>(0x0066fa60)(this, window);

    }
}
void CIFSlotWithHelp::WriteItemname(class CIFHelperBubbleWindow *a1) {

    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x0067a570)(this, a1);
    CSOItem *myitem = this->ItemInfo;
    if(m_Settings->EnableCharacterBound)
    {
        if (myitem->GetItemData()->CanDrop == 0 && myitem->GetItemData()->CanBorrow == 0 && myitem->GetItemData()->CanTrade == 0) {

            std::wstring mymsg2 = L"\nCharacter-bound\n";
            a1->WriteLine(mymsg2, 0xff, 0xff4a4a, 0, 1);
        }
    }
}

void CIFSlotWithHelp::RenderMyselfIMPL() {

    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *)>(0x00685e50)(this);
    bool showBestSlot = false;
    if (this->GetParentWindowId() == 70 && this->ItemInfo && g_pCGInterface) {
        int equipmentSlot = GetBestSlotEquipmentIndex(this->ItemInfo->GetObjectData()->m_typeId.m_type_id_value);
        if (equipmentSlot >= 0) {
            CSOItem *equippedItem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(equipmentSlot);
            if (equippedItem) {
                showBestSlot = GetBestSlotItemPoint(this->ItemInfo) > GetBestSlotItemPoint(equippedItem);
            }
        }
    }

    if (showBestSlot) {
        SYSTEMTIME time;
        GetSystemTime(&time);
        int frame = (time.wMilliseconds / 111) + 1;
        std::ostringstream frameStream;
        frameStream << frame;
        std::n_string imgpath;
        imgpath.assign(("interface\\ifcommon\\thaidu0ngpr0\\bestslot\\bestslot_" + frameStream.str() + ".ddj").c_str());
        RenderSlotOverlay(imgpath, &N0000062D);
    }

    if (this->ItemInfo != NULL)
    {
        if (this->ItemInfo->ItemLocked == 1)
        {

            std::n_string imgpath = "clientlibrary\\slotup\\com_item_lock.ddj";
            const IDirect3DBaseTexture9 *puVarxx = Fun_CacheTexture_Create(imgpath);
            g_RStateMgr.SetTextureForStage(0, puVarxx);
            int local_159 = 0;
            //printf("%d\n",(float *)((int)this + 0x24c));
            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(&N0000062D, 0x60, &local_159) != 0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_159, 2);
            }
        }

    }
    if (this != NULL)
    {
        if (this->SlotisLocked == 10)
        {

            std::n_string imgpath = "clientlibrary\\mall\\mall_pre_select.ddj";
            const IDirect3DBaseTexture9 *puVarxx = Fun_CacheTexture_Create(imgpath);
            g_RStateMgr.SetTextureForStage(0, puVarxx);
            int local_159 = 0;
            //printf("%d\n",(float *)((int)this + 0x24c));
            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(&N0000062D, 0x60, &local_159) != 0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_159, 2);
            }
        }

    }
    if (this->GetType() == 73) {
        if (this->m_SkillID && this->m_SkillID != 0 && g_pCGInterface) {
            if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE) {
                SYSTEMTIME time2;
                GetSystemTime(&time2);
                int mss = time2.wMilliseconds;
                int loopp = mss / 152;
                std::ostringstream temps;
                int imgs = loopp + 1;
                temps << imgs;
                std::string imgpath = "clientlibrary\\fellowpets\\edge_" + temps.str() + ".ddj";
                std::wstring imgpaths = TO_WSTRING(imgpath).c_str();
                const IDirect3DBaseTexture9 *puVarxx = Fun_CacheTexture_Create(TO_NSTRING(imgpaths));
                g_RStateMgr.SetTextureForStage(0, puVarxx);
                int local_159 = 0;
                //printf("%d\n",(float *)((int)this + 0x24c));
                if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(&N0000062D, 0x60, &local_159) != 0) {
                    IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                    g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                    g_RStateMgr.FUN_00470060(6, local_159, 2);
                }
            }
        }
    }
}


enum SKILL_SLOTS
{
    GDR_SKILL_SLOT_UNIQUE_1 = 103777,
    GDR_SKILL_SLOT_UNIQUE_2 = 103977,
    GDR_SKILL_SLOT_UNIQUE_3 = 104177,
    GDR_SKILL_SLOT_UNIQUE_4 = 104377,
    GDR_SKILL_SLOT_UNIQUE_5 = 104577,
};
int CIFSlotWithHelp::OnMouseLeftUpIMPL(int a1, int x, int y)
{

    CIFSlotWithHelp *slotptr = (CIFSlotWithHelp *) g_CurrentIfUnderCursor;
    //printf("%p \n", slotptr);
    //printf("current if under cursor = %02x\n", slotptr);
    short ALTState = GetKeyState(VK_MENU);
    if (slotptr && (ALTState == -127 || ALTState == -128)) {
        if (*(DWORD *) slotptr == 0xDA18E4 || *(DWORD *) slotptr == 0xD9E0FC) {
            /*   if (*(DWORD*)slotptr == 0xD9E0FC) {
                 CGWndBase* shit;
                 if (shit) {
                     slotptr = reinterpret_cast<CIFSlotWithHelp*>(shit->m_parentControl);
                 }
             }*/

            int slotId = (slotptr->UniqueID() - 87);
            //printf("%d\n", slotId);
            if (slotptr->Type == 70 && (slotId >= 13 && slotId <= 108)) {
                if (slotptr->ItemInfo) {

                    if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                    {
                        CNIFEnchantWnd * ptr = g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168);
                        if(ptr->GetGuiFromList<CIFMacroAlchemySlot>(5022) != NULL)
                        {
                            if(ptr->GetGuiFromList<CIFMacroAlchemySlot>(5022)->IsVisible())
                            {
                                if(slotptr->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                                   && slotptr->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                                   && slotptr->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                                {
                                    if(slotptr->ItemInfo->GetItemData()->Rarity != 0)
                                    {
                                        ptr->GetGuiFromList<CIFMacroAlchemySlot>(5022)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                                        {
                                            g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->ShowEnchancerAndSpoofSlot();
                                        }
                                        return true;
                                    }
                                    else
                                    {
                                        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THIS_PLACE_FOR_A_SOX_ITEM"));
                                        g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,KmtGetText(L"UIIT_KMT_THIS_PLACE_FOR_A_SOX_ITEM"));
                                    }
                                }
                                else if(slotptr->ItemInfo->GetItemData()->IsEnhancer())
                                {
                                    if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                                    {
                                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() == WEAPON)
                                        {
                                            if(slotptr->ItemInfo->GetItemData()->IsEnhancer() && slotptr->ItemInfo->GetItemData()->m_param3 == 100663296)
                                            {
                                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(slotptr->ItemInfo->GetItemData()->m_param1))
                                                {
                                                    ptr->GetGuiFromList<CIFMacroAlchemySlot>(5023)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
                                                    return true;
                                                }
                                                else
                                                {
                                                    g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                    g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                }

                                            }
                                            else
                                            {
                                                g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));
                                                g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));

                                            }
                                        }
                                        else if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() == ARMOR)
                                        {
                                            if(slotptr->ItemInfo->GetItemData()->IsEnhancer() && slotptr->ItemInfo->GetItemData()->m_param3 == 16909056)
                                            {
                                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(slotptr->ItemInfo->GetItemData()->m_param1))
                                                {
                                                    ptr->GetGuiFromList<CIFMacroAlchemySlot>(5023)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
                                                    return true;
                                                }
                                                else
                                                {
                                                    g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                    g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                }
                                            }
                                            else
                                            {
                                                g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));
                                                g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));

                                            }
                                        }
                                        else if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() == ACC)
                                        {
                                            if(slotptr->ItemInfo->GetItemData()->IsEnhancer() && slotptr->ItemInfo->GetItemData()->m_param3 == 83886080)
                                            {
                                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(slotptr->ItemInfo->GetItemData()->m_param1))
                                                {
                                                    ptr->GetGuiFromList<CIFMacroAlchemySlot>(5023)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
                                                    return true;
                                                }
                                                else
                                                {
                                                    g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                    g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                }
                                            }
                                            else
                                            {
                                                g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));
                                                g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));

                                            }
                                        }
                                        else if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() == SHIELD)
                                        {
                                            if(slotptr->ItemInfo->GetItemData()->IsEnhancer() && slotptr->ItemInfo->GetItemData()->m_param3 == 67108864)
                                            {
                                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(slotptr->ItemInfo->GetItemData()->m_param1))
                                                {
                                                    ptr->GetGuiFromList<CIFMacroAlchemySlot>(5023)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
                                                    return true;
                                                }
                                                else
                                                {
                                                    g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                    g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                }
                                            }
                                            else
                                            {
                                                g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));
                                                g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_RECIPE_MISMATCH"));

                                            }
                                        }
                                        else
                                        {
                                            g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_LOAD_EQUIP_FIRST"));
                                            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_LOAD_EQUIP_FIRST"));
                                        }
                                    }
                                }
                                else if(slotptr->ItemInfo->GetItemData()->IsProofStone())
                                {
                                    if(slotptr->ItemInfo->GetItemData()->IsProofStone())
                                    {
                                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                                        {
                                            if(slotptr->ItemInfo->GetItemData()->IsProofStone())
                                            {
                                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() != 0 && g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetEnhancerInfoFromSlot())
                                                {
                                                    if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeConditionForProofStone(slotptr->ItemInfo->GetItemData()->m_param1))
                                                    {
                                                        ptr->GetGuiFromList<CIFMacroAlchemySlot>(5024)->m_pMySlot->m_pSlot->CopySlot(slotptr);
                                                        return true;
                                                    }
                                                    else
                                                    {
                                                        g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                        g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,TSM_GETTEXTPTR(L"UIIT_MSG_ENCHANT_EQUIPCLASS_MISMATCH"));
                                                    }
                                                }
                                                else
                                                {
                                                    g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_PUT_A_ITEM_AND_ENHANCER_FIRST"));
                                                    g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,KmtGetText(L"UIIT_KMT_PUT_A_ITEM_AND_ENHANCER_FIRST"));
                                                }
                                            }
                                            else
                                            {
                                                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_MY_PUT_A_MAGIC_PROOF_STONE"));
                                                g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,KmtGetText(L"UIIT_KMT_YOU_CAN_PUT_A_MAGIC_PROOF_STONE"));
                                            }
                                        }
                                    }
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    int aa = reinterpret_cast<int(__thiscall*)(CIFWnd*, int, int, int)>(0x00680fa0)(this, a1, x, y);

    if(this->UniqueID() == 100005)
    {
         CIFMacroMenuAutoSkill* AutoSkillSlot = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot;
        if(this->m_SkillID != 0 && AutoSkillSlot->ActiveTabPage == 2)
        {
            std::n_wstring selectedname = AutoSkillSlot->SelectedPartyMemberName;
            if(!selectedname.empty())
            {
                if(AutoSkillSlot->PartyBuffList.find(selectedname)
                   != AutoSkillSlot->PartyBuffList.end())
                {
                    if(this->N595 != 2) // is aktiftir
                    {
                        std::vector<int>& skillList = AutoSkillSlot->PartyBuffList[selectedname];
                        std::vector<int>::iterator skillIt = std::find(skillList.begin(), skillList.end(), this->m_SkillID);
                        if (skillIt != skillList.end()) {
                            // m_SkillID varsa sil
                            skillList.erase(skillIt);

                        }
                        this->N595 = 2;
                    }
                    else if(this->N595 == 2) /// is eklenmemiştir
                    {
                        std::vector<int>& skillList = AutoSkillSlot->PartyBuffList[selectedname];
                        std::vector<int>::iterator skillIt = std::find(skillList.begin(), skillList.end(), this->m_SkillID);
                        if (skillIt == skillList.end()) {
                            // m_SkillID varsa sil
                            skillList.push_back(this->m_SkillID);
                        }


                        this->N595 = 0;
                    }
                }
            }

        }
    }
    else if(this->UniqueID() == GDR_SKILL_SLOT_UNIQUE_1)
    {
        if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_DEACTIVE) {

            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_1 == 1 &&
                            data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_1)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_1 = 1;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)1; /// SKILL SLOT
                                buf << (byte)1;
                                SendMsg(buf);
                                if (!m_Player->PetSkillTimerRunning) {
                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 3000);
                                    m_Player->PetSkillTimerRunning = true;
                                }
                                this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }
                    }
                }
            }
        }
        else if(this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE)
        {
            this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_1 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_1)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_1 = 0;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)1; /// SKILL SLOT
                                buf << (byte)0;
                                SendMsg(buf);

                            }
                        }
                    }
                }
            }
        }
    }
    else if(this->UniqueID() == GDR_SKILL_SLOT_UNIQUE_2)
    {
        if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_DEACTIVE) {

            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_2 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_2)
                            {

                                m_Player->m_FellowSkillData[0].Enable_Skill_2 = 1;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)2; /// SKILL SLOT
                                buf << (byte)1;
                                SendMsg(buf);
                                if (!m_Player->PetSkillTimerRunning) {
                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 3000);
                                    m_Player->PetSkillTimerRunning = true;
                                }

                                this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }
                    }
                }
            }
        }
        else if(this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE)
        {
            this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_2 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_2)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_2 = 0;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)2; /// SKILL SLOT
                                buf << (byte)0;
                                SendMsg(buf);

                            }
                        }
                    }
                }
            }
        }
    }
    else if(this->UniqueID() == GDR_SKILL_SLOT_UNIQUE_3)
    {
        if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_DEACTIVE) {

            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_3 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_3)
                            {

                                m_Player->m_FellowSkillData[0].Enable_Skill_3 = 1;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)3; /// SKILL SLOT
                                buf << (byte)1;
                                SendMsg(buf);
                                if (!m_Player->PetSkillTimerRunning) {
                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 3000);
                                    m_Player->PetSkillTimerRunning = true;
                                }

                                this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }
                    }
                }
            }
        }
        else if(this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE)
        {
            this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_3 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_3)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_3 = 0;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)3; /// SKILL SLOT
                                buf << (byte)0;
                                SendMsg(buf);

                            }
                        }
                    }
                }
            }
        }
    }
    else if(this->UniqueID() == GDR_SKILL_SLOT_UNIQUE_4)
    {
        if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_DEACTIVE) {

            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_4 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_4)
                            {

                                m_Player->m_FellowSkillData[0].Enable_Skill_4 = 1;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)4; /// SKILL SLOT
                                buf << (byte)1;
                                SendMsg(buf);
                                if (!m_Player->PetSkillTimerRunning) {
                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 3000);
                                    m_Player->PetSkillTimerRunning = true;
                                }

                                this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }
                    }
                }
            }
        }
        else if(this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE)
        {
            this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_4 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_4)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_4 = 0;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)4; /// SKILL SLOT
                                buf << (byte)0;
                                SendMsg(buf);

                            }
                        }
                    }
                }
            }
        }
    }
    else if(this->UniqueID() == GDR_SKILL_SLOT_UNIQUE_5)
    {
        if (this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_DEACTIVE) {

            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_5 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_5)
                            {

                                m_Player->m_FellowSkillData[0].Enable_Skill_5 = 1;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)5; /// SKILL SLOT
                                buf << (byte)1;
                                SendMsg(buf);
                                if (!m_Player->PetSkillTimerRunning) {
                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 3000);
                                    m_Player->PetSkillTimerRunning = true;
                                }

                                this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_ACTIVE;
                            }
                        }
                    }
                }
            }
        }
        else if(this->SlotisLocked == GDR_FELLOW_SKILL_SLOT_ACTIVE)
        {
            this->SlotisLocked = GDR_FELLOW_SKILL_SLOT_DEACTIVE;
            for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
            {
                CICharactor *pUser = GetCharacterObjectByID_MAYBE(it->first);
                if (pUser != NULL) {
                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if(data)
                    {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;
                        if(m_CustomDataManager->m_RefFellowPetSystem.find(NameStr) != m_CustomDataManager->m_RefFellowPetSystem.end())
                        {
                            if(m_Player->m_FellowSkillData.size() > 0 && m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_5 == 1&&
                                                                         data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_5)
                            {
                                m_Player->m_FellowSkillData[0].Enable_Skill_5 = 0;
                                CMsgStreamBuffer buf(0x189A);
                                buf << m_Player->m_FellowSkillData[0].ID64;
                                buf << NameStr;
                                buf << (byte)data->GetData().Level;
                                buf << (byte)5; /// SKILL SLOT
                                buf << (byte)0;
                                SendMsg(buf);

                            }
                        }
                    }
                }
            }
        }
    }
    return aa;
}
void CIFSlotWithHelp::sub_686DB0() const {
    reinterpret_cast<void(__thiscall*)(const CIFSlotWithHelp*)>(0x00686DB0)(this);
}

int CIFSlotWithHelp::OnMouseLeftDownIMPL(int a1, int x, int y){

        int aa = reinterpret_cast<int(__thiscall*)(CIFWnd*, int, int, int)>(0x0046fd60)(this, a1, x, y);
    CIFSlotWithHelp *slotptr = (CIFSlotWithHelp *) g_CurrentIfUnderCursor;
    //printf("%p \n", slotptr);
    //printf("current if under cursor = %02x\n", slotptr);
    short ALTState = GetKeyState(VK_CONTROL);
    short MouseState = GetKeyState(VK_LBUTTON);
    if (slotptr && (ALTState == -127 || ALTState == -128) && (MouseState == -127 || MouseState == -128)) {
        if (*(DWORD *) slotptr == 0xDA18E4 || *(DWORD *) slotptr == 0xD9E0FC) {
            /*   if (*(DWORD*)slotptr == 0xD9E0FC) {
                 CGWndBase* shit;
                 if (shit) {
                     slotptr = reinterpret_cast<CIFSlotWithHelp*>(shit->m_parentControl);
                 }
             }*/

            int slotId = (slotptr->UniqueID() - 87);
            //printf("%d\n", slotId);
            if (slotptr->Type == 70 && (slotId >= 13 && slotId <= 108)) {
                if (slotptr->ItemInfo) {


                    CIFWholeChat *pWholeChat = (CIFWholeChat *) g_pCGInterface->m_IRM.GetResObj(56, 0);
                    CIFChatViewer *pChatViewer = (CIFChatViewer *) g_pCGInterface->m_IRM.GetResObj(1, 0);
                    if (pWholeChat) {
                        CIFEdit *pCIFEdit;
                        pCIFEdit = pWholeChat->m_pEdit;
                        if (pCIFEdit) {
                            if (g_global->m_LinkedGlobalItemEnd > 0) {

                                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_ALLOWED_ONLY_ONE_ITEM_FOR_LINK"));
                                return 1;
                            }
                            std::wstring currStr = std::wstring(pCIFEdit->GetText());
                            currStr.append(L" ");
                            g_global->m_LinkedGlobalItemBegin = currStr.length();

                            std::wstring ItemNames = L"";

                            const SItemData *data = &g_CGlobalDataManager->GetItemData(slotptr->ItemInfo->m_refObjItemId);
                            ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();

                            std::wstring Item = L"<" + ItemNames + L">";
                            std::wstring underline = L"\u035F";
                            std::wstring ItemName;
                            for (size_t i = 0; i < Item.size(); i++) {
                                std::wstring bkp = Item.substr(i, 1);
                                ItemName.append(underline);
                                ItemName.append(bkp);
                            }
                            pCIFEdit->SetText(std::wstring(currStr).append(ItemName.append(L" ")).c_str());

                            g_global->m_LinkedGlobalItemEnd = std::wstring(pCIFEdit->GetText()).length();
                            g_global->m_CurrentLinkedItemName = ItemName;
                            g_global->m_LinkedGlobalSlot = slotId;
                        }
                    }
                    #if 0
                    else if (pChatViewer) {
                        CIFEdit *pCIFEdit;
                        pCIFEdit = pChatViewer->m_InputBox;
                        if (pCIFEdit) {
                            if (g_global->m_LinkedGlobalItemEnd > 0) {

                                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_ALLOWED_ONLY_ONE_ITEM_FOR_LINK"));
                                return 1;
                            }
                            std::wstring currStr = std::wstring(pCIFEdit->GetText());
                            currStr.append(L" ");
                            g_global->m_LinkedGlobalItemBegin = currStr.length();

                            std::wstring ItemNames = L"";

                            const SItemData *data = &g_CGlobalDataManager->GetItemData(slotptr->ItemInfo->m_refObjItemId);
                            ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();

                            std::wstring Item = L"<" + ItemNames + L">";
                            std::wstring underline = L"\u035F";
                            std::wstring ItemName;
                            for (size_t i = 0; i < Item.size(); i++) {
                                std::wstring bkp = Item.substr(i, 1);
                                ItemName.append(underline);
                                ItemName.append(bkp);
                            }
                            pCIFEdit->SetText(std::wstring(currStr).append(ItemName.append(L" ")).c_str());

                            g_global->m_LinkedGlobalItemEnd = std::wstring(pCIFEdit->GetText()).length();
                            g_global->m_CurrentLinkedItemName = ItemName;
                            g_global->m_LinkedGlobalSlot = slotId;

                        }
                    }
                    #endif
                }
            }
        }
    }
    return aa;
}

void CIFSlotWithHelp::AppendAdvancedInfo(CIFHelperBubbleWindow *window) {
    /*  CSOItem* myitem = this->ItemInfo;
      if(myitem->GetItemData()->m_itemClass < 13)
      {*/

    reinterpret_cast<void(__thiscall *)(CIFSlotWithHelp *, CIFHelperBubbleWindow *)>(0x0066FD60)(this, window);

    //   }

    /*0: EQUIP_SLOT_HELM
    1: EQUIP_SLOT_MAIL,
    2: EQUIP_SLOT_SHOULDERGUARD,
    3: EQUIP_SLOT_GAUNTLET,
    4: EQUIP_SLOT_PANTS,
    5: EQUIP_SLOT_BOOTS,
    6: EQUIP_SLOT_WEAPON,
    7: EQUIP_SLOT_SHIELD or ARROW,
    9: EQUIP_SLOT_EARRING,
    10: EQUIP_SLOT_NECKLACE,
    11: EQUIP_SLOT_L_RING,
    12: EQUIP_SLOT_R_RING,*/
    short ALTState = GetKeyState(VK_MENU);
    if (ALTState == -127 || ALTState == -128)// ALT is hold
    {
        wchar_t buffer[45];
        std::wstring mymsg(L"\n       <<Comparison>>");
        UINT msgcolor = 0xFFEFDAA4;//No Difference: FFFEFEFF | Better: FFFFD953 | Worse: FFFF4A4A
        window->Write(mymsg, 0, msgcolor, 0, 3);
        CSOItem *myitem = this->ItemInfo;
        CSOItem *curritem;
        switch (myitem->GetObjectData()->m_typeId.m_type_id_value) {
            case 0x132C://ch sword
            case 0x1B2C://ch blade
            case 0x232C://ch spear
            case 0x2B2C://ch glaive
            case 0x332C://ch bow
            case 0x6B2C://eu dagger
            case 0x3B2C://eu 1h
            case 0x432C://eu 2h
            case 0x4B2C://eu axe
            case 0x632C://eu crossbow
            case 0x532C://eu warlock
            case 0x5B2C://eu 2hstaff
            case 0x732C://eu harp
            case 0x7B2C://eu staff
            {
                int dmi = 0, dma = 0;
                float drmi = 0, drma = 0;
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(6);
                if (myitem->m_PhyAtkPwrMax || curritem->m_PhyAtkPwrMax) {
                    dmi = myitem->m_PhyAtkPwrMin - curritem->m_PhyAtkPwrMin;
                    dma = myitem->m_PhyAtkPwrMax - curritem->m_PhyAtkPwrMax;

                    if (dmi == 0 && dma == 0) {
                        msgcolor = 0xFFFEFEFF;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_PLUS_0_PLUS_0"));
                    } else if (dmi > -1 && dma > -1) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_PLUS_VALUE_PLUS_VALUE"), dmi, dma);
                    } else if (dmi < 0 && dma < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_VALUE_VALUE"), dmi, dma);
                    } else if (dmi > 0 && dma < 0 && (dmi - dma) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_PLUS_VALUE_VALUE"), dmi, dma);
                    } else if (dma > 0 && dmi < 0 && (dma - dmi) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_VALUE_PLUS_VALUE"), dmi, dma);
                    } else if (dmi > 0 && dma < 0 && (dmi - dma) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_PLUS_VALUE_VALUE"), dmi, dma);
                    } else if (dma > 0 && dmi < 0 && (dma - dmi) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ATK_PWR_VALUE_PLUS_VALUE"), dmi, dma);
                    }

                    mymsg.assign(buffer);
                    window->Write(mymsg, 0, msgcolor, 0, 1);
                }

                if (myitem->m_MagAtkPwrMax || curritem->m_MagAtkPwrMax) {
                    dmi = myitem->m_MagAtkPwrMin - curritem->m_MagAtkPwrMin;
                    dma = myitem->m_MagAtkPwrMax - curritem->m_MagAtkPwrMax;

                    if (dmi == 0 && dma == 0) {
                        msgcolor = 0xFFFEFEFF;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_PLUS_0_PLUS_0"));
                    } else if (dmi > -1 && dma > -1) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_PLUS_VALUE_PLUS_VALUE"), dmi, dma);
                    } else if (dmi < 0 && dma < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_VALUE_VALUE"), dmi, dma);
                    } else if (dmi > 0 && dma < 0 && (dmi - dma) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_PLUS_VALUE_VALUE"), dmi, dma);
                    } else if (dma > 0 && dmi < 0 && (dma - dmi) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_VALUE_PLUS_VALUE"), dmi, dma);
                    } else if (dmi > 0 && dma < 0 && (dmi - dma) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_PLUS_VALUE_VALUE"), dmi, dma);
                    } else if (dma > 0 && dmi < 0 && (dma - dmi) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ATK_PWR_VALUE_PLUS_VALUE"), dmi, dma);
                    }

                    mymsg.assign(buffer);
                    window->Write(mymsg, 0, msgcolor, 0, 1);
                }

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_AttackRateValue - curritem->m_AttackRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_ATTACK_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_ATTACK_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_ATTACK_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_CriticalValue - curritem->m_CriticalValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_CRITICAL_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_CRITICAL_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_CRITICAL_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                if (myitem->m_PhyReinforcementMax || curritem->m_PhyReinforcementMax) {
                    drmi = myitem->m_PhyReinforcementMin - curritem->m_PhyReinforcementMin;
                    drma = myitem->m_PhyReinforcementMax - curritem->m_PhyReinforcementMax;

                    if (drmi == 0 && drma == 0) {
                        msgcolor = 0xFFFEFEFF;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT_PLUS_0_PERCENT"));
                    } else if (drmi > -1 && drma > -1) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi < 0 && drma < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi > 0 && drma < 0 && (drmi - drma) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drma > 0 && drmi < 0 && (drma - drmi) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi > 0 && drma < 0 && (drmi - drma) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drma > 0 && drmi < 0 && (drma - drmi) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    }

                    mymsg.assign(buffer);
                    window->Write(mymsg, 0, msgcolor, 0, 1);
                }

                if (myitem->m_MagReinforcementMax || curritem->m_MagReinforcementMax) {
                    drmi = myitem->m_MagReinforcementMin - curritem->m_MagReinforcementMin;
                    drma = myitem->m_MagReinforcementMax - curritem->m_MagReinforcementMax;

                    if (drmi == 0 && drma == 0) {
                        msgcolor = 0xFFFEFEFF;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT_PLUS_0_PERCENT"));
                    } else if (drmi > -1 && drma > -1) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi < 0 && drma < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi > 0 && drma < 0 && (drmi - drma) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drma > 0 && drmi < 0 && (drma - drmi) > 0) {
                        msgcolor = 0xFFFFD953;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drmi > 0 && drma < 0 && (drmi - drma) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT_1F_PERCENT"), drmi * 100, drma * 100);
                    } else if (drma > 0 && drmi < 0 && (drma - drmi) < 0) {
                        msgcolor = 0xFFFF4A4A;
                        swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT_PLUS_1F_PERCENT"), drmi * 100, drma * 100);
                    }

                    mymsg.assign(buffer);
                    window->Write(mymsg, 0, msgcolor, 0, 1);
                }
                break;
            }

            case 0x0A2C://ch shield
            case 0x122C://eu shield
            {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(7);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_BlockingRateValue - curritem->m_BlockingRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_BLOCKING_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_BLOCKING_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_BLOCKING_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

            case 0x0AAC://ch earring
            case 0x0E2C://eu earring
            {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(9);
                float drmi = 0;
                drmi = myitem->m_PhyAbsorption - curritem->m_PhyAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagAbsorption - curritem->m_MagAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

            case 0x12AC://ch necklace
            case 0x162C://eu necklace
            {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(10);
                float drmi = 0;
                drmi = myitem->m_PhyAbsorption - curritem->m_PhyAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagAbsorption - curritem->m_MagAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

            case 0x1AAC://ch ring
            case 0x1E2C://eu ring
            {
                CIFEquipment *myequipments = g_pCGInterface->GetMainPopup()->GetEquipment();
                curritem = myequipments->GetEquipmentObjectBySlot(11);

                float drmi = 0;
        mymsg.assign(KmtGetText(L"UIIT_KMT_LEFT"));
                msgcolor = 0xFFEFDAA4;
                window->Write(mymsg, 0, msgcolor, 0, 2);

                drmi = myitem->m_PhyAbsorption - curritem->m_PhyAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagAbsorption - curritem->m_MagAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                curritem = myequipments->GetEquipmentObjectBySlot(12);

                mymsg.assign(L"\n<<Right>>");
                msgcolor = 0xFFEFDAA4;
                window->Write(mymsg, 0, msgcolor, 0, 2);

                drmi = myitem->m_PhyAbsorption - curritem->m_PhyAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagAbsorption - curritem->m_MagAbsorption;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_ABSORPTION_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Head
            case 0x0D2C:
            case 0x092C:
            case 0x0CAC:
            case 0x0DAC:
            case 0x08AC:
            case 0x09AC: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(0);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Shoulder
            case 0x15AC:
            case 0x10AC:
            case 0x152C:
            case 0x11AC:
            case 0x112C:
            case 0x14AC: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(2);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Chest
            case 0x1CAC:
            case 0x192C:
            case 0x18AC:
            case 0x1DAC:
            case 0x19AC:
            case 0x1D2C: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(1);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Legs
            case 0x25AC:
            case 0x21AC:
            case 0x252C:
            case 0x212C:
            case 0x24AC:
            case 0x20AC: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(4);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Hands
            case 0x2CAC:
            case 0x292C:
            case 0x28AC:
            case 0x2DAC:
            case 0x2D2C:
            case 0x29AC: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(3);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }

                //Foot
            case 0x30AC:
            case 0x312C:
            case 0x31AC:
            case 0x34AC:
            case 0x352C:
            case 0x35AC: {
                curritem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(5);
                int dmi = 0;
                float drmi = 0;
                drmi = myitem->m_PhyDefPwrValue - curritem->m_PhyDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagDefPwrValue - curritem->m_MagDefPwrValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_1F"), drmi);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_1F"), drmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_DEF_PWR_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_MaxDurability - curritem->m_MaxDurability;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_DURABILITY_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                dmi = myitem->m_ParryRateValue - curritem->m_ParryRateValue;
                if (dmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_VALUE"), dmi);
                } else if (dmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_VALUE"), dmi);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PARRY_RATE_PLUS_0"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_PhyReinforcementValue - curritem->m_PhyReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_PHY_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);

                drmi = myitem->m_MagReinforcementValue - curritem->m_MagReinforcementValue;
                if (drmi > 0) {
                    msgcolor = 0xFFFFD953;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_1F_PERCENT"), drmi * 100);
                } else if (drmi < 0) {
                    msgcolor = 0xFFFF4A4A;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_1F_PERCENT"), drmi * 100);
                } else {
                    msgcolor = 0xFFFEFEFF;
                    swprintf_s(buffer, sizeof(buffer), KmtGetText(L"UIIT_KMT_MAG_REINFORCE_PLUS_0_PERCENT"));
                }
                mymsg.assign(buffer);
                window->Write(mymsg, 0, msgcolor, 0, 1);
                break;
            }
        }
    }
}

bool CIFSlotWithHelp::Func_28IMPL(CGWnd* a1, int a2, int a3) {
    if(this->GetSlot() == 700)
    {

        CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
        int SlotType = birakilanslot->GetParentWindowId();
        switch (SlotType) {
            case 0x46:
            {
                if(this->GetSlot() == 700)
                {
                    if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                    {
                        CopySlot(birakilanslot);
                        //m_pSlot->SetSlotData(birakilanslot->ItemInfo);
                    }

                }

            } break;
            default:
                break;
        }
        return true;
    }
    else
    {
        return reinterpret_cast<bool(__thiscall *)(CIFSlotWithHelp *, CGWnd*, int, int)>(0x00666560)(this, a1, a2, a3);
    }

}
