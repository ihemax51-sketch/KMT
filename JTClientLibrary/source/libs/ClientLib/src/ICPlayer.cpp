#include <CustomData/CustomDataManager.h>
#include "ICPlayer.h"
#include "GInterface.h"
#include "GlobalDataManager.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CICPlayer, 0x00ef1b44)


// CICPlayer::IsGameMasterMAYBE(void) .text 009D4C40 00000009   R . . . . T .
bool CICPlayer::IsGameMaster() {
    return N000094A7 & 1;
}

std::n_wstring CICPlayer::GetCharName() const {
    return m_wstrCharName;
}
const std::n_wstring &CICPlayer::GetTitleName() const
{
    return m_titleText;
}
unsigned char CICPlayer::GetCurrentLevel() const {
    return m_btLevel;
}

long long int CICPlayer::GetCurrentExp() const {
    return m_i64CurrentExp;
}

short CICPlayer::GetStatPointAvailable() const {
    return m_sStatPointAvailable;
}

short CICPlayer::GetStrength() const {
    return m_sStrStat;
}

short CICPlayer::GetIntelligence() const {
    return m_sIntStat;
}

const std::n_wstring &CICPlayer::GetJobAlias() const {
    return m_wstrJobAlias;
}

int CICPlayer::GetCurrentJobExperiencePoints() const {
    return reinterpret_cast<int (__thiscall *)(const CICPlayer *)>(0x009d4d20)(this);
}

void CICPlayer::Func_15(int param_1, float *param_2) {
    //printf("%s %d %p\n", __FUNCTION__, param_1, param_2);
    //reinterpret_cast<void (__thiscall *)(const CICharactor *, int param_1, float *param_2)>(0x009d49c0)(this, param_1, param_2);
    if ((param_1 == 3) && (g_pCGInterface != NULL)) {
        g_pCGInterface->FUN_00777a70(0, 1);
    }
    CICUser::Func_15(param_1, param_2);
}

void CICPlayer::Func_15_impl(int param_1, float *param_2) {
    CICPlayer::Func_15(param_1, param_2);
}

undefined4 CICPlayer::FUN_009d68f0() {
    return reinterpret_cast<undefined4 (__thiscall *)(const CICPlayer *)>(0x009d68f0)(this);
}
int CICPlayer::GetWorldID()
{
    return reinterpret_cast<int(__thiscall*)(CICPlayer*)>(0x009D4D90)(this) % 65536;
}

CICPlayer* CICPlayer::GetCICPlayerByUniqueID(DWORD UniqueID)
{
    return reinterpret_cast<CICPlayer*(__cdecl*)(DWORD)>(0x009C3220)(UniqueID);
}
DWORD CICPlayer::GetObjEffect() /// 009cf990 DWORD PTR
{//Does this gets the proper NEEDED effect or it might be getting another effect? well, its the proper addr of the effect of the whole objects in sro, and i got it from makeeffect function so xD
    return m_Effects;
}
void CICPlayer::MakeEffect(unsigned __int32 effectID)
{
    reinterpret_cast<void* (__thiscall*)(CICPlayer*, unsigned __int32)>(0x009CF960)(this, effectID);
}

void CICPlayer::RemoveEffect(unsigned __int32 effectID)
{
    reinterpret_cast<void* (__thiscall*)(CICPlayer*, unsigned __int32)>(0x009CD2B0)(this, effectID);
}

boolean CICPlayer::CheckSpeedOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;

                int CheckScroll = 6;
                int CheckChSpeed = 989855744;
                int CheckEuSpeed = 1073741830;


                // Speed overlap kontrolü
                if ((targetSkillOverlap == CheckChSpeed && currentSkillOverlap == CheckEuSpeed) ||
                    (targetSkillOverlap == CheckEuSpeed && currentSkillOverlap == CheckChSpeed) ||
                    (targetSkillOverlap == CheckScroll && currentSkillOverlap == CheckScroll))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

DWORD32 CICPlayer::GetBuffListBase(DWORD32 addr) {
    if (g_pMyPlayerObj)
        return *(DWORD32*)(addr + 0x288);
    else
        return 0;
}
boolean CICPlayer::TargetIsBuffInUse(DWORD32 SkillID, DWORD32 Target) {
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0)
                return false;
            if (Base == dwListBase)
                return false;
            if (SkillID == *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C)) {
                return true;
            }
        }
    }
    return false;
}

