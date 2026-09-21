//
// Created by YUMBUL on 15.10.2024.
//

#include "CustomSettingManager.h"

CustomSettingManager* m_Settings;

CustomSettingManager* CustomSettingManager::getInstance() {
    if (m_Settings == NULL) {
        m_Settings = new CustomSettingManager();
    }
    return m_Settings;
}

CustomSettingManager::CustomSettingManager() {
    GrantName = false;
    IconManager = false;
    IconManagerRight = false;
    TitleManager = false;
    TitleManagerColor = false;
    RankingWnd = false;
    UniqueHistoryWnd = false;
    EventRegisterWnd = false;
    EventScheduleWnd = false;
    AchievementsWnd = false;
    SecondarySlot = false;
    OldLogin = false;
    OldExpBar = false;
    OldAlchemy = false;
    MoveSkillBoard = false;
    EnableServerInfoSkill = false;
    EnableOldMainPopup = false;
    HideTitleWhileTagActive = false;
    ItemComparison = false;
    AutoSortButton = false;
    EnableAutoSkill = false;
    MaxMasteryLevel = 0;
    ChineseMasteryLimit = 0;
    EuropeanMasteryLimit = 0;
    ServerMaxLevel = 0;
    EnableAutoStrInt = false;
    EnablePickSoxEffect = false;
    GuildJobMode = false;
    EnableUniqueTarget = false;
    EnableMacro = false;


    StartAutoSort = false;
    AutoSortRunning = false;

    NewCharInfoScreen = false;
    NewIDPWScreen = false;
    ServerName = std::n_string("KMTGuard");
    EnableQuickLogin = true;

    PSTitleIsLoaded = false;

    EnableChangeLog = false;
    ShowChangeLogFirstLogin = false;
    FixNewJobSuit = false;
    EnableOldItemMall = false;
    InsertCommaToPrices = false;
    EnableCharacterBound = false;
    EnableNewItemMall = false;
    ENABLE_EMOJI_SYSTEM = false;
    ENABLE_NEW_PT_MATCH = false;
    ENABLE_NEW_JOB_UI = false;
    ENABLE_NEW_ALCHEMY = false;
    EnableItemTranslation = false;
    ItemTranslationPayment = 0;
    ItemTranslationPrice = 0;
    NonClosePTForm = false;
    EnableLuckySpin = false;
    EnableLuckySpinSilk = true;
    LuckySpinPrice = 0;
    ShowGuideMenu = true;
    ShowGuideLuckySpin = true;
    ShowGuideItemChest = true;
    ShowGuideDropLogs = true;
    ShowGuideMacro = true;
    ShowGuideDailyLogin = true;
    ShowGuideDiscord = true;
    ShowGuideWebsite = true;
    ShowGuideFacebook = true;
    ShowGuideAutoEquip = true;
    AutoEquipMaxLevel = 90;
    ShowGuideWebViewer = true;
    ShowGuideMapLocation = true;
    EnableSpecialOffers = false;
    ShowGuideSpecialOffers = true;
    EnablePvpChallenge = true;
    ShowGuidePvpChallenge = true;
    ShowGuideKillerAnimation = true;
    NewInventoryDesign = true;
    EnableOfflineStall = false;
    MenuLikeMaxi = false;
    MenuCasy = false;
}
