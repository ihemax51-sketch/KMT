#pragma once

#include "IFFrame.h"
#include "IFScrollManager.h"
#include "IFSlotWithHelp.h"

class CIFSkillBoard : public CIFFrame
{/*

GFX_DECLARE_MESSAGE_MAP(CIFSkillBoard)
public:
    GFX_DECLARE_DYNCREATE_FN(CIFSkillBoard)
*/
//GFX_DECLARE_DYNAMIC_EXISTING(CIFSkillBoard, 0x00eea61c)
public:
    CIFSkillBoard();
    ~CIFSkillBoard();
    bool OnCreateIMPL(long ln);
    void OnUpdate() override;
    int OnCloseWndIMPL();
    void OnTimerIMPL(int timerId);
    void SelectMastery(int p1, int p2);
    GFX_MSGMAP* MessageMap();
    void OnClickAutoMastery();
    void OnClickAutoSkill();
    void OnBtnClick();
    void EnableAutoMastery(int MasteryID);
    void EnableAutoSkill(int MasteryID);
    void SyncAutoControls();
public:
    void* skillGroupMap;                       //0x07B0
    int SelectedMasteryID1;              //0x07B8
    int SelectedMasteryID;              //0x07B8

    byte SelectedMasteryLevel;           //0x07BC
    byte N00003128;                      //0x07BD
    byte N0000312B;                      //0x07BE
    byte N00003129;                      //0x07BF
    void *CMasteryData;                     //0x07C0
    CIFScrollManager *m_scrollManager;//0x07C4
    CIFSlotWithHelp *m_masteryIcon; //0x07C8
    CIFStatic *m_masteryNameText;     //0x07CC
    CIFStatic *m_masteryLevelText;    //0x07D0
    CIFButton *m_masteryLevelUpButton;//0x07D4
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(2008)
        ENSURE_OFFSET(skillGroupMap, 0x07B0)
        //  ENSURE_OFFSET(SelectedMasteryLevel, 0x07BC)
        ENSURE_OFFSET(CMasteryData, 0x07C0)

    END_FIXTURE()

    RUN_FIXTURE(CIFSkillBoard)



}; //Size=0x07D8
