// Credits: @florian0 https://www.elitepvpers.com/forum/sro-pserver-guides-releases/4256375-source-fix-old-exp-bar-writing-code.html

#include "NIFUnderMenuBar.h"

#include "ICPlayer.h"

#include "BSLib/Debug.h"
#include "Data\LevelData.h"
#include "GInterface.h"
#include "GlobalDataManager.h"
#include <stdio.h>

#include <cmath>
#include <SecondBar/IFExtQuickSlotSecond.h>
#include <SecondBar/IFExtQuickSlotCustom.h>
#include <Macro/IFMacroMenu.h>
#include <CustomData/CustomSettingManager.h>
#include <GEffSoundBody.h>
#include <IFPlayerMiniInfo.h>

namespace {
double CalculateSafeJobExperiencePercentage(int currentExperience, int requiredExperience) {
    if (currentExperience <= 0 || requiredExperience <= 0) {
        return 0.0;
    }

    double percentage = static_cast<double>(currentExperience) * 100.0 /
                        static_cast<double>(requiredExperience);
    if (percentage < 0.0) {
        return 0.0;
    }
    if (percentage > 99.99) {
        return 99.99;
    }

    return percentage;
}

void UpdateJobExperienceGauges(CNIFGauge *gauges[10], double percentage) {
    int barNumber = static_cast<int>(floor(percentage / 10.0));
    if (barNumber < 0) {
        barNumber = 0;
    } else if (barNumber > 9) {
        barNumber = 9;
    }

    for (int i = 0; i < 10; ++i) {
        if (!gauges[i]) {
            continue;
        }

        const float value = barNumber > i ? 1.0f : 0.0f;
        gauges[i]->background_value = value;
        gauges[i]->foreground_value = value;
    }

    CNIFGauge *partialGauge = gauges[barNumber];
    if (!partialGauge) {
        return;
    }

    const float remainingPercentage = static_cast<float>(
        (percentage - (barNumber * 10.0)) / 10.0);
    partialGauge->background_value = remainingPercentage;
    partialGauge->foreground_value = remainingPercentage;
}
}

void CNIFUnderMenuBar::GetSlotUseItem(undefined4 Slots) {
    if (Slots >= 51 && Slots < 101) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[Slots-51]->UseItem(Slots);
        return;
    }
    if (Slots >= 101 && Slots < 141) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[Slots-101]->UseItem(Slots);
        return;
    }
    if(Slots >= 141)
    {
        return;
    }
    reinterpret_cast<void(__thiscall *)(CNIFUnderMenuBar *, undefined4)>(0x0060bb80)(this, Slots);
}

