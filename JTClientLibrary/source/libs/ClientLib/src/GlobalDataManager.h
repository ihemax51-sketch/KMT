#pragma once

#include "Data/ActionWndData.h"
#include "Data/CharacterData.h"
#include "Data/DropGoldData.h"
#include "Data/GachaItemSetData.h"
#include "Data/GachaNpcData.h"
#include "Data/GameGuideData.h"
#include "Data/ItemData.h"
#include "Data/LevelData.h"
#include "Data/LevelGoldData.h"
#include "Data/MagicOptionData.h"
#include "Data/MallItemMenuListData.h"
#include "Data/MessageTipData.h"
#include "Data/NPCChatData.h"
#include "Data/NPCPosData.h"
#include "Data/QuestData.h"
#include "Data/RefPackageItemData.h"
#include "Data/RefPricePolicyOfItem.h"
#include "Data/RefScrapOfPackageItemData.h"
#include "Data/RefShopTabData.h"
#include "Data/RefShopTabGroupData.h"
#include "Data/RefShopdata.h"
#include "Data/ShopItemStockPeriod.h"
#include "Data/ShopItemStockQuantity.h"
#include "Data/TeleportBuilding.h"
#include "Data/TeleportData.h"
#include "Data/TeleportLink.h"
#include <BSLib/_internal/custom_stl.h>

#include <ghidra/undefined.h>
#include <map>
#include <Data/SkillData.h>
#include <Data/SSkillData.h>
#include <GFXFM/IFileManager.h>
#include <Data/ItemMallGroupData.h>


#define g_CGlobalDataManager ((CGlobalDataManager*)0x00EEDF08)

class CGlobalDataManager {
public:
    virtual void Unknown();
    /// \address 0093f710
    void *FUN_0093f710(IFileManager *pFileManager);
    void GetWorldCodeByID(std::wstring* code, unsigned short ID);
    CSOItemPackage* GetSOPackageByShopData(int nShopId, BYTE btGroup, BYTE btTab, BYTE btIndex);
    int GetSOPackageIndexByShop(int nShopId, BYTE btTab, const CSOItemPackage* pPackageItem);
    int GetSOPackageIndexByShop(int nShopId, BYTE btTab, int nRefPackageItemId);
    CSkillData *GetSkillData(undefined4 effectId);
    SSkillData *GetSSkillData(undefined4 param_1);
    undefined4 FUN_008cbac0(int a1);
    struct RegionStr
    {
        char pad_0000[8]; //0x0000
        unsigned short RegionID; //0x0008
        char pad_000A[2]; //0x000A
        float X; //0x000C
        float Y; //0x0010
        float Z; //0x0014
        char pad_0018[40]; //0x0018
    };
    RegionStr* FUN_00938260(int a1);
    CSkillData* GetSkillbyGroupIDandBasicLevel(int a1, int a2);
    undefined4 FUN_008d83a0(int a1);
    /// \address 0093f370
    const CCharacterData *GetCharacter(int id);

    /// \address 0093f2f0
    /// \return valid pointer to item data
    /// \return NULL if item was not found
    const CItemData *GetItem(int refObjItemId) const;

    /// \address 0093f5f0
    /// \remark Will assert when refObjItemId is not found
    const SItemData &GetItemData(int refObjItemId) const;

    /// \brief Get level data for given level
    /// \details Returns the data from leveldata.txt
    /// \param level Level of character or job
    /// \address 00937f20
    /// \remark Known bug: Will crash if value of level is not present in leveldata.txt
    const SLevelData &GetLevelData(int level) const;

    /// \address 00939a60
    undefined FUN_00939a60(undefined4 param_1);

    /// \address 0093a610
    undefined4 FUN_0093a610(std::n_wstring param_1);
    DWORD GetEffectIdByName(wchar_t *szNameStr);
    DWORD GetEffectIdByName2(wchar_t *szNameStr);
    const SItemData * GetItemDataWithCodeName(wchar_t*  a1) const;
    const SItemData *FindItemDataByCodeNameSafe(const std::n_wstring& codeName) const;
    void FUN_0092f1f0(int * p1, int p2);
    unsigned short* CallFuncWithAsm(CSOItem* pItem);
    unsigned __int8 GetEmptyInventorySlots();
    undefined4 FUN_00944940(int a1);
private:
    CRefShopTabData* GetItemMallTabData(int nShopId, BYTE btGroup, BYTE btTab);

