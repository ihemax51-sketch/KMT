#pragma once
#include <IFSelectableArea.h>
#include <IFCheckBox.h>
#include <IFSliderCtrl.h>
#include <IFScrollManager.h>
#include <SRIFLib/NIFLattice.h>
#include <ICUser.h>
#include <CustomInterface/IFPopupList.h>
#include <CustomInterface/IFPopupList2.h>
#include <ICMonster.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"
#include "IFMacroSlot.h"
#include "IFMacroMenuAutoSkillSlotViewer.h"
#include "IFMacroSlotWep.h"
#include "IFMacroSlotSkill.h"
#include "IFMacroMenuAutoSkillPartySlot.h"

class CIFMacroMenuAutoSkill : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoSkill)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoSkill)
private:
    CIFMacroMenuAutoSkill(void);
    ~CIFMacroMenuAutoSkill(void);
    bool OnCreate(long ln) override;
    void AddPtMemberBtn();
    void RemovePtMemberBtn();
    void OnUpdate() override;
    void OnUnknownStuff();
    void LoadSkills();
    void LoadBuffs();
    void SaveButton();
    void CancelButton();
    void Clear();
    int Func_4(int a2) override;
    static const int numberOfTabs = 3;
    static const int tabWidth = 204;
    static const int tabHeight = 24;
    static const int tabFirstId = 300;
    int replace;
    enum SRadiusList
    {
        GDR_RADIUS_30 = 0,
        GDR_RADIUS_50 = 1,
        GDR_RADIUS_70 = 2,
        GDR_RADIUS_100 = 3,
        GDR_RADIUS_150 = 4,
        GDR_RADIUS_200 = 5,
        GDR_RADIUS_300 = 6,
    };
public:
    bool Macro_AutoSkill;
    void UpdateMenuSize();
    void ActivateTabPage(BYTE page);
    void LoadPartyBuffInfos();
    void ClearPartySlotDDJ();
    void AutoSave(int SlotSeq, byte SlotType, int SlotData);
    CIFScrollManager* m_scroll_skill_list;
    CIFMacroMenuAutoSkillSlotViewer *m_slotviewer[18];
    CIFMacroMenuAutoSkillPartySlot* m_PartySlot[7];
    CIFMacroSlotSkill* skillslots[24];
    CIFMacroSlotSkill* buffslots[24];
    byte ActiveTabPage;
    CIFPopupList2* m_popup;
    CIFSelectableArea** m_pTabsSecond;
    CNIFLattice * ps;
    std::n_wstring SelectedPartyMemberName;

    std::map<std::n_wstring, std::vector<int>> PartyBuffList;

    CIFMacroSlotWep* SkillWeaponSlot;
    CIFMacroSlotWep* BuffWeaponSlot;

    CIFMacroSlotWep* SkillShieldSlot;
    CIFMacroSlotWep* BuffShieldSlot;

    bool AutoSkillTimerRunning;
    bool CheckBuffWeaponSlot(int Type1, int Type2);
    bool CheckBuffShieldCondition();
    bool CheckSkillWeaponAndWear(int Type1, int Type2);
    bool CheckSkillShieldCondition();
    bool CheckBuffShieldSlot();
    bool CheckSkillWeaponSlots();
    bool IsAutoSkillRuntimeReady();
    const SItemData* GetEquippedItemData(byte slot);
    void ResetSelectedTarget();
    CICharactor* ResolveSelectedTarget();
    void SetSelectedTarget(CICharactor* target);
    void StartAutoSkill();
    void JustAutoAttack();
    int FindAttackSkillSlot();
    bool SkillVeItemUyumu(tid_t CastWeapon1, tid_t CastWeapon2);
    int FindNeededBuff();
    void FindMonsterViaAttackAndPartyBuff();
    CICMonster* GetNearMonsterID();
    int AutoResurrrectPartyMember();

    boolean TargetIsBuffInUse(DWORD32 SkillID, DWORD32 Target);
    DWORD32 GetBuffListBase(DWORD32 Addr);

    float CalculateDistance(const D3DXVECTOR3& loc1, const D3DXVECTOR3& loc2);
    D3DVECTOR generateRandomPositionWithinDistance(const D3DVECTOR& center, float minDistance, float maxDistance);
    D3DXVECTOR3 ConvertToGlobalCoordinates(const uregion& region, const D3DXVECTOR3& localCoords);
    CICharactor* GetNearPartyDeadMemberID();
    int IHaveResurrectSkill(byte Level);
    bool BuffVeItemUyumu(tid_t CastWeapon1, tid_t CastWeapon2);

    time_t MyLastResUseTime;
    void LoadPartyBuffs();
    void SelectedPartyBuffs();
    void LoadPartyMembers();
    int JustPartyBuff();
    CICharactor * GetNearPartyMemberID();
    boolean CheckMagOverlap(DWORD32 SkillID, DWORD32 Target);
    boolean CheckPhyOverlap(DWORD32 SkillID, DWORD32 Target);
    boolean CheckSameBuffOverlap(DWORD32 SkillID, DWORD32 Target);
    CIObject* GetNearestObj();

    CICharactor* SelectObj;
    unsigned int SelectObjUniqueId;
    std::vector<CIGIDObject*> InvalidObjects;
};