void CNIFUnderMenuBar::Fun_0060a940(int byte)
{

    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar*, int)>(0x0060a940)(this, byte);
    if(m_Settings->SecondarySlot)
    {
        if (!g_pMyPlayerObj) {
            return;
        }
        for (int i = 0; i < 50; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70
                &&g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }
        }
        for (int i = 0; i < 40; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }
        }
    }


    if(m_Settings->EnableMacro)
    {
        if (!g_pMyPlayerObj) {
            return;
        for (int i = 0; i < 12; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }
        }
        /// UPDATE WEAPONS
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }
            ///SHIELD
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }

            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == 70
                && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
            {
                int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetSlotVisual(news);
            }

            for (int i = 0; i < 8; i++) {

                if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetSlotType() == 70
                    && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == byte)
                {
                    int news = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ItemInfo->ActiveItemTime;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetSlotVisual(news);
                }
            }

    }
}

}
bool CNIFUnderMenuBar::Func_28_IMPL(int a1, int a2, int a3) {
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    int SlotType = birakilanslot->GetParentWindowId();
    if(birakilanslot->UniqueID() == 10007 || birakilanslot->UniqueID() == 10006 || birakilanslot->UniqueID() == 100005)
    {
        return true;
    }
    return reinterpret_cast<bool(__thiscall*)(CNIFUnderMenuBar*, int, int, int)>(0x0060e950)(this, a1, a2, a3);

}
void CNIFUnderMenuBar::SaveQuickslotData(int byIndexSot) {
    if (byIndexSot >= 51 && byIndexSot <= 100) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[byIndexSot - 50]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[byIndexSot - 50]->m_pMySlot->m_pSlot);
        return;
    }
    if (byIndexSot >= 101 && byIndexSot <= 140) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[byIndexSot - 100]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[byIndexSot - 100]->m_pMySlot->m_pSlot);
        return;
    }
    if(byIndexSot >= 101)
    {
        return;
    }
    //printf("Index %d \n", byIndexSot);
    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar*, int)>(0x0060bea0)(this, byIndexSot);
}
void CNIFUnderMenuBar::UpdateSkillSlot(unsigned int p1, unsigned int p2)
{
    if (!g_pMyPlayerObj) {
        return;
    }
    CSkillData* newsdata = g_CGlobalDataManager->GetSkillData(p1);
    if(m_Settings->SecondarySlot)
    {
        for (int i = 0; i < 50; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 73 || g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 25)
            {
                int SkillIndex =g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
                CSkillData* oldsdata = g_CGlobalDataManager->GetSkillData(SkillIndex);

                if (newsdata->ID == oldsdata->ID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot);

                }
                else if (newsdata->GroupID == oldsdata->GroupID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSkillInfo(p1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot);

                }
            }

        }
        for (int i = 0; i < 40; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 73 || g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 25)
            {
                int SkillIndex = g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
                CSkillData* oldsdata = g_CGlobalDataManager->GetSkillData(SkillIndex);

                if (newsdata->ID == oldsdata->ID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot);

                }
                else if (newsdata->GroupID == oldsdata->GroupID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSkillInfo(p1);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot);

                }
            }

        }
    }
    if(m_Settings->EnableMacro)
    {
        for(int i = 0; i < 24; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 73 || g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 25)
            {
                int SkillIndex =g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
                CSkillData* oldsdata = g_CGlobalDataManager->GetSkillData(SkillIndex);

                if (newsdata->ID == oldsdata->ID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot);

                }
                else if (newsdata->GroupID == oldsdata->GroupID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot->GetSkillInfo(p1);
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[i]->m_pMySlot->m_pSlot);

                }
            }
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 73 || g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 25)
            {
                int SkillIndex =g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->GetSkillSlotInDex();
                CSkillData* oldsdata = g_CGlobalDataManager->GetSkillData(SkillIndex);

                if (newsdata->ID == oldsdata->ID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot);

                }
                else if (newsdata->GroupID == oldsdata->GroupID)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot->GetSkillInfo(p1);
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[i]->m_pMySlot->m_pSlot);

                }
            }
        }
    }



    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar*, unsigned int, unsigned int)>(0x0060c230)(this, p1, p2);

}
void CNIFUnderMenuBar::FUN_0060e540(byte OldSlotType, byte OldSlotData, byte NewSlotType, byte NewSlotData, byte p5)
{
    if(m_Settings->SecondarySlot)
    {
        for (int i = 0; i < 50; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
                &&g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

                if (NewSlotType == 0)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                }

                if (NewSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                if (NewSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot);

            }
            else if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                     &&g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

                if (OldSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                if (OldSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot);

            }
        }
        for (int i = 0; i < 40; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
                &&g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

                if (NewSlotType == 0)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                }

                if (NewSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                if (NewSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot);

            }
            else if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                     && g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

                if (OldSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                if (OldSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot);

            }
        }

    }
    if(m_Settings->EnableMacro)
    {
        for(int i = 0; i < 12; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
                &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

                if (NewSlotType == 0)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ClearSlot();
                }

                if (NewSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                if (NewSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot);

            }
            else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                     && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

                if (OldSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                if (OldSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot);
            }
        }
        ///WEAPONS
        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
            &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

            if (NewSlotType == 0)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();
            }

            if (NewSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            if (NewSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot);

        }
        else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                 && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

            if (OldSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            if (OldSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot);
        }

        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
            &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

            if (NewSlotType == 0)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();
            }

            if (NewSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            if (NewSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot);

        }
        else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                 && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

            if (OldSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            if (OldSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot);
        }

        ///SHIELDS


        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
            &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

            if (NewSlotType == 0)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ClearSlot();
            }

            if (NewSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            if (NewSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot);

        }
        else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                 && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

            if (OldSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            if (OldSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot);
        }

        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
            &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

            if (NewSlotType == 0)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ClearSlot();
            }

            if (NewSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            if (NewSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot);

        }
        else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                 && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

            if (OldSlotType == 70)
            {
                CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            if (OldSlotType == 71)
            {
                CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                if (item) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ItemInfo = item;
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                }

            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot);
        }


        for(int i = 0; i < 8; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetSlotType() == OldSlotType
                &&g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == OldSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetSlotType(NewSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetInventorySlotType(NewSlotData);

                if (NewSlotType == 0)
                {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ClearSlot();
                }

                if (NewSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                if (NewSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(NewSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetIconSlot(NewSlotType, NewSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot);

            }
            else if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetSlotType() == NewSlotType
                     && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == NewSlotData)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetSlotType(OldSlotType);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetInventorySlotType(OldSlotData);

                if (OldSlotType == 70)
                {
                    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
                    CSOItem* item = inventory->GetItemBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                if (OldSlotType == 71)
                {
                    CSOItem* item = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(OldSlotData);
                    if (item) {
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ItemInfo = item;
                        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetIconSlot(OldSlotType, OldSlotData);
                    }

                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot);
            }
        }
    }

    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar*, byte,byte,byte,byte, byte)>(0x0060e540)(this,OldSlotType,
                                                                                                  OldSlotData, NewSlotType, NewSlotData, p5);
}