boolean CICPlayer::CheckMagOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;

                int ChFireMagDef = 3;
                int EuDeityMagDef = 1744830467;

                // Mag overlap kontrolü
                if ((targetSkillOverlap == ChFireMagDef && currentSkillOverlap == EuDeityMagDef) ||
                    (targetSkillOverlap == EuDeityMagDef && currentSkillOverlap == ChFireMagDef))
                {

                    return true;
                }
            }
        }
    }
    return false;
}

boolean CICPlayer::CheckPhyOverlap(DWORD32 SkillID, DWORD32 Target)
{
    DWORD32 dwListBase = GetBuffListBase(Target);
    if (dwListBase != 0)
    {
        DWORD32 Base = dwListBase;
        for (int guard = 0; guard < 256; ++guard)
        {
            Base = *(DWORD32*)(Base);
            if (Base == 0 || Base == dwListBase)
            {
                break;
            }
            DWORD32 currentSkillID = *(DWORD32*)(*(DWORD32*)(Base + 0x8) + 0x5C);
            if (g_CGlobalDataManager->GetSkillData(currentSkillID) != NULL && g_CGlobalDataManager->GetSkillData(SkillID) != NULL)
            {
                int currentSkillOverlap = g_CGlobalDataManager->GetSkillData(currentSkillID)->Action_Overlap;
                int targetSkillOverlap = g_CGlobalDataManager->GetSkillData(SkillID)->Action_Overlap;

                int ChColdPhyDef = 2;
                int EuDeityPhyDef = 1728053250;


                // Phy overlap kontrolü
                if ((targetSkillOverlap == ChColdPhyDef && currentSkillOverlap == EuDeityPhyDef) ||
                    (targetSkillOverlap == EuDeityPhyDef && currentSkillOverlap == ChColdPhyDef))
                {

                    return true;
                }
            }
        }
    }
    return false;
}
void __fastcall CICPlayer::EffectHook(CICPlayer* param_1)
{

    /*   int UQID = param_1->GetUniqueId();

       std::map<int, int>::iterator it = g_pCICPlayer->UniqueTargetHashmapPlayer.find(UQID);
       if(it->first == UQID)
       {
           printf("unique mapta varsa silemiyorum\n");
       }
   */
    /* else
     {
     reinterpret_cast<void* (__fastcall*)(CICPlayer*)>(0x009cd5c0)(param_1);
     }*/
    return;
}
void CICPlayer::RemoveExistingUniqueData(const DWORD dead_object)
{
    if (g_pMyPlayerObj)
    {
        bool remove_key_player = false;
        bool remove_key_unique = false;
        unsigned __int32 remove_val_key = 0;

        std::map<int, int>::iterator it_1 = m_CustomDataManager->UniqueTargetHashmapPlayer.find(dead_object);
        if (it_1 != m_CustomDataManager->UniqueTargetHashmapPlayer.end()) //player gitti, player map'ten playerı sil, unique map'ten value 0 yap.
        {
            remove_key_player = true;
            remove_val_key = it_1->second;
        }

        if (!remove_key_player) //player değil ise bak
        {
            std::map<int, int>::iterator it_2 = m_CustomDataManager->UniqueTargetHashmap.find(dead_object);
            if (it_2 != m_CustomDataManager->UniqueTargetHashmap.end()) //unique gitti, player mapten unique'i 0 yap, unique map'ten unique'i sil.
            {
                remove_key_unique = true;
                remove_val_key = it_2->first;

                CICPlayer* LastTarget = g_pMyPlayerObj->GetCICPlayerByUniqueID(it_2->second);
                if (LastTarget)
                {
                    LastTarget->RemoveEffect(g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT"));
                }
            }
        }
        else //player gitmiş
        {
            m_CustomDataManager->UniqueTargetHashmapPlayer.erase(remove_val_key);

            if (m_CustomDataManager->UniqueTargetHashmap.count(remove_val_key) > 0)
                m_CustomDataManager->UniqueTargetHashmap[remove_val_key] = 0;

            return;
        }

        if (remove_key_unique)
        {
            m_CustomDataManager->UniqueTargetHashmap.erase(remove_val_key);

            if (m_CustomDataManager->UniqueTargetHashmapPlayer.count(remove_val_key) > 0)
                m_CustomDataManager->UniqueTargetHashmapPlayer[remove_val_key] = 0;
        }
    }
}
