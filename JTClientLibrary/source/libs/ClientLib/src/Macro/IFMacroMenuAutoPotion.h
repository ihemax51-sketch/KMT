#pragma once
#include <IFSelectableArea.h>
#include <IFCheckBox.h>
#include <IFSliderCtrl.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"
#include "IFMacroSlot.h"

class CIFMacroMenuAutoPotion : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoPotion)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoPotion)
private:
    enum MacroSlots
    {
        HP_SLOT = 0,
        MP_SLOT = 1,
        CHAR_PILL_SLOT = 2,
        CHAR_PURI_PILL_SLOT = 3,
        CHAR_SPEED_SLOT = 4,
        CHAR_VIGOR_HP_SLOT = 5,
        CHAR_VIGOR_MP_SLOT = 6,
        PET_HP_SLOT = 7,
        PET_MP_SLOT = 8,
        PET_PILL_SLOT = 9,
        PET_RES_SLOT= 10,
        PET_SUMMON_SLOT = 11
    };
    CIFMacroMenuAutoPotion(void);
    ~CIFMacroMenuAutoPotion(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnUnknownStuff();
    static const int numberOfTabs = 3;
    static const int tabWidth = 200;
    static const int tabHeight = 24;
    static const int tabFirstId = 300;
    undefined1 OnCloseWnd() override;
public:
    bool Macro_AutoPotion;
    bool CharacterHpTimerRunning;
    bool CharacterMpTimerRunning;
    bool CharacterHpUsing;
    bool CharacterMpUsing;
    bool CharacterVigorTimerRunning;
    bool CharacterVigorUsing;
    bool CharacterVigorTimerRunningMP;
    bool CharacterVigorUsingMP;
    bool CharacterPillTimerRunning;
    bool CharacterPillUsing;
    bool CharacterPillTimerRunningPuri;
    bool CharacterPillUsingPuri;
    bool CharacterSpeedTimerRunning;
    bool PetHpTimerRunning;
    bool PetHpUsing;
    bool PetHgpTimerRunning;
    bool PetHgpUsing;
    bool PetPillTimerRunning;
    bool PetPillUsing;
    bool SummonTimerRunning;
    bool PetResTimerRunning;


    CIFCheckBox* CharHP_CheckBox;
    CIFStatic* CharHP_SliderValue;
    CIFSliderCtrl* CharHP_Slider;

    CIFCheckBox* CharMP_CheckBox;
    CIFStatic* CharMP_SliderValue;
    CIFSliderCtrl* CharMP_Slider;

    CIFCheckBox* PetHP_CheckBox;
    CIFStatic* PetHP_SliderValue;
    CIFSliderCtrl* PetHP_Slider;

    CIFCheckBox* PetMP_CheckBox;
    CIFStatic* PetMP_SliderValue;
    CIFSliderCtrl* PetMP_Slider;

    CIFCheckBox* CharPill_CheckBox;
    CIFCheckBox* CharSpeed_CheckBox;
    CIFCheckBox* CharPuriPill_CheckBox;


    CIFCheckBox* PetPill_CheckBox;
    CIFCheckBox* PetRes_CheckBox;
    CIFCheckBox* PetSpawn_CheckBox;

    CIFCheckBox* CharVigorHP_CheckBox;
    CIFStatic* CharVigorHP_SliderValue;
    CIFSliderCtrl* CharVigorHP_Slider;

    CIFCheckBox* CharVigorMP_CheckBox;
    CIFStatic* CharVigorMP_SliderValue;
    CIFSliderCtrl* CharVigorMP_Slider;

    CIFMacroSlot* MySlots[12];


    void UpdateMenuSize();
    void ActivateTabPage(BYTE page);
    void SaveButton();
    void CancelBtn();
    void AutoSave(byte SlotSeq, byte SlotType, int SlotData);
    void LoadSettings(byte Slot, byte Active, byte Value);
    void CheckCharacterHP();
    void UseCharacterHpPotion();

    void CheckCharacterMP();
    void UseCharacterMpPotion();

    void CheckCharacterVigorHP();
    void UseCharacterVigorHpPotion();

    void CheckCharacterVigorMP();
    void UseCharacterVigorMpPotion();


    void CheckCharacterPILL();
    void UseCharacterPILL();

    void CheckCharacterPILLPuri();
    void UseCharacterPILLPuri();
    void UseCharacterSpeed();


    void CheckPetHP();
    void UsePetHpPotion();


    void CheckPetHgp();
    void UsePetHgpPotion();

    void CheckPetPILL();
    void UsePetPILL();
    void CheckSummonedPet();
    void CheckDeadPet();
    bool IsRuntimeReady() const;
    void StartAutomation();
    void StopAutomation();


    CIFSelectableArea** m_pTabsSecond;
    struct Macro_AutoPotionData
    {
        BYTE Slot;
        BYTE Active;
        BYTE Value;
    };

    void UpdateSlot();
    void UpdatePetSlotVisual(byte i);
    int GetHPItemID();
    int GetMPItemID();

};