void CNIFUnderMenuBar::LoadSlots(byte SlotSetq, undefined4 SlotType, undefined4 Data) {

    if (SlotSetq >= 51 && SlotSetq <= 100) {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[SlotSetq - 51]->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq >= 101 && SlotSetq <= 140)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(1363, 1)->myslots[SlotSetq - 101]->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq >= 141 && SlotSetq <= 152)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[SlotSetq - 141]->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq == 153)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq == 154)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq == 155)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq == 156)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq >= 157 && SlotSetq <=180)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->skillslots[SlotSetq-157]->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq >= 181 && SlotSetq <=204)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->buffslots[SlotSetq-181]->LoadSlot(SlotSetq, SlotType, Data);
    }
    if(SlotSetq >= 205 && SlotSetq <= 212)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[SlotSetq - 205]->LoadSlot(SlotSetq, SlotType, Data);
    }
    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar*, byte, undefined4, undefined4)>(0x0060aa90)(this, SlotSetq, SlotType, Data);


}
void CNIFUnderMenuBar::FindAndUpdate(int SlotData, int a2, int a3)
{
    if (!g_pMyPlayerObj) {
        return;
    }
    CSOItem *mynewitem;
    int Slot = -1;
    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    for (int is = 0; is < inventory->InventorySlotCount(); is++) {
        CSOItem *newitem = inventory->GetItemBySlot(is);
        if (newitem->m_blValid != 0) {
            if (newitem->m_refObjItemId == a2) {
                Slot = is;
                mynewitem = newitem;
                break;
            }
        }
    }

    if(Slot == -1)
    {
        const CItemData *data = g_CGlobalDataManager->GetItem(a2);
        if(data != NULL)
        {
            if(data->GetData().IsHPPotion() || data->GetData().IsMPPotion() || data->GetData().IsVIGOR() || data->GetData().IsPETHpPotion() ||
               data->GetData().IsPETMpPotion() || data->GetData().IsPill() || data->GetData().IsPuriPill() ||
               data->GetData().IsPetRes() ||data->GetData().IsCureCOS())
            {
                unsigned short Typeid = data->GetData().m_typeId.m_type_id_value;
                for (int is = 0; is < inventory->InventorySlotCount(); is++) {
                    CSOItem *newitem = inventory->GetItemBySlot(is);
                    if (newitem->m_blValid != 0) {
                        if(newitem->GetItemData() != NULL)
                        {
                            if (newitem->GetItemData()->m_typeId.m_type_id_value == Typeid) {
                                Slot = is;
                                mynewitem = newitem;
                                break;
                            }
                        }

                    }
                }
            }

        }
    }
    if(m_Settings->SecondarySlot)
    {
        for (int i = 0; i < 50; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70) {
                if (Slot == -1) {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                } else {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotSecond>(1351, 1)->myslots[i]->m_pMySlot->m_pSlot);
            }
        }

        for (int i = 0; i < 40; i++) {

            if (g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70) {
                if (Slot == -1) {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->ClearSlot();
                } else {
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                    g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
                }
                g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFExtQuickSlotCustom>(CustomQuickSlot, 1)->myslots[i]->m_pMySlot->m_pSlot);
            }
        }
    }
    if(m_Settings->EnableMacro)
    {
        for(int i = 0; i < 11; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->GetSlotType() == 70) {
                if (Slot == -1) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->ClearSlot();
                } else {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MySlots[i]->m_pMySlot->m_pSlot);
            }
        }
        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == 70) {
            if (Slot == -1) {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillWeaponSlot->m_pMySlot->m_pSlot);
        }

        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->GetSlotType() == 70) {
            if (Slot == -1) {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->ClearSlot();
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffWeaponSlot->m_pMySlot->m_pSlot);
        }

        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == 70) {
            if (Slot == -1) {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->ClearSlot();
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->SkillShieldSlot->m_pMySlot->m_pSlot);
        }


        if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->GetSlotType() == 70) {
            if (Slot == -1) {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->ClearSlot();
            } else {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
            }
            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->BuffShieldSlot->m_pMySlot->m_pSlot);
        }

        for(int i = 0; i < 8; i++)
        {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetInventorySlotType() == SlotData && g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->GetSlotType() == 70) {
                if (Slot == -1) {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->ClearSlot();
                } else {
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetSlotData(mynewitem);
                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot->SetInventorySlotType(Slot);
                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->SaveSlotData(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoScrollSlot->Slot1[i]->m_pMySlot->m_pSlot);
            }
        }
    }

    reinterpret_cast<void(__thiscall *)(CNIFUnderMenuBar *, int, int, int)>(0x0060c430)(this, SlotData, a2, a3);
}

