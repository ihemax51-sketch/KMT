#include "GlobalDataManager.h"
#include "ICPlayer.h"
#include <BSLib/Debug.h>
#include <GInterface.h>
#include <Data/ItemMallGroupData.h>
#include <BSLib/multibyte.h>

undefined4 CGlobalDataManager::FUN_008cbac0(int a1) {
    return reinterpret_cast<undefined4(__thiscall *)(CGlobalDataManager *, int)>(0x008cbac0)(this, a1);
}
CGlobalDataManager::RegionStr* CGlobalDataManager::FUN_00938260(int a1) {
    //printf("%p\n", a1);
    if(a1 == 500000)
    {
        static RegionStr region;
        region.RegionID = 25503;
        region.X = 1284;
        region.Y = 111;
        region.Z = 1305;
        return &region;
    }

    RegionStr* aa = reinterpret_cast<CGlobalDataManager::RegionStr*(__thiscall *)(CGlobalDataManager *, int)>(0x00938260)(this, a1);
    //printf("%p %d\n", aa, a1);

    return aa;
}

void *CGlobalDataManager::FUN_0093f710(IFileManager *pFileManager) {
    return reinterpret_cast<void *(__thiscall *) (CGlobalDataManager *, IFileManager *)>(0x0093f710)(this, pFileManager);
}
void CGlobalDataManager::GetWorldCodeByID(std::wstring* code, unsigned short ID)
{
    reinterpret_cast<void(__thiscall*)(CGlobalDataManager*, std::wstring*, unsigned short)>(0x009399B0)(this, code, ID);
}

CRefShopTabData *CGlobalDataManager::GetItemMallTabData(int nShopId, BYTE btGroup, BYTE btTab) {
    std::map<unsigned __int32, CItemMallGroupData *>::const_iterator it = m_mapItemMallGroupData.find(nShopId);
    if (it == m_mapItemMallGroupData.end() || !it->second ||
        btGroup >= it->second->m_vShopData.size()) {
        return NULL;
    }

    CRefShopdata *pShopData = it->second->m_vShopData[btGroup];
    if (!pShopData || pShopData->m_vRefShopTabGroupData.empty()) {
        return NULL;
    }

    CRefShopTabGroupData *pTabGroupData = pShopData->m_vRefShopTabGroupData[0];
    if (!pTabGroupData || btTab >= pTabGroupData->m_vRefShopTabData.size()) {
        return NULL;
    }

    return pTabGroupData->m_vRefShopTabData[btTab];
}

CSOItemPackage *CGlobalDataManager::GetSOPackageByShopData(int nShopId, BYTE btGroup, BYTE btTab, BYTE btIndex) {
    CRefShopTabData *pTabData = GetItemMallTabData(nShopId, btGroup, btTab);
    if (!pTabData || btIndex >= pTabData->m_vPackageItems.size()) {
        return NULL;
    }

    return pTabData->m_vPackageItems[btIndex];
}