    /// \brief Get level object for given level
    /// \param level Level of either character or job
    /// \return pointer to leveldata if level was found
    /// \return NULL if level was not found
    /// \address 0x00937e00
    const CLevelData *GetLevel(int level) const;

    /// \address 0093ef10
    undefined4 LoadFromPk2(int typeMAYBE, int id);

    char pad_0000[360];              //0x0000
public:
    std::map<unsigned __int32, CSkillData *> m_skillDataMap;//0x016C
private:
    char pad_0178[224];              //0x0178

    std::map<unsigned __int32, CItemData *> m_itemDataMap;                               //0x258
    std::n_map<unsigned __int32, CCharacterData *> m_characterDataMap;                   //0x264
    std::map<unsigned __int32, CGameGuideData *> m_gameGuideDataMap;                     //0x270
    char unknown_gap[4 * 20];                                                            // 0x27C
    std::map<unsigned __int32, CActionWndData *> m_actionWndDataMap;                     //0x2CC
    char empty_maps_times_8[3 * 4 * 8];                                                  //0x2D8
    std::map<unsigned __int32, CRefScrapOfPackageItemData *> m_refScrapOfPackageItemData;//0x338
    std::map<unsigned __int32, CRefPackageItemData *> m_refPackageItemDataMap;           //0x344
    std::map<unsigned __int32, CRefPricePolicyOfItem *> m_refPricePolicyOfItemMap;       //0x350
public:
    std::map<unsigned __int32, CRefShopdata *> m_refShopDataMap;                         //0x35C
public:
    std::map<unsigned __int32, CRefShopTabGroupData *> m_refShopTabGroupDataMap;         //0x368
    std::map<unsigned __int32, CRefShopTabData *> m_refShopTabDataMap;                   //0x374
    std::map<unsigned __int32, CItemMallGroupData *> m_mapItemMallGroupData;
    std::map<unsigned __int32,  unsigned __int32> itemmall_related_map2; //0x38C
    char empty_maps_times_8_2[3 * 4 * 8];                                                //0x398
    char refshoptabdata_related_map[3 * 4];                                              //0x3F8
    std::map<unsigned __int32, CShopItemStockQuantity *> m_shopItemStockQuantityMap;     //0x404
public:                                                                                  // TODO: Make a getter for level data and make the member private
    std::map<unsigned __int32, CLevelData *> m_levelDataMap;                             //0x410
private:
    std::map<unsigned __int32, CTeleportData *> m_teleportDataMap;        //0x41C
    std::map<unsigned __int32, CTeleportLink *> m_teleportLinkMap;        //0x428
    std::map<unsigned __int32, CTeleportBuilding *> m_teleportBuildingMap;//0x434
    std::map<unsigned __int32, CNPCChatData *> m_npcChatDataMap;          //0x440
    std::map<unsigned __int32, CMessageTipData *> m_messageTipDataMap;    //0x44C
public:
    std::map<unsigned __int32, CMagicOptionData *> m_magicOptionDataMap;  //0x458
private:
    std::map<unsigned __int32, CQuestData *> m_questDataMap;              //0x464
    char gap1[3 * 4];
    std::map<unsigned __int8, CDropGoldData *> m_dropGoldDataMap;                 //0x47C
    std::map<unsigned __int32, CNPCPosData *> m_npcPosDataMap;                    //0x488
    std::map<unsigned __int32, CMallItemMenuListData *> m_mallItemMenuListDataMap;//0x494
    std::map<unsigned __int32, CShopItemStockPeriod *> m_shopItemStockPeriodMap;  //0x4A0
    char gap2[26 * 4];                                                    //0x4AC
    std::map<unsigned __int32, CGachaItemSetData *> m_gachaItemSetDataMap;//0x4FC
    std::map<unsigned __int32, CGachaNpcData *> m_gachaNpcDataMap;        //0x508
    char gap3[4 * 4];                                                     //0x514
    std::map<unsigned __int8, CLevelGoldData *> m_levelGoldDataMap;       //0x524
    char gap4[3 * 4];


private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(m_skillDataMap, 0x016C)
        ENSURE_OFFSET(m_characterDataMap, 0x0264)
        ENSURE_OFFSET(m_levelDataMap, 0x0410)
    END_FIXTURE()

    RUN_FIXTURE(CGlobalDataManager)






};
