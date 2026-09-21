//
// Created by YUMBUL on 18.02.2024.
//

#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>
#include "IFCOSManager.h"
#include "IFCOSCommand.h"
#include "GInterface.h"
#include "ICCos.h"
#include <GlobalDataManager.h>
#include <ICPlayer.h>
void CIFCOSManager::OnUpdateIMPL() {
    reinterpret_cast<void *(__thiscall *) (CIFCOSManager *)>(0x00810560)(this);
    for (std::list<CIFCOSStatus *>::iterator it = this->N00000A57.begin();
         it != this->N00000A57.end(); ++it) {
        if ((*it)->m_petUniqueID != 0) {
            CICCos *pUser = (CICCos*)GetCharacterObjectByID_MAYBE((*it)->m_petUniqueID);
            if (pUser != NULL) {
                if (m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end()) {
                    if ((*it)->gauge2->m_valueFg <= 0.30) {
                        (*it)->gauge2->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_bad.ddj", 1, 1);
                    } else if ((*it)->gauge2->m_valueFg > 0.30 && (*it)->gauge2->m_valueFg <= 0.80) {
                        (*it)->gauge2->TB_Func_13("clientlibrary\\fellowpets\\pt_mp.ddj", 1, 1);
                    } else if ((*it)->gauge2->m_valueFg > 0.80) {
                        (*it)->gauge2->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_good.ddj", 1, 1);
                    }

                    static const CCharacterData *data = NULL;
                    data = g_CGlobalDataManager->GetCharacter(pUser->GetCommonData()->RefObjectId);
                    if (data) {
                        std::n_wstring NameStr = pUser->GetCommonData()->NameStrID;

                        if(!m_Player->m_FellowSkillData.empty())
                        {
                            if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_1 > 0)
                            {
                                if(data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_1)
                                {
                                    if(m_Player->m_FellowSkillData[0].Enable_Skill_1 == 1)
                                    {
                                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_1);

                                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_1 == 0)
                                        {
                                            /// PET ICIN
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_1, (DWORD32)pUser)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_1, (DWORD32) g_pMyPlayerObj)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }

                                        }

                                    }
                                }
                            }
                            if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_2 > 0)
                            {
                                if(data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_2)
                                {
                                    if(m_Player->m_FellowSkillData[0].Enable_Skill_2 == 1)
                                    {
                                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_2);

                                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_2 == 0)
                                        {
                                            /// PET ICIN
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_2, (DWORD32)pUser)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_2, (DWORD32) g_pMyPlayerObj)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }

                                        }

                                    }
                                }
                            }
                            if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_3 > 0)
                            {
                                if(data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_3)
                                {
                                    if(m_Player->m_FellowSkillData[0].Enable_Skill_3 == 1)
                                    {
                                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_3);

                                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_3 == 0)
                                        {
                                            /// PET ICIN
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_3, (DWORD32)pUser)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_3, (DWORD32) g_pMyPlayerObj)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }

                                        }

                                    }
                                }
                            }
                            if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_4 > 0)
                            {
                                if(data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_4)
                                {
                                    if(m_Player->m_FellowSkillData[0].Enable_Skill_4 == 1)
                                    {
                                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_4);

                                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_4 == 0)
                                        {
                                            /// PET ICIN
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_4, (DWORD32)pUser)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_4, (DWORD32) g_pMyPlayerObj)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }

                                        }

                                    }
                                }
                            }
                            if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_5 > 0)
                            {
                                if(data->GetData().Level >= m_CustomDataManager->m_RefFellowPetSystem[NameStr].Active_Level_5)
                                {
                                    if(m_Player->m_FellowSkillData[0].Enable_Skill_5 == 1)
                                    {
                                        int cooldown = g_pCGInterface->GetSkillCoolTimeManager()->FUN_009bba90(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_5);

                                        if(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillType_5 == 0)
                                        {
                                            /// PET ICIN
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_5, (DWORD32)pUser)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            if (cooldown == 0 && !g_pMyPlayerObj->TargetIsBuffInUse(m_CustomDataManager->m_RefFellowPetSystem[NameStr].SkillID_5, (DWORD32) g_pMyPlayerObj)) {

                                                if (!m_Player->PetSkillTimerRunning) {

                                                    g_pCGInterface->StartTimer(START_PETT_BUFF_TIMER_1, 5000);
                                                    m_Player->PetSkillTimerRunning = true;
                                                    break;
                                                }
                                            }

                                        }

                                    }
                                }
                            }


                        }
                    }
                    break;
                }
            }
        }
    }
}
bool CIFCOSManager::OnCreateIMPL(long ln) {
    bool b = reinterpret_cast<bool(__thiscall *)(CIFCOSManager *, long)>(0x0080f050)(this, ln);


    return b;
}
