//
// Created by YUMBUL on 29.03.2024.
//

#include "CustomCICPlayer.h"

CustomCICPlayer* m_Player;

CustomCICPlayer* CustomCICPlayer::getInstance() {
    if (m_Player == NULL) {
        m_Player = new CustomCICPlayer();
    }
    return m_Player;
}
CustomCICPlayer::CustomCICPlayer(){
    Enabled_AutoStr = false;
    Enabled_AutoInt = false;
    Enabled_AutoMastery = false;
    Selected_AutoMasteryId = 0;
    Enabled_AutoSkill = false;
    Selected_AutoSkillMasteryId = 0;
    Selected_AutoSkillId = 0;

    FirstSpawn = false;

    m_MacroAutoSkillData = std::map<int,Macro_AutoSkillData>();

    PetSkillTimerRunning = false;

    MailAddress = std::n_wstring();

    m_FellowSkillData = std::vector<Fellow_SkillData>();
    ReverseSlot = 9999;

    FocusedId = 0;


    m_MyNewAutoHunting_Enabled = false;
    m_MagicPopTimerRunning = false;
    m_MagicPopSettings = 700;


    m_AchievementsCondition = std::map<int, SAchievementsCondition>();
    m_Achievements = std::map<int, byte>();
}
