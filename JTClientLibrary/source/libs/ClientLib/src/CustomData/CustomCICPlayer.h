#pragma once
#include <BSLib/_internal/custom_stl.h>
#include <Rpc.h>

class CustomCICPlayer {
private:
    CustomCICPlayer();
public:
    CustomCICPlayer* getInstance();
    bool Enabled_AutoStr;
    bool Enabled_AutoInt;
    bool Enabled_AutoMastery;
    int Selected_AutoMasteryId;
    bool Enabled_AutoSkill;
    int Selected_AutoSkillId;



    bool PetSkillTimerRunning;

    std::n_wstring MailAddress;


    bool FirstSpawn;

    struct Macro_AutoSkillData
    {
        int SlotSeq;
        BYTE SlotType;
        int Data;
    };
    std::map<int,Macro_AutoSkillData> m_MacroAutoSkillData;





    struct Fellow_SkillData
    {
        __int64 ID64;
        byte Enable_Skill_1;
        byte Enable_Skill_2;
        byte Enable_Skill_3;
        byte Enable_Skill_4;
        byte Enable_Skill_5;
    };

    std::vector<Fellow_SkillData> m_FellowSkillData;
    int ReverseSlot;



    int FocusedId;

    bool m_MyNewAutoHunting_Enabled;
    int m_MagicPopSettings;
    bool m_MagicPopTimerRunning;




    std::map<int, byte> m_Achievements;
    struct SAchievementsCondition
    {
        int RefAchievementID;
        int RefAchievementConditionID;
        __int64 ProgressCount;
    };
    std::map<int, SAchievementsCondition> m_AchievementsCondition;

    // Keep newly introduced state after the legacy fields so existing
    // client-side offsets remain stable across translation units.
    int Selected_AutoSkillMasteryId;
};


extern CustomCICPlayer* m_Player;