void CNIFUnderMenuBar::Update() {
    // Skip, if player object is not loaded yet (KEEP!)
    if(m_Settings->OldExpBar)
    {
        if (!g_pMyPlayerObj) {
            return;
        }

        // Some other call that is important (KEEP!)
        ((void (__thiscall *)(CNIFUnderMenuBar *)) 0x46CD80)(this);

        // Check if control is visible
        if (!((char (__thiscall *)(CNIFUnderMenuBar *)) 0x00B8F530)(this)) {
            return;
        }

        // Retrieve LevelData for current Level
        // (this is one line of Media\server_dep\silkroad\textdata\leveldata.txt)
        CLevelData *data = g_CGlobalDataManager->m_levelDataMap[g_pMyPlayerObj->m_btLevel];

        // Don't continue if level-data is invalid
        if (data == NULL) {
            return;
        }

        // Calculate EXP as percentage value
        const SLevelData &sData = data->GetData();
        double exp_percentage = g_pMyPlayerObj->m_i64CurrentExp * 100.0 / sData.m_expC;

        // Limit maximum percentage to 99.99%
        if (exp_percentage > 99.99) {
            exp_percentage = 99.99;
        }

        // Calculate the number of bars that are full
        int barnum = static_cast<int>(floor(exp_percentage / 10.0));


        for (int i = 0; i < 10; i++) {

            // Fill or empty bars
            if (barnum <= i) {
                gauges[i]->background_value = 0.0;
                gauges[i]->foreground_value = 0.0;
            } else {
                gauges[i]->background_value = 1.0;
                gauges[i]->foreground_value = 1.0;
            }
        }

        // Fill the bar that is neither full or empty with the remaining percentage
        float exp_remain = static_cast<float>((exp_percentage - (barnum * 10.0)) / 10.0);

        gauges[barnum]->background_value = exp_remain;
        gauges[barnum]->foreground_value = exp_remain;

        // Assign more texts
        this->lbl_level->SetTextFormatted(L"Level: %d", g_pMyPlayerObj->m_i64CurrentExp);
        this->lbl_percentage->SetTextFormatted(L"%.2lf %%", exp_percentage);

        // SkillPoints
        this->lbl_spcount->SetTextFormatted(L"%d", g_pMyPlayerObj->m_nSkillPoint);
        this->gauge_skillexp->background_value = g_pMyPlayerObj->m_nSkillPoint_Progress / 400.0f;
        this->gauge_skillexp->foreground_value = g_pMyPlayerObj->m_nSkillPoint_Progress / 400.0f;

        // You can also draw text directly at the gauge. It will be centered automatically
        // this->gauge_skillexp->SetTextFormatted(L"%d", g_CICPlayer->skill_exp);


        this->lbl_exp_bar_scaler->SetText(L""); // Prescaler is disabled

        // This label is right in the middle of the EXP-Bar
        //this->lbl_360->SetText(L"lbl_360");
    }
    else if(m_Settings->ENABLE_NEW_JOB_UI)
    {
        if(g_pMyPlayerObj->GetJobType() != TRIJOB_NOJOB)
        {
            if (!g_pMyPlayerObj) {
                return;
            }

            //// Some other call that is important (KEEP!)
            //((void (__thiscall *)(CNIFUnderMenuBar*)) 0x46CD80)(this);

            //// Check if control is visible0060b440
            //if (!((char (__thiscall *)(CNIFUnderMenuBar*)) 0x00B8F530)(this)) {
            //    return;
            //}

            // Retrieve LevelData for current Level
            // (this is one line of Media\server_dep\silkroad\textdata\leveldata.txt)
            CLevelData *data = g_CGlobalDataManager->m_levelDataMap[g_pMyPlayerObj->GetJobLevel()];

            // Don't continue if level-data is invalid
            if (data == NULL) {
                return;
            }

            // Calculate EXP as percentage value
            const SLevelData &sData = data->GetData();

            if(g_pMyPlayerObj->GetJobType() == TRIJOB_TRADER)
            {
                double exp_percentage = CalculateSafeJobExperiencePercentage(
                    g_pMyPlayerObj->GetCurrentJobExperiencePoints(), sData.m_jobExpTrader);
                this->lbl_percentage->SetTextFormatted(L"%.2lf %%", exp_percentage);
                this->lbl_360->SetTextFormatted(L"%.2lf %%", exp_percentage);

                UpdateJobExperienceGauges(gauges, exp_percentage);

                this->lbl_spcount->NTB_Func_13("clientlibrary\\common\\com_job_hunter02_icon.ddj", 1, 1);
                this->lbl_spcount->SetGWndSize(32,36);
                wnd_pos test = this->GetPos();
                this->lbl_spcount->MoveGWnd(test.x + 10, test.y - 8);

                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->ShowGWnd(true);
                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->TB_Func_13("clientlibrary\\common\\com_job_hunter_icon.ddj", 1, 1);

            }
            else if(g_pMyPlayerObj->GetJobType() == TRIJOB_HUNTER)
            {
                double exp_percentage = CalculateSafeJobExperiencePercentage(
                    g_pMyPlayerObj->GetCurrentJobExperiencePoints(), sData.m_jobExpHunter);
                this->lbl_percentage->SetTextFormatted(L"%.2lf %%", exp_percentage);
                this->lbl_360->SetTextFormatted(L"%.2lf %%", exp_percentage);

                UpdateJobExperienceGauges(gauges, exp_percentage);
///com_job_hunter_icon
/// com_job_teaf_icon
                this->lbl_spcount->NTB_Func_13("clientlibrary\\common\\com_job_hunter02_icon.ddj", 1, 1);
                this->lbl_spcount->SetGWndSize(32,36);
                wnd_pos test = this->GetPos();
                this->lbl_spcount->MoveGWnd(test.x + 10, test.y - 8);


                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->ShowGWnd(true);
                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->TB_Func_13("clientlibrary\\common\\com_job_hunter_icon.ddj", 1, 1);
            }
            else if(g_pMyPlayerObj->GetJobType() == TRIJOB_THIEF)
            {
                double exp_percentage = CalculateSafeJobExperiencePercentage(
                    g_pMyPlayerObj->GetCurrentJobExperiencePoints(), sData.m_jobExpRobber);
                this->lbl_percentage->SetTextFormatted(L"%.2lf %%", exp_percentage);
                this->lbl_360->SetTextFormatted(L"%.2lf %%", exp_percentage);

                UpdateJobExperienceGauges(gauges, exp_percentage);

                this->lbl_spcount->NTB_Func_13("clientlibrary\\common\\com_job_teaf02_icon.ddj", 1, 1);
                this->lbl_spcount->SetGWndSize(32,36);
                wnd_pos test = this->GetPos();
                this->lbl_spcount->MoveGWnd(test.x + 10, test.y - 8);


                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->ShowGWnd(true);
                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->TB_Func_13("clientlibrary\\common\\com_job_teaf_icon.ddj", 1, 1);
            }


            this->GetResObj<CNIFStatic>(11)->SetText(L"Job Mode");




            // Assign more texts
            this->lbl_level->SetTextFormatted(L"Lv. %d", g_pMyPlayerObj->GetJobLevel());
            // Skillpoints
            this->lbl_spcount->SetTextFormatted(L"", 0);



            //  this->lbl_360->SetText(L"Testtt");


            this->gauge_skillexp->SetTextFormatted(L"", 0);
            this->gauge_skillexp->background_value = 400.f / 400.0f;
            this->gauge_skillexp->foreground_value = 400.f / 400.0f;



            //this->m_IRM.GetResObj<CNIFStatic>(11, 0)->SetText(L"", 0);
            // You can also draw text directly at the gauge. It will be centered automatically
            // this->gauge_skillexp->SetText(L"%d", g_CICPlayer->skill_exp);


            this->lbl_exp_bar_scaler->SetText(L""); // Prescaler is disabled


            // This label is right in the middle of the EXP-Bar
            //this->lbl_360->SetText(L"lbl_360");
            CIFPlayerMiniInfo *miniinfo = (CIFPlayerMiniInfo*)g_pCGInterface->m_IRM.GetResObj(11, 1);



            wchar_t buffer1[255];
            if(g_pMyPlayerObj->GetJobType() != TRIJOB_NOJOB)
            {
                miniinfo->NameText->SetText(g_pMyPlayerObj->GetJobAlias().c_str());
                swprintf_s(buffer1, L"Lv %d", g_pMyPlayerObj->GetJobLevel());
                miniinfo->LevelText->SetText(buffer1);

                // underbar->lbl_level->SetText(buffer1);
            }
            else
            {
                miniinfo->NameText->SetText(g_pMyPlayerObj->GetCharName().c_str());
                swprintf_s(buffer1, L"Lv %d", g_pMyPlayerObj->GetCurrentLevel());
                miniinfo->LevelText->SetText(buffer1);
            }
        }
        else
        {
            //  this->LoadResObj<CNIFStatic>(11)->SetText(L"Skill point");
            if(g_pMyPlayerObj->GetJobType() == TRIJOB_NOJOB)
            {
                wchar_t buffer1[255];
                CIFPlayerMiniInfo *miniinfo = (CIFPlayerMiniInfo*)g_pCGInterface->m_IRM.GetResObj(11, 1);
                miniinfo->NameText->SetText(g_pMyPlayerObj->GetCharName().c_str());
                swprintf_s(buffer1, L"Lv %d", g_pMyPlayerObj->GetCurrentLevel());
                miniinfo->LevelText->SetText(buffer1);
                g_pCGInterface->GetGuiFromList<CIFStatic>(JobIconID)->ShowGWnd(false);

                reinterpret_cast<void(__thiscall *)(const CNIFUnderMenuBar *)>(0x0060b440)(this);
            }
        }
    }
    else
    {
        reinterpret_cast<void (__thiscall *)(const CNIFUnderMenuBar*)>(0x0060b440)(this);
    }

}

