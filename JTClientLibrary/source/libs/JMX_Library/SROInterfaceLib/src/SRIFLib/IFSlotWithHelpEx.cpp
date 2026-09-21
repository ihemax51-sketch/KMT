#include <GInterface.h>
#include <MacroAlchemy/IFAlchemyMacro.h>
#include "IFSlotWithHelpEx.h"
#include "NIFEnchantWnd.h"


bool CIFSlotWithHelpEx::Func_28IMPL(CGWnd* a1, int a2, int a3) {
    if(this->m_pSlot->GetSlot() == 700)
    {
        if (a2 == 0) {
            return true;
        }
        CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
        int SlotType = birakilanslot->GetParentWindowId();
        switch (SlotType) {
            case 0x46:
            {
                if(this->m_pSlot->GetSlot() == 700)
                {
                    if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                    {
                        m_pSlot->CopySlot(birakilanslot);
                        //m_pSlot->SetSlotData(birakilanslot->ItemInfo);

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

                                && (birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 1 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 3
                                    || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4() != 4))
                        {
                            macro->ShoulderHandsFootAddedtoSlot();
                        }
                        else if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 5 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 12)
                        {
                            macro->AccAddedtoSlot();
                        }
                    }

                }
                else if(this->m_pSlot->GetSlot() == 501)
                {
                    if(birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 1
                       && birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() != 13)
                    {
                        if(birakilanslot->ItemInfo->GetItemData()->Rarity != 0)
                        {
                            m_pSlot->CopySlot(birakilanslot);
                            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                            {
                                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->ShowEnchancerAndSpoofSlot();
                            }
                        }
                        else
                        {
                            g_pCGInterface->ShowMessage_Warning(L"This place for a SoX item.");
                            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE,L"This place for a SoX item.");
                        }
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
        return reinterpret_cast<bool(__thiscall *)(CIFSlotWithHelpEx *, CGWnd*, int, int)>(0x00687d80)(this, a1, a2, a3);
    }

}