#pragma once
#include <IFSelectableArea.h>
#include <IFCheckBox.h>
#include <IFSlotWithHelp.h>
#include <SRIFLib/IFSlotWithHelpEx.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"

class CIFMacro : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFMacro)
GFX_DECLARE_MESSAGE_MAP(CIFMacro)
private:
    CIFMacro(void);
    ~CIFMacro(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void UpdateMenuSize();
    void AutoPotionOnOffBtn();
    void AutoPotionOptionBtn();
    void AutoSkillOnOffBtn();
    void AutoSkillOptionBtn();

    void AutoHuntOnOffBtn();
    void AutoHuntOptionBtn();

    CIFButton* autopotiononoffbtn;
    CIFButton* autoskillonoffbtn;
    CIFButton* autohuntonoffbtn;
    CIFButton* petfilteronoffbtn;
    CIFButton* autoscrollonoffbtn;

    void PetFilterOptionBtn();
    void PetFilterOnOffBtn();
    void AutoScrollOptionBtn();
    void AutoScrollOnOffBtn();
    void SendPacket();


    void GDR_MACRO_AUTO_HUNT_CHECKBOX_STATE(bool b);
void ActivePetFilter();
void ActiveAutoPotion();
void ActiveAutoSkill();
void ActiveAutoHunt();
void ActiveAutoScroll();
};