int CGlobalDataManager::GetSOPackageIndexByShop(int nShopId, BYTE btTab,
                                                const CSOItemPackage *pPackageItem) {
    if (!pPackageItem) {
        return -1;
    }

    std::map<unsigned __int32, CRefShopdata *>::const_iterator shopIt = m_refShopDataMap.find(nShopId);
    if (shopIt == m_refShopDataMap.end() || !shopIt->second ||
        shopIt->second->m_vRefShopTabGroupData.empty()) {
        return -1;
    }

    CRefShopTabGroupData *pTabGroupData = shopIt->second->m_vRefShopTabGroupData[0];
    if (!pTabGroupData || btTab >= pTabGroupData->m_vRefShopTabData.size()) {
        return -1;
    }

    CRefShopTabData *pTabData = pTabGroupData->m_vRefShopTabData[btTab];
    if (!pTabData) {
        return -1;
    }

    for (size_t i = 0; i < pTabData->m_vPackageItems.size(); ++i) {
        CSOItemPackage *pCandidate = pTabData->m_vPackageItems[i];
        if (pCandidate == pPackageItem) {
            return static_cast<int>(i);
        }
    }

    CRefPackageItemData *pTargetData = pPackageItem->GetPackageItemData();
    for (size_t i = 0; i < pTabData->m_vPackageItems.size(); ++i) {
        CSOItemPackage *pCandidate = pTabData->m_vPackageItems[i];
        CRefPackageItemData *pCandidateData = pCandidate ? pCandidate->GetPackageItemData() : NULL;
        if (pTargetData && pCandidateData && pCandidateData->m_id == pTargetData->m_id) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int CGlobalDataManager::GetSOPackageIndexByShop(int nShopId, BYTE btTab,
                                                int nRefPackageItemId) {
    std::map<unsigned __int32, CRefShopdata *>::const_iterator shopIt = m_refShopDataMap.find(nShopId);
    if (shopIt == m_refShopDataMap.end() || !shopIt->second ||
        shopIt->second->m_vRefShopTabGroupData.empty()) {
        return -1;
    }

    CRefShopTabGroupData *pTabGroupData = shopIt->second->m_vRefShopTabGroupData[0];
    if (!pTabGroupData || btTab >= pTabGroupData->m_vRefShopTabData.size()) {
        return -1;
    }

    CRefShopTabData *pTabData = pTabGroupData->m_vRefShopTabData[btTab];
    if (!pTabData) {
        return -1;
    }

    for (size_t i = 0; i < pTabData->m_vPackageItems.size(); ++i) {
        CSOItemPackage *pPackageItem = pTabData->m_vPackageItems[i];
        CRefPackageItemData *pPackageData = pPackageItem ? pPackageItem->GetPackageItemData() : NULL;
        if (pPackageData && static_cast<int>(pPackageData->m_id) == nRefPackageItemId) {
            return static_cast<int>(i);
        }
    }

    return -1;
}


const CCharacterData *CGlobalDataManager::GetCharacter(int id) {
    std::n_map<unsigned __int32, CCharacterData *>::const_iterator it = m_characterDataMap.find(id);
    if (it == m_characterDataMap.end()) {
        if (LoadFromPk2(0, id) == 0) {
            return NULL;
        } else {
            it = m_characterDataMap.find(id);
        }
    }
    return it->second;
}

const SItemData &CGlobalDataManager::GetItemData(int refObjItemId) const {

    const CItemData *pItem = GetItem(refObjItemId);
    BS_ASSERT_MSG(pItem != NULL, "Item: %d", pItem);
    return pItem->GetData();
}

const CItemData *CGlobalDataManager::GetItem(int refObjItemId) const {
    return reinterpret_cast<const CItemData *(__thiscall *)(const CGlobalDataManager *, int)>(0x0093f2f0)(this,
                                                                                                          refObjItemId);
}

const CLevelData *CGlobalDataManager::GetLevel(int level) const {
    std::map<unsigned __int32, CLevelData *>::const_iterator it = m_levelDataMap.find(level);
    if (it != m_levelDataMap.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}

const SLevelData &CGlobalDataManager::GetLevelData(int level) const {
    const CLevelData *levelData = GetLevel(level);
    return levelData->GetData();
}

undefined CGlobalDataManager::FUN_00939a60(undefined4 param_1) {
    return reinterpret_cast<undefined (__thiscall *)(const CGlobalDataManager *, undefined4)>(0x00939a60)(this,
                                                                                                          param_1);
}

undefined4 CGlobalDataManager::FUN_0093a610(std::n_wstring param_1) {
    return reinterpret_cast<undefined4 (__thiscall *)(const CGlobalDataManager *, std::n_wstring)>(0x0093a610)(this, param_1);
}

undefined4 CGlobalDataManager::LoadFromPk2(int typeMAYBE, int id) {
    return reinterpret_cast<undefined4(__thiscall *)(CGlobalDataManager *, int, int)>(0x0093ef10)(this, typeMAYBE, id);
}
DWORD CGlobalDataManager::GetEffectIdByName(wchar_t* szNameStr)
{
    return reinterpret_cast<DWORD(__cdecl*)(wchar_t*, DWORD)>(0x00A950A0)(szNameStr, 0);
}
DWORD CGlobalDataManager::GetEffectIdByName2(wchar_t* szNameStr)
{
    return reinterpret_cast<DWORD(__cdecl*)(wchar_t*, DWORD)>(0x00a95240)(szNameStr, 0);
}

const SItemData * CGlobalDataManager::GetItemDataWithCodeName(wchar_t* szNameStr) const
{
    const SItemData* aa = reinterpret_cast<const SItemData*(__thiscall*)(const CGlobalDataManager*, wchar_t*)>(0x009461a0)(this, szNameStr);

    return aa;
}

const SItemData *CGlobalDataManager::FindItemDataByCodeNameSafe(const std::n_wstring& codeName) const
{
    if (codeName.empty()) {
        return NULL;
    }

    for (std::map<unsigned __int32, CItemData *>::const_iterator it = m_itemDataMap.begin(); it != m_itemDataMap.end(); ++it) {
        const CItemData *item = it->second;
        if (!item) {
            continue;
        }

        const SItemData &data = item->GetData();
        if (data.CodeName == codeName || data.OrgObjCodeName == codeName) {
            return &data;
        }
    }

    return NULL;
}


void CGlobalDataManager::FUN_0092f1f0(int * p1, int p2)
{
    ///printf("%p %d \n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CGlobalDataManager*, int*, int)>(0x0092f1f0)(this, p1, p2);
}
unsigned short* CGlobalDataManager::CallFuncWithAsm(CSOItem* pItem)
{
    // Bellek adresleri ve değişkenler
    DWORD dwGetItemDataOffset = 0x008ba440; // GetItemData offseti
    DWORD dwCallAddr_Func = 0x009461a0;    // FUN_009461a0 adresi

    int iVar6 = 0;
    unsigned short* puVar4 = NULL;

    __asm
    {
    ; ECX'e this pointer'ı ata
    mov ecx, [this]

    ; GetItemData fonksiyonunu çağır
    mov edx, pItem           ; pItem işaretçisini edx'e ata
    call dwGetItemDataOffset ; GetItemData fonksiyonunu çağır

    ; EAX register'ındaki sonucu iVar6'ya ata
    mov iVar6, eax

    ; FUN_009461a0 fonksiyonunu çağır
    mov ecx, [this]           ; this pointer'ını ecx'e ata
    mov eax, iVar6            ; iVar6 değerini eax'e ata
    add eax, 0x304            ; iVar6'ya 0x304 ekle
    call dwCallAddr_Func      ; FUN_009461a0 fonksiyonunu çağır

    ; EAX register'ındaki sonucu puVar4'e ata
    mov puVar4, eax
    }

    return puVar4;
}




CSkillData* CGlobalDataManager::GetSkillData(undefined4 effectId)
{
    CSkillData* aa = reinterpret_cast<CSkillData *(__thiscall*)(CGlobalDataManager*, int)>(0x0093f250)(this, effectId);
    return aa;
}

SSkillData* CGlobalDataManager::GetSSkillData(undefined4 param_1)

{
    SSkillData *aa = reinterpret_cast<SSkillData *(__thiscall *) (CGlobalDataManager *, int)>(0x0093f5d0)(this, param_1);
    return aa;
}

CSkillData* CGlobalDataManager::GetSkillbyGroupIDandBasicLevel(int SkillID, int a2) {

    CSkillData* a = reinterpret_cast<CSkillData*(__thiscall *)(CGlobalDataManager *, int, int)>(0x0093f770)(this, SkillID, a2);
    return a;
}
undefined4 CGlobalDataManager::FUN_008d83a0(int a1) {
    undefined4 a =  reinterpret_cast<undefined4(__thiscall *)(CGlobalDataManager *, int)>(0x008d83a0)(this, a1);
    return a;
}
undefined4 CGlobalDataManager::FUN_00944940(int a1) {
    undefined4 a =  reinterpret_cast<undefined4(__thiscall *)(CGlobalDataManager *, int)>(0x00944940)(this, a1);
    return a;
}
unsigned __int8 CGlobalDataManager::GetEmptyInventorySlots()
{
    DWORD dwCallAddr_1 = 0x00798D00;
    DWORD dwCallAddr_2 = 0x006A1D20;
    DWORD dwCallAddr_3 = 0x006B6230;

    unsigned __int8 remainingSlots = 0;

    __asm
    {
    mov ecx, dword ptr ds : 0x0110F80C;
    call dwCallAddr_1;
    mov ecx, eax;
    call dwCallAddr_2;
    mov ecx, eax;
    call dwCallAddr_3;
    mov remainingSlots, al;
    }

    return remainingSlots;
    }