bool CNIFUnderMenuBar::IsPotionOrPillInQuickslot(int slot) {
    return reinterpret_cast<bool(__thiscall*)(CNIFUnderMenuBar *, int)>(0x0060b1d0)(this, slot);
}
void CNIFUnderMenuBar::PressButtonAutoPotion()
{
    if(m_Settings->EnableMacro)
    {
            if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->IsVisible()) {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(false);
                CGEffSoundBody::get()->PlaySound(L"snd_window_close");
            }
            else {
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ActivateTabPage(0);

                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->ActivateTabPage(0);
                CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        }
        return;
    }

    reinterpret_cast<void(__thiscall*)(CNIFUnderMenuBar *)>(0x0060cd10)(this);
}
void CNIFUnderMenuBar::UseSlot(int slot) {
    CIFInventory *pInventory = g_pCGInterface->GetMainPopup()->GetInventory();
    CIFEquipment *pEquipment = g_pCGInterface->GetMainPopup()->GetEquipment();

    CIFSlotWithHelp *relatedSlot = m_pMySlots[slot]->m_pSlot;

    if (!relatedSlot->GetBGFilename().empty()) {

        switch (relatedSlot->GetItemSourceParentWindowId()) {
            case 70: {
                CIFSlotWithHelp *pSlot = this->m_pMySlots[slot]->m_pSlot;
                if (pSlot->GetItem() == NULL) {
                    return;
                }

                BS_WARNING("CNIFUnderMenuBar::UseSlot(%d) %p", slot, relatedSlot);

                CSOItem *item = pInventory->GetItemBySlot(pSlot->GetInventorySlotIndex());
                if ((item->m_blValid) &&
                    (item->IsSameItemType(&m_pMySlots[slot]->m_pSlot->GetMyItem())) &&
                    (CGWnd::GetDraggedGWnd() == NULL)) {

                    if (item->GetItemData()->IsItemEtc()) {
                        if (!item->GetItemData()->IsAmmo()) {
                            pInventory->GetItemSlotBySlotId(m_pMySlots[slot]->m_pSlot->GetInventorySlotIndex())->UseItem();
                            return;
                        }
                    }

                    if (item->GetItemData()->IsItemCOS()) {
                        pInventory->GetItemSlotBySlotId(m_pMySlots[slot]->m_pSlot->GetInventorySlotIndex())->UseItem();
                        return;
                    }

                    CIFSlotWithHelp *inventorySlot = pInventory->GetItemSlotBySlotId(m_pMySlots[slot]->m_pSlot->GetInventorySlotIndex());
                    if (g_pCGInterface->m_lstReservedItemMallData.empty()) {
                        CGWnd::SetDraggedGWnd(inventorySlot);
                        pEquipment->Func_28(inventorySlot, 0, 0);
                        CGWnd::SetDraggedGWnd(NULL);
                        return;
                    }
                }
            }
                break;

            default:
                reinterpret_cast<void(__thiscall *)(CNIFUnderMenuBar *, int)>(0x0060b850)(this, slot);
                break;
        }

    }
};
