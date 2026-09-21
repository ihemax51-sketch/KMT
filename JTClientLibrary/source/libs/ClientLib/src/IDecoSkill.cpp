//
// Created by YUMBUL on 14.08.2024.
//
#include <ghidra/undefined.h>
#include <cstdio>
#include <CustomData/CustomDataManager.h>
#include "IDecoSkill.h"
#include "ICPlayer.h"


void CIDecoSkill::CreateEffect(int dwInst, int player, int effectId, undefined4 param_4, undefined4 param_5, int param_6)
{
    CICPlayer* test = reinterpret_cast<CICPlayer *>(player);
    if(test)
    {
        if(m_CustomDataManager->HideEffects.find(effectId) != m_CustomDataManager->HideEffects.end())
        {
            if(m_CustomDataManager->HideEffects[effectId].JobMode)
            {
                if(test->GetJobType() != 4)
                {
                    return;
                }
            }
            if(m_CustomDataManager->HideEffects[effectId].MapSettings)
            {
                if(m_CustomDataManager->m_EventMapSettings.find(test->GetRegion().r) != m_CustomDataManager->m_EventMapSettings.end())
                {
                    return;
                }
            }
        }


    }
    reinterpret_cast<void(__thiscall *)(CIDecoSkill *, int, int,int,undefined4 ,undefined4,int)>(0x00a54020)(this, dwInst, player, effectId, param_4, param_5, param_6);
}