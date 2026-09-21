#pragma once
#include "IFSkillBoard.h"
#include "IFStatic.h"
#include "IFWnd.h"
#include "Data/SkillData.h"
struct sMasteryData {
    int nMasteryId;
    BYTE btLevel;
};

struct sSkillData {
    int m_skillId;
    int m_enable;
    CSkillData *m_pSkillData;
};

struct SSkillMaps {
    std::map<int, sMasteryData *> m_masteryData;//0x036C
    std::map<int, sSkillData *> m_SkillData;    //0x0378

public:
    bool CanLearnSkill(int skillId);
    int GetCurrentMasteryCount();
    byte test(int a1);
    byte NextSkillLevel_MAYBE(int a1);
    bool SkillIsLearned_MAYBE(int a1);
    CSkillData* GetSkillDataIFSkillActive(int a1);
    void FUN_009c12b0(int a1);

    bool IsLearnedMastery(int skillId);
    undefined4 FUN_00a95dc00(int masteryID);
};

class CIFSkill : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFSkill, 0x00eea73c)
public:
    void ShowGWndIMPL(bool p1);
    void FUN_006a6e60(undefined4 a1);
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
public:
    SSkillMaps sSkillMaps;           //0x036C
    char pad_0x0384[4];             //0x0384
    int ActiveMasterTabID; //0x0388
    int ActiveMasteryID; //0x038C
    CIFStatic *m_skillPointValueText;//0x0390
    char pad_0x0394[0x4];            //0x0394
    CIFSkillBoard *m_skillBoard;     //0x0398
    char pad_0x039C[0x18];           //0x039C


BEGIN_FIXTURE()
        ENSURE_SIZE(0x03B4)

    END_FIXTURE()

    RUN_FIXTURE(CIFSkill)


};