#pragma once
#include <IFSelectableArea.h>
#include <IFTextBox.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"
#include "IFMacroMenuAutoPotion.h"
#include "IFMacroMenuAutoSkill.h"
#include "IFMacroMenuAutoHunt.h"
#include "IFMacroMenuPickFilter.h"
#include "IFMacroMenuAutoScrollSlot.h"
//#include "IFMacroAutoPotionSlot.h"

class CIFMacroMenu : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFMacroMenu)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenu)
private:
    CIFMacroMenu(void);
    ~CIFMacroMenu(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    int Func_4(int a2) override;
    void OnUnknownStuff();

    static const int numberOfTabs = 5;
    static const int tabWidth = 120;
    static const int tabHeight = 24;
    static const int tabFirstId = 100;


public:
    void UpdateMenuSize();
    void ActivateTabPage(BYTE page);
    void ApplySavedSlots();
private:
    bool m_savedSlotsApplied;

public:
    CIFSelectableArea** m_pTabs;
    CIFMacroMenuAutoPotion* AutoPotionSlot;
    CIFMacroMenuAutoSkill* AutoSkillSlot;
    CIFMacroMenuAutoHunt* AutoHuntSlot;
    CIFMacroMenuPickFilter* PickupFilterSlot;
    CIFMacroMenuAutoScrollSlot* AutoScrollSlot;
    CIFTextBox* descbox;
};
