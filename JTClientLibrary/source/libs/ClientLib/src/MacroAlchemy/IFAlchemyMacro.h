#pragma once
#include <IFSelectableArea.h>
#include <IFCheckBox.h>
#include <IFScrollManager.h>
#include <IObject.h>
#include <IFEdit.h>
#include <IFDecoratedStatic.h>
#include <CustomInterface/IFPopupList.h>
#include "d3dx9math.h"
#include "IFMacroAlchemySlot.h"
#include "IFAlchemyMacroBlueSlot.h"
#include "IFAlchemyMacroAttrSlot.h"

class CIFAlchemyMacro : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFAlchemyMacro)
GFX_DECLARE_MESSAGE_MAP(CIFAlchemyMacro)
private:
    CIFAlchemyMacro(void);
    ~CIFAlchemyMacro(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnTimer(int) override;
    int Func_4(int a2) override;

void AddMattrToList();
void RemoveMattrToList();
    int getDecimalFromLast4HexDigits(int value);
public:
    byte TargetPlus;
    CIFStatic* TargetPlusLabel;
    CIFAlchemyMacroBlueSlot* m_BlueSlots[11];
    CIFAlchemyMacroAttrSlot* m_AttrSlots[11];
    CIFScrollManager* m_scroll;
    CIFScrollManager* m_scrollattr;
    byte LuckyPowderMinPlus;
    CIFCheckBox* LuckyPowderCheckBox;
    CIFStatic* LuckyPowderMinPlusLabel;


    byte ImmortalMinPlus;
    CIFCheckBox* ImmortalCheckBox;
    CIFStatic* ImmortalMinPlusLabel;

    byte AstralMinPlus;
    CIFCheckBox* AstralCheckBox;
    CIFStatic* AstralMinPlusLabel;

    byte SteadyMinPlus;
    CIFCheckBox* SteadyCheckBox;
    CIFStatic* SteadyMinPlusLabel;

    byte LuckyMinPlus;
    CIFCheckBox* LuckyCheckBox;
    CIFStatic* LuckyMinPlusLabel;

    undefined1 OnCloseWnd() override;
    CIFMacroAlchemySlot* m_ItemSlot;
    CIFDecoratedStatic* Slotdeco;
    bool Fusing;
    bool AutomationRunning;

    byte CurrentPage;

    std::wstring SelectedAttrCode;
    std::wstring SelectedRemoveAttrCode;
    int GetDegreeLevel(int itemClass);
    void StartButton();
    void StopButton();
    void StopAutomation(const wchar_t* reason = NULL);
    void OnAlchemyResponse();
    void OnTrackedItemRemoved();
    void UpdateMenuSize();
    void PLUS_UP_BUTTON();
    void PLUS_DOWN_BUTTON();
    void LUCKYPOWDER_UP_BUTTON();
    void LUCKYPOWDER_DOWN_BUTTON();
    void IMMORTAL_UP_BUTTON();
    void IMMORTAL_DOWN_BUTTON();
    void ASTRAL_UP_BUTTON();
    void ASTRAL_DOWN_BUTTON();
    void STEADY_UP_BUTTON();
    void STEADY_DOWN_BUTTON();
    void LUCKY_UP_BUTTON();
    void LUCKY_DOWN_BUTTON();
    int CheckImmortal();
    int FindImmortal();
    void FusePlus();
    int FindBlueStone(std::wstring OptName);
    int FindAttrStone(std::wstring OptName);
    int CheckAstral();
    int FindAstral();
    int CheckImmortalForAstral();
    int CheckSteady();
    int FindSteady();
    int CheckLucky();
    int FindLuck();
    int CheckLuckyPowder();
    int FindLuckPowder();
    int FindElixir();

    void AutoPlusPage();
    void AutoBluePage();
    void AutoStatPage();

    void ClearDDJList();
    void ClearDDJListAttr();
    enum eBlueId
    {
        STR,
        INT,
        HP,
        MP,
        PARRY_RATE,
        ATTACK_RATE,
        DURABILITY,

    };
    struct MagStr
    {
        int MagID;
        int Param1;
        int Param4;
        std::wstring Code;
    };
    std::vector<MagStr> m_BlueList;

    struct AttrStr
    {
        std::wstring Code;
        std::wstring Name;
    };
    std::vector<AttrStr> m_AttrList;

    struct TargetAttrStr
    {
        std::wstring Code;
        std::wstring Name;
        int Target;
    };
    std::vector<TargetAttrStr> m_TargetAttrList;

void WeaponAddedtoSlot();
void ClearList();
void ClearListAttr();

void LoadBlueList();
void LoadAttrList();
void ShieldAddedtoSlot();
void HeadChestLegsAddedtoSlot();
void ShoulderHandsFootAddedtoSlot();
void AccAddedtoSlot();
void FuseBlueNew();
float CalculateAttack(float minL, float minU, float maxL, float maxU, float plus, float inc, float currentMin, float currentMax);
    double round(double value);
void LoadTargetAttrList();
void FuseAttrStone();
float CalculateNoPlus(float minL, float minU, float maxL, float maxU, float currentMin, float currentMax);
float CalculateAttackSecond(float minL, float maxL, float plus, float inc, float currentValue);
float CalculateAttackSecondNoPlus(float minL, float maxL, float currentValue);
    int FindAttrs(int index);

private:
    bool HasTrackedItem() const;
    void StartOperationTimeout();
};
