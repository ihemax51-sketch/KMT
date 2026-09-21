//
// Created by YUMBUL on 13.10.2023.
//

#include "IFMacroAlchemySlot.h"
#include "IFAlchemyMacro.h"
#include <GInterface.h>
#include <GlobalDataManager.h>
#include <TextStringManager.h>
#include <SRIFLib/NIFEnchantWnd.h>

GFX_IMPLEMENT_DYNCREATE(CIFMacroAlchemySlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFMacroAlchemySlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFMacroAlchemySlot::CIFMacroAlchemySlot(void)
    : m_pMySlot(NULL), copyslot(NULL), m_nMagicCubeSlotGID(0) {
}

CIFMacroAlchemySlot::~CIFMacroAlchemySlot(void) {

}

bool CIFMacroAlchemySlot::OnCreate(long ln) {
    CIFWnd::OnCreate(ln);

  //  m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacroslot.txt");
    //m_IRM.CreateInterfaceSection("Create", this);
    RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    m_pMySlot = (CIFSlotWithHelpEx*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelpEx), m_pSlotRect, this->UniqueID(), 0);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);

  /*  m_pMySlot = m_IRM.GetResObj<CIFSlotWithHelpEx>(1, 0);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
*/
    m_pMySlot->MoveGWnd(this->GetPos().x, this->GetPos().y);
    m_pMySlot->m_pSlot->BringToFront();
    this->ShowGWnd(true);
    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFMacroAlchemySlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
}

void CIFMacroAlchemySlot::OnUpdate() {
    if(m_pMySlot != NULL)
    {
        if(m_pMySlot->m_pSlot != NULL)
        {
            m_pMySlot->m_pSlot->ShowGWnd(true);
            m_pMySlot->m_pSlot->BringToFront();
        }
    }
}
void CIFMacroAlchemySlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);
    m_pMySlot->m_pSlot->SetInventorySlotType(Data);
}


void CIFMacroAlchemySlot::UseItem(int Slot) {
    if(this->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        CIFMainPopup *popup = g_pCGInterface->GetMainPopup();
        CIFInventory *inventory = popup->GetInventory();

        CSOItem *item = inventory->GetItemBySlot(Slot);

        if (item->m_blValid == 0) {
            return;
        }

        const SItemData *pItemData = item->GetItemData();
        NEWMSG(0x704C)
            pReq << (BYTE)(Slot+13) << (UINT16)(pItemData->m_typeId.m_type_id_value);
        SENDMSG()
    }
}
void CIFMacroAlchemySlot::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFMacroAlchemySlot::Func_26(int a1) {
    //printf("%p \n", a1);
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
    if (pWnd != (CIFSlotWithHelp*) 0x0) {
        CIFAlchemyMacro *macro = NULL;
        if (this->m_pMySlot->m_pSlot->GetSlot() == 500) {
            macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
            if (macro != NULL) {
                macro->OnTrackedItemRemoved();
            }
        }
        pWnd->ClearSlot();
        if(this->m_pMySlot->m_pSlot->GetSlot() == 500)
        {
            macro->m_BlueList.clear();
            macro->m_AttrList.clear();
            macro->m_TargetAttrList.clear();
            macro->ClearDDJList();
            macro->ClearList();
            macro->ClearDDJListAttr();
        }
        else if (this->m_pMySlot->m_pSlot->GetSlot() == 501)
        {
            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->HideEnchancerAndSpoofSlot();
            }
        }

     //   SaveSlotData(pWnd);
    }

}

//bool CIFMySlot::Func_25(int a1) {
//    //printf("%p \n", a1);
//    //CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
//    //if (pWnd == (CIFSlotWithHelp *) 0x0) {
//    //    ClearSlot();
//    //}
//    return false;
//}


bool CIFMacroAlchemySlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    if (birakilanslot == NULL || birakilanslot->ItemInfo == NULL || birakilanslot->ItemInfo->GetItemData() == NULL) {
        return false;
    }
    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 0x46:
        {
            if(this->m_pMySlot->m_pSlot->GetSlot() == 500)
            {
                if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                   && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                   && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    CIFAlchemyMacro * macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
                    if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 6) /// IS THIS WEAPON
                    {
                       macro->WeaponAddedtoSlot();
                    }
                    else if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 4)
                    {
                        macro->ShieldAddedtoSlot();
                    }
                    else if((birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 1 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 2
                            || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 3  || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 9
                            || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() ==  10                                                                                                          || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 11)

                    && (birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 1 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 3
                            || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() == 4))
                    {
                        macro->HeadChestLegsAddedtoSlot();
                    }
                    else if((birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 1 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 2
                             || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 3  || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 9
                             || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() ==  10                                                                                                          || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 11)

                             && (birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 1 && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 3
                                 && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 4))
                    {
                        macro->ShoulderHandsFootAddedtoSlot();
                    }
                    else if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 5 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 12)
                    {
                        macro->AccAddedtoSlot();
                    }
                }

            }
            else if(this->m_pMySlot->m_pSlot->GetSlot() == 501)
            {
                if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                   && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                   && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                    {
                    if(birakilanslot->ItemInfo->GetItemData()->Rarity != 0)
                    {
                        m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                        {
                            g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->ShowEnchancerAndSpoofSlot();
                        }
                    }
                    else
                    {
                        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THIS_PLACE_FOR_A_SOX_ITEM"));
                        g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,KmtGetText(L"UIIT_KMT_THIS_PLACE_FOR_A_SOX_ITEM"));
                    }
                    }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 502)
            {
                if(birakilanslot->ItemInfo->GetItemData()->IsEnhancer())
                {
                    if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                    {
                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() == WEAPON)
                        {
                            if(birakilanslot->ItemInfo->GetItemData()->IsEnhancer() && birakilanslot->ItemInfo->GetItemData()->m_param3 == 100663296)
                            {
                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(birakilanslot->ItemInfo->GetItemData()->m_param1))
                                {
                                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
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
                            if(birakilanslot->ItemInfo->GetItemData()->IsEnhancer() && birakilanslot->ItemInfo->GetItemData()->m_param3 == 16909056)
                            {
                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(birakilanslot->ItemInfo->GetItemData()->m_param1))
                                {
                                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
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
                            if(birakilanslot->ItemInfo->GetItemData()->IsEnhancer() && birakilanslot->ItemInfo->GetItemData()->m_param3 == 83886080)
                            {
                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(birakilanslot->ItemInfo->GetItemData()->m_param1))
                                {
                                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
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
                            if(birakilanslot->ItemInfo->GetItemData()->IsEnhancer() && birakilanslot->ItemInfo->GetItemData()->m_param3 == 67108864)
                            {
                                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeCondition(birakilanslot->ItemInfo->GetItemData()->m_param1))
                                {
                                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->SetStartButtonState(true);
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
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 503)
            {
                if(birakilanslot->ItemInfo->GetItemData()->IsProofStone())
                    {
                        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                        {
                           if(birakilanslot->ItemInfo->GetItemData()->IsProofStone())
                           {
                               if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetItemInfoFromSlot() != 0 && g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetEnhancerInfoFromSlot())
                               {
                                   if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->GetDegreeConditionForProofStone(birakilanslot->ItemInfo->GetItemData()->m_param1))
                                   {
                                       m_pMySlot->m_pSlot->CopySlot(birakilanslot);
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
        } break;
        default:
            break;
    }
    return true;
}
void CIFMacroAlchemySlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFMacroAlchemySlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}
