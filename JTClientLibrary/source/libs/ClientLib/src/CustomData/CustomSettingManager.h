#pragma once

#include <map>
#include <string>
#include <BSLib/_internal/custom_stl.h>
#include <Rpc.h>

class CustomSettingManager {
private:
    CustomSettingManager();
public:

    // Singleton pattern için getInstance() fonksiyonu ekleyebilirsiniz.
    CustomSettingManager* getInstance();

    byte GrantName;
    byte IconManager;
    byte IconManagerRight;
    byte TitleManager;
    byte TitleManagerColor;

    byte RankingWnd;
    byte UniqueHistoryWnd;
    byte EventRegisterWnd;
    byte EventScheduleWnd;
    byte AchievementsWnd;
    byte SecondarySlot;
    byte OldLogin;
    byte OldExpBar;
    byte OldAlchemy;
    byte MoveSkillBoard;
    byte EnableServerInfoSkill;
    byte EnableOldMainPopup;
    byte HideTitleWhileTagActive;
    byte ItemComparison;
    byte AutoSortButton;
    byte EnablePtMemberViewer;
    byte EnableAutoSkill;
    int MaxMasteryLevel;
    int ChineseMasteryLimit;
    int EuropeanMasteryLimit;
    byte ServerMaxLevel;
    byte EnableAutoStrInt;
    byte EnablePickSoxEffect;
    byte GuildJobMode;
    byte EnableUniqueTarget;
    byte EnableMacro;


    byte StartAutoSort;
    byte AutoSortRunning;
    byte NewCharInfoScreen;
    byte NewIDPWScreen;
    std::n_string ServerName;
    bool EnableQuickLogin;

    byte PSTitleIsLoaded;

    bool EnableChangeLog;
    bool ShowChangeLogFirstLogin;
    bool FixNewJobSuit;
    bool EnableOldItemMall;
    bool InsertCommaToPrices;
    bool EnableCharacterBound;
    bool EnableNewItemMall;

    bool ENABLE_EMOJI_SYSTEM;
    bool ENABLE_NEW_PT_MATCH;
    bool ENABLE_NEW_JOB_UI;
    bool ENABLE_NEW_ALCHEMY;
    bool EnableItemTranslation;
    byte ItemTranslationPayment;
    int ItemTranslationPrice;
    bool NonClosePTForm;
    bool EnableLuckySpin;
    bool EnableLuckySpinSilk;
    int LuckySpinPrice;
    bool ShowGuideMenu;
    bool ShowGuideLuckySpin;
    bool ShowGuideItemChest;
    bool ShowGuideDropLogs;
    bool ShowGuideMacro;
    bool ShowGuideDailyLogin;
    bool ShowGuideDiscord;
    bool ShowGuideWebsite;
    bool ShowGuideFacebook;
    bool ShowGuideAutoEquip;
    int AutoEquipMaxLevel;
    bool ShowGuideWebViewer;
    bool ShowGuideMapLocation;
    bool EnableSpecialOffers;
    bool ShowGuideSpecialOffers;
    bool EnablePvpChallenge;
    bool ShowGuidePvpChallenge;
    bool ShowGuideKillerAnimation;
    bool NewInventoryDesign;
    bool EnableOfflineStall;
    bool MenuLikeMaxi;
    bool MenuCasy;
};

extern CustomSettingManager* m_Settings;
