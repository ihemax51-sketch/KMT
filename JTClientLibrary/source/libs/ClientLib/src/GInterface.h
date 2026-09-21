#pragma once

#include <Data/SOItemPackage.h>
#include <Managers/ItemReuseDelayManager.h>
#include "BSLib/BSLib.h"

#include "IFWnd.h"
#include "IFSupporterChatWnd.h"
#include "IFTimerWnd.h"
#include "IFQuickStateWnd.h"
#include "IFQuickStateHalfWnd.h"
#include "NavigationDeadreckon.h"
#include "AlramGuideMgrWnd.h"

#include "IFFade.h"
#include "IFNPCWindow.h"
#include "IFMainPopup.h"
#include "IFSkillPracticeBox.h"
#include "IFHelperBubbleWindow.h"
#include "IFChatModule.h"
#include "MessageBoxManager.h"
#include "AutoPotion.h"

#include "SRIFLib/NInterfaceResource.h"
#include "SRIFLib/NIFGuildWnd.h"
#include "SRIFLib/NIFCommunityWnd.h"
#include "SRIFLib/NIFWorldMap.h"

#include "IFSystemMessage.h"
#include "IFExtQuickSlot.h"

#include "unsorted.h"
#include "IFItemMall.h"
#include "IFItemMallZzim.h"
#include "IFItemMallShopSlot.h"
#include "SkillCoolTimeManager.h"

#define g_pCGInterface (*(CGInterface **) 0x0110F80C)

class CIFDropLogWnd;

/// \todo Extract this hole enum.
enum eGinterfaceID {
    GDR_CHAT_BOARD = 1,                 // CIFChatViewer

    GDR_MINIMAP = 10,                   // CIFMinimap
    GDR_PLAYER_MINI_INFO = 11,          // CIFPlayerMiniInfo
    GDR_UNDERBAR = 12,                  // CIFUnderBar - CNIFUnderBar

    GDR_STORE = 15,                     // CIFStoreForPackage || CIFStore
    GDR_TARGETWINDOW = 16,              // CIFTargetWindow

    GDR_STORAGEROOM = 19,               // CIFStorageRoom
    GDR_NOTICE = 20,                    // CIFNotify

    GDR_MAGICSTATEBOARD = 22,           // CIFMagicStateBoard
    GDR_COMMUNITY = 23,                 // CIFCommunity
    GDR_QUICKPARTYBOARD = 24,           // CIFQuickPartyWnd
    GDR_MAINPOPUP = 25,                 // CIFMainPopup
    GDR_EXCHANGE = 26,                  // CIFExchange
    GDR_LOADING = 27,                   // CIFLoading
    GDR_WORLDMAP = 28,                  // CIFWorldMap
    GDR_NPCWINDOW = 30,                 // CIF_NPCWindow
    GDR_PRACTICEPOPUP = 31,             // CIFSkillPracticeBox
    GDR_GAMEGUIDE = 32,                 // CIFGameGuide
    GDR_STALL = 33,                     // CIFStall
    GDR_EQUIP_DUR_ERROR_WND = 34,       // CIFDurabilityErr
    GDR_WARNING_WND = 35,               // CIFNotify
    GDR_COMPOSITE_ITEM = 36,            // CIFCompositeItemWnd

    GDR_COS_MANAGER = 39,               // CIFCOSManager
    GDR_DELAY_GAUGE_BOARD = 40,         // CIFDelayGaugeBoard
    GDR_EVENTGUIDE = 41,                // CIFEventGuide
    GDR_UPDATE_QUEST_INFO = 42,         // CIFNotify
    GDR_SKILLWITHDRAWAL = 43,           // CIFSkillWithdrawal
    GDR_ALCHEMYBOX = 44,                // CIFAlchemyBox
    GDR_QUESTINFO = 45,                 // CIFQuestInfo
    GDR_QUESTINFO_GUIDE = 46,           // CIFQuestInfoGuide
    GDR_GUILD_NOTIFY_WRITE = 47,        // CIFGuildNotifyWrite

    GDR_GUILD_LEVEL_UP = 49,            // CIFGuildLevelUp
    GDR_ITEM_MALL = 50,                 // CIFItemMall
    GDR_WHISPERLIST = 51,               // CIFWhisperList

    GDR_CHANGE_PLAYER_MODEL = 54,       // CIFChangePlayerModel
    GDR_LETTER_ALARM_GUIDE = 55,        // CIFLetterAlarmGuide
    GDR_WHOLE_CHAT = 56,                // CIFWholeChat
    GDR_GUILDMASTER_ELECTION = 57,      // CIFGuildMasterElection
    GDR_GUILDMASTER_LEAVE = 58,         // CIFGuildMasterLeave
    GDR_REQUEST_GUILDWAR = 59,          // CIFRequestGuildWar

    GDR_CAS_REQUEST_SERVICE = 61,       // CIFCASRequestService
    GDR_CAS_CHAT_VIEW = 62,             // CIFCASChatView
    GDR_JOB_ALIAS = 63,                 // CIFJobAlias
    GDR_JOB_RANK = 64,                  // CIFJobRank
    GDR_JOB_CONTRIBUTION_RANK = 65,     // CIFJobContributionRank
    GDR_PREV_JOB_INFO = 66,             // CIFPrevJobInfo
    GDR_SPECIALTY_DEAL = 67,            // CIFSpecialtyDeal
    GDR_SYSTEM_MESSAGE_VIEW = 68,       // CIFSystemMessage

    GDR_INVENTORY = 70,                 // CIFInventory
    GDR_EQUIPMENT = 71,                 // CIFEquipment
    GDR_PARTY = 72,                     // CIFParty
    GDR_SKILL = 73,                     // CIFSkill
    GDR_ACTION = 74,                    // CIFAction
    GDR_PLAYERINFO = 75,                // CIFPlayerInfo
    GDR_QUEST = 76,                     // CIFQuest or maybe CNIF
    GDR_APPRENTICESHIP = 77,            // CIFApprenticeShip

    GDR_REGION_INFO_VIEW = 83,          // CIFRegionView

    GDR_HELPBUBBLEWND = 97,             // CIFHelperBubbleWindow
    GDR_SYSTEM = 98,                    // CIFSystemWnd
    GDR_SELECTED_ITEM = 99,             // CIFDraggedStatic

    GDR_CHECK_CONFIRM_WND = 101,        // CIFCheckConfirmWnd

    GDR_HELPBUBBLEWNDEXT = 103,         // CIFHelperBubbleWindow

    GDR_COS_WND = 120,                  // CIFCOS

    GDR_PARTY_MATCH = 130,              // CIFPartyMatch
    GDR_PARTY_MATCH_REQ_JOIN = 131,     // CIFPartyMatchReqJoin
    GDR_GUILD_SUMMON_REQP = 132,        // CIFGuildSummonReq
    GDR_EXT_QUICK_SLOT = 133,           // CIFExtQuickSlot
    GDR_EXT_QUICK_SLOT_SECOND = 134,    // CIFExtQuickSlotSecond
    GDR_AUTO_POTION = 135,              // CIFAutoPotion

    GDR_GHACHA = 140,                   // CIFGhaCha

    GDR_GUILDSTORAGEROOM = 145,         // CIFStorageRoom

    GDR_STALL_NETWORK = 150,            // CIFStallNetwork
    GDR_TIMER_SUMMONED_MOB = 151,       // CIFTimerSummonedMob
    GDR_EVENT_FESTIVAL = 152,           // CIFEventGather
    GDR_MENTOR_MATCH = 153,             // CIFMentorMatch
    GDR_APPRENTICESHIP_HONOR_RANK = 154,// CIFApprenticeShipHonorRanking
    GDR_APPRENTICESHIP_JUDGE = 155,     // CIFApprenticeShipJudge
    GDR_GRANT_MAGIC_ATTRIBUTE = 156,    // CIFGrantMagicAttributeWnd
    GDR_EQUIP_STATE_WND = 157,          // CEquipStateImg
    GDR_ALRAM_GUIDE_MGR_WND = 158,      // CAlramGuideMgrWnd
    GDR_TRADE_INFO_WND = 159,           // CIFTradeInfoWnd

    GDR_SUPPORTERCHAT = 161,            // CIFSupporterChatWnd

    GDR_CONFIRMREPUTATION_GUIDE = 165,  // CIFConfirmReputationGuide
    GDR_CONFIRMREPUTATION = 166,        // CNIFConfirmReputation
    GDR_RENEWAL_ITEM_MALL = 167,        // CIFNewItemMall

    GDR_MINI_QUEST = 173,               // CNIFMiniQuestWnd
    GDR_TARGET_MENU = 174,              // CNIFTargetMenu

    GDR_GUILD = 176,                    // CNIFGuildWnd

    GDR_SERVEREVENT_GUIDE = 179,        // CIFServerEventGuide

    GDR_OPENMARKETALRAM_GUIDE = 198,    // CIFOpenMarketAlramGuide

    GDR_STA_BATTERYREMAIN = 201,        // CIFStatic
    GDR_STA_FIRSTNUM = 202,             // CIFStatic
    GDR_STA_SECONDNUM = 203,            // CIFStatic

    GDR_TAX_MANAGEMENT = 300,           // CIFTaxManagement
    GDR_FORTRESS_MAP = 301,             // CIFFortressMap
    GDR_FORTRESS_BUSINESS = 302,        // CIFFortressBusiness
    GDR_FORTRESS_WAR_APPLY_WND = 303,   // CIFFortressWarApplyWnd
    GDR_FORTRESS_MAKEITEM_WND = 304,    // CIFFortressMakeItemWnd

    GDR_GNGWC_RANK = 400,               // CIFGNGWCRank

    GDR_TIMER_WND = 410,                // CIFTimerWnd

    GDR_QUICK_STATE = 420,              // CIFQuickStateWnd
    GDR_QUICK_STATE_HARF = 421,         // CIFQuickStateHalfWnd

    GDR_EVENTGUIDE_SECOND = 30006,      // CIFEventGuideSecond

    // Devkit Examples.....

    GDR_MENU_GUIDE = 13378,         // CIFflorian0Guide // Nice ID
    GDR_CHEST_GUIDE = 13779,
    GDR_FACEBOOK_GUIDE = 13780,
    GDR_DISCORD_GUIDE = 13781,
    GDR_WEB_GUIDE = 13783,
    GDR_DAILY_LOGIN = 13784,
    GDR_MACRO_GUIDE = 13785,
    GDR_ALCHEM_MACRO_GUIDE = 13786,
    GDR_WEBS_GUIDE = 13787,
    GDR_AUTOEQUIP_GUIDE = 13788,
    GDR_LUCKY_SPIN_GUIDE = 13789,
    GDR_SPECIAL_OFFERS_GUIDE = 13790,
    GDR_DROPLOG_OPEN_ICON = 13791,
    GDR_PVP_CHALLENGE_GUIDE = 13792,
    GDR_KILLER_ANIMATION_GUIDE = 13793,
    GDR_EVENT_REGISTER_GUIDE = 13794,



    HP_TIMER = 80001,
    HP_USINGTIMER = 80002,
    MP_TIMER = 80003,
    MP_USINGTIMER = 80004,
    VIGOR_TIMER_HP = 80005,
    VIGOR_USINGTIMER_HP = 80006,
    VIGOR_TIMER_MP = 80007,
    VIGOR_USINGTIMER_MP = 80008,
    PILL_TIMER = 80009,
    PILL_USINGTIMER = 80010,
    PILL_TIMER_PURI = 80012,
    PILL_USINGTIMER_PURI = 80013,
    SPEED_TIMER = 80011,
    PET_HP_TIMER = 80014,
    PET_HP_USINGTIMER = 80015,
    PET_HGP_TIMER =  80016,
    PET_HGP_USINGTIMER =  80017,

    PET_PILL_TIMER =  80018,
    PET_PILL_USINGTIMER =  80019,
    PET_SUMMON_TIMER = 80020,
    PET_RES_TIMER = 80021,

    START_AUTO_SKILL = 80022,
    START_BACK_TOWN = 80023,
    STARTED_INVITE_PLAYER_PARTY = 80024,
    START_AUTO_HUNT = 80025,
    START_AUTO_POTION = 80026,
    START_PICK_PET_TIMER = 90002,
    START_AUTO_SCROLL_TIMER = 90003,

    START_PETT_BUFF_TIMER_1 = 90004,
    SKILL_AUTOMATION_TIMER = 90005,
    ANIMATION_QUEUE_TIMER = 90006,

};

enum eMarketChatType : BYTE {
    UNKNOWN_CHAT, // what deal windows has chat inside it?
    STALL_CHAT,

    MARKET_CHAT_NUM
};

struct SChatMetaData {
    SChatMetaData() {
        chatIndex = -1;
        chatType = 0;
        strRecipient = L"";
        strMessage = L"";
    }

    char chatIndex;
    char chatType;
    // Name of the receiving character, in case of PM, string is empty otherwise.
    std::n_wstring strRecipient;
    // Always the message
    std::n_wstring strMessage;
};

class CGInterface : public CIFWnd {
public:
    enum eTimerIds {
        TIMER_AUTOPOTION_HP = 14,
        TIMER_AUTOPOTION_MP = 15,
        TIMER_AUTOPOTION_CURE_PILL = 16
    };

    enum eSuspendMode : DWORD {
        SUSPEND_MODE_NONE,
        SUSPEND_MODE_TELPORTIONG = 11,
    };

public:
    // int OnKeyDown(int keycode, int a3, char a4);

    /// \details Ithink param_1 is an struct thats cotains nChar
    /// \address 0078b660
    BOOL OnCharIMPL(UINT, UINT, UINT);

    /// \address 0078ae00
    BOOL OnCharInSusspendMode(UINT, UINT, UINT);
    void UsePetSkill_1();
    /// \details Iguess its getting MainpopUps hotkeys
    UINT FUN_00778310(UINT &, UINT &);
    void OnItemMallSectionControl(bool bCreate);
    CSkillCoolTimeManager* GetSkillCoolTimeManager();
    void FUN_0079ac50(int p1,int p2);
    CIFItemMall * GetItemMall();
    void sub_79A620();
    /// \address 0079d620
    bool OnEscapePressed();
    void SetWindowMessageEscapeHandled(bool handled);
    bool ConsumeWindowMessageEscapeHandled();
    undefined IsInteractionBlocked(undefined1 p1);

    bool OnCreateIMPL(long ln);

    void OnTimerIMPL(int timerId);

    void SuspendMacroAutomationForWorldTransition();

    void BeforeOnCreate();

    void AfterOnCreate();

    /// \brief Show a message in the warning area (green)
    ///
    /// \details Show a message in the green area in the center of the client.
    /// The message fades out after a certain amount of time.
    ///
    /// \address 0077b580
    ///
    /// \param msg The message to be displayed
    ///
    /// Example:
    /// \code
    /// g_pCGInterface->ShowMessage_Warning(L"Hello World");
    /// \endcode
    void ShowMessage_Warning(const std::n_wstring &msg);

    /// \brief Show a message in the notice area (red)
    ///
    /// \details Show a message in the red area in the center of the client.
    /// The message fades out after a certain amount of time.
    ///
    /// \address 00777bf0
    ///
    /// \param msg The message to be displayed
    ///
    /// Example:
    /// \code
    /// g_pCGInterface->ShowMessage_Notice(L"Hello World");
    /// \endcode
    void ShowMessage_Notice(const std::n_wstring &msg);

    /// Shows a notice in the native center notification area with matching
    /// custom text and banner colors. Standard notices restore native colors.
    void ShowMessage_ColoredNotice(const std::n_wstring &msg,
                                   unsigned char red,
                                   unsigned char green,
                                   unsigned char blue);

    /// \brief Show a message in the quest area (blue)
    ///
    /// \details Show a message in the blue area in the center of the client.
    /// The message fades out after a certain amount of time.
    ///
    /// \address 0077b5b0
    ///
    /// \param msg The message to be displayed
    ///
    /// Example:
    /// \code
    /// g_pCGInterface->ShowMessage_Quest(L"Hello World");
    /// \endcode
    void ShowMessage_Quest(const std::n_wstring &msg);

    /// \address 007781B0
    void WriteSystemMessage(eLogType btLevel, LPCWSTR lpszText);
    void WriteSystemMessageForHook(eLogType btLevel, LPCWSTR lpszText);

    /// \brief Send given message as global by using item at given slot
    /// \address 007901c0
    /// \todo Make message parameter a const reference once caller is under our control
    void WriteGlobalMessage(unsigned char nSlot, std::n_wstring message);

    void CreateFlorian0Event();

    CAlramGuideMgrWnd *GetAlarmManager();

    /// \address 0079D5B0
    void ToggleActionWnd();

    /// \address 0079B0B0
    void ToggleApprenticeshipWnd();

    /// \address 0079ACE0
    void TogglePlayerInfoWnd();

    /// \address 0079F690
    void ToggleGameGuide();

    /// \address 0079b140
    void ToggleGGuide(bool bVisible);

    /// \address 0079B020
    void ToggleInventoryWnd();

    /// \address 0079ad70
    void ToggleEquipmentWnd();

    /// \address 0079AE90
    void TogglePartyWnd();

    /// \address 0079AE00
    void ToggleSkillWnd();

    /// \address 0079af20
    void ShowInventoryWnd();

    /// \address 0079af70
    void ShowApprenticeshipWnd();

    /// \address 0079afd0
    void ToggleMainPopup();

    /// \address 0079ABE0
    void ToggleWorldMap();

    /// \address 0079C750
    void ToggleAutoPotion(bool bVisible);

    /// \address 00778B70
    int Get_SelectedObjectId();

    /// \address 00777A30
    CIFTimerWnd *Get_TimerWindow();

    /// \address 00777A50
    CIFQuickStateHalfWnd *Get_QuickStateHalfWnd();

    /// \address 00777a70
    void FUN_00777a70(undefined4 param_1, undefined4 param_2);

    /// \address 00798D40
    CIF_NPCWindow *Get_GDR_NPCWINDOW();

    /// \address 00798D00
    CIFMainPopup *GetMainPopup();

    /// \address 007994C0
    CIFStorageRoom *Get_GDR_STORAGEROOM();

    /// \address 00799640
    CIFStorageRoom *Get_GDR_GUILDSTORAGEROOM();

    /// \address 007990e0
    void ToggleQuestNew();

    /// \address 007994f0
    CNIFCommunityWnd *GetCommunityWnd();

    CIFSystemMessage *GetSystemMessageView();

    /// \address 00777c30
    void FUN_00777c30(ChatType type, const wchar_t *message, D3DCOLOR color, int a5);

    /// \address 00778a10
    void WriteInMarketChatMsg(BYTE btChatType, LPCWSTR lpszMsg, D3DCOLOR dwColor);

    /// \address 00777cf0
    void AddTargetToWhisperList(std::n_wstring &recipient);
    CNIFWorldMap *GetCNIFWorldMap();
    void ShowLogMessage(int color, const wchar_t* msg);
    CIFDropLogWnd* EnsurePickInventoryWindow();
    bool TogglePickInventoryWindow();
    bool TryUseSecondarySlotHotkey(int keycode);
    int OnKeyDown(int keycode, int a3, int a4);
    void CreateCustomMessageBox(int ID, int MasteryID);
    CItemReuseDelayManager* GetItemReuseDelayManager();
    int Wtf();
    void UnLockMovement();
    void LockMovement(int param);


    void Set_SelectedObjectId(int p);

    /// \address 00778190
    void WriteErrorMessage(byte errorType, unsigned __int16 errorCode, int colorARGB, int a5, int a6);

    /// \address 0078BEA0
    int TryParseChatCommands(const wchar_t *text, RECT &r, std::vector<void *> &v);

    /// \address 00787C10
    void sub_787C10(SChatMetaData &meta);

    /// \address 0079b8a0
    void FUN_0079b8a0(undefined1 a1, undefined4 a2);

    /// \address 0078f4e0
    void RequestStatIncrement(undefined4 a1, undefined4 a2, undefined4 a3, undefined4 a4);

    /// \address 007994e0
    CNIFUnderMenuBar *GetUnderMenuBar();

    /// \address 00799840
    CIFExtQuickSlot *GetExtQuickSlot();

    /// \address 00799920
    CNIFWorldMap *GetWorldMap();

    /// \address 0079a840
    void SaveGWndPostion(DWORD dwID, wnd_pos sWndPos);
    void CallNIFEnchantWnd(char param_1);
private:
    /// \address 0079a7e0
    void FUN_0079a7e0(CGWndBase *pGWnd) const;

    /// \address 0079ab80
    /// same but difffffffreeeent
    void FUN_0079ab80(CNIFWnd *pWnd);

    /// \address 00798c50
    void SetOnMiddleOfScreen(DWORD dwNInterfaceID);

    /// \address 0079ac50
    void SaveGWndPosOnMiddleOfScreen(DWORD dwID, CGWnd *pParent);
public:
    void Fun_00777f00();
private:
public:
    CIRMManager m_IRM;                       //0x036C
    char pad_0x039C[0xC];                    //0x039C
    CIFSupporterChatWnd *m_academyChatWindow;//0x03A8
    CIFTimerWnd *m_timerWindow;              //0x03AC
    CIFQuickStateWnd *N00002636;             //0x03B0
    CIFQuickStateHalfWnd *N00002637;         //0x03B4
    char pad_0x03B8[0xC];                    //0x03B8
private:
    int last_mouse_x; //0x03C4 3C4
    int last_mouse_y; //0x03C8
    char pad_03CC[1]; //0x03CC
    char b_blockWASD; //0x03CD
    char b_freeCameraMode; //0x03CE
    char pad_03CF[45]; //0x03CF
    class CCameraWorking *m_pCameraWorking; //0x03FC
    class CIFCameraDataWnd *m_pCameraDataWnd; //0x0400
    class CIFCameraDataMainWnd *m_pCameraDataMainWnd; //0x0404
    char pad_0x0408[0xA4]; //0x0408
public:
    int m_silk_own; //0x04AC
    int m_silk_gift; //0x04B0
    int m_silk_point; //0x04B4
    std::list<SReservedItemMallData> m_lstReservedItemMallData; //0x04B8
private:
    char pad_04C4[0x40]; //0x04C4
    DWORD N0000268B; //0x0504
public:
    int AAAA;
    int TO166;
    char pad_0510[280]; //0x0510
    int m_selectedObjectId; //0x0628
    char pad_0x062C[0x80]; //0x062C
public:
    CNavigationDeadreckon m_Nav;//0x06AC
private:
    CAutoPotion *m_pAutoPotion; //0x06FC
    char pad_0x0700[0x20]; //0x0700
    CIFFade *m_screenCover; //0x0720
    CIF_NPCWindow *m_npcWindow; //0x0724
    CIFMainPopup *m_mainPopup; //0x0728
    CIFSkillPracticeBox *m_skillUpWindow; //0x072C
public:
    CIFHelperBubbleWindow *m_helperWindow; //0x0730
private:
    CIFHelperBubbleWindow *N00002717; //0x0734
    char pad_0738[8]; //0x0738
    CIFChatModule *m_pMarketChatModule[MARKET_CHAT_NUM]; //0x0740
    CMessageBoxManager m_messageBoxManager; //0x0748
    class CMessageTipManager *m_pCMessageTipManager; //0x0774 774
    class CSkillCoolTimeManager *m_pCSkillCoolTimeManager; //0x0778
    CItemReuseDelayManager *m_pCItemReuseDelayManager; //0x077C
    class CSkillRunTimeManager *m_pCSkillRunTimeManager; //0x0780
    char pad_0784[0x07BA - 0x0784]; //0x0784
    bool m_bDisappeard; //0x07BA i hope it's called like zat
    char pad_07bb[0x07CD - 0x7BB]; //0x07BB
public:
    undefined1 field_0x7cd;

private:
    char pad_07d0[0x07EC - 0x07D0]; //0x07D0
    eSuspendMode m_SuspendMode; //0x07EC // kyuubi09: plz make more research about zat brOzaer becuase im lazy fox.
    char pad_07F0[4]; //0x07F0
    class CPreUseItem *m_pCPreUseItem; //0x07F4
    char pad_07F8[92]; //0x07F8


BEGIN_FIXTURE()
        ENSURE_SIZE(0x854)
        ENSURE_OFFSET(m_lstReservedItemMallData, 0x04B8)
        ENSURE_OFFSET(m_pMarketChatModule, 0x740)
        ENSURE_OFFSET(m_pCSkillRunTimeManager, 0x780)
        ENSURE_OFFSET(m_bDisappeard, 0x07BA)
        ENSURE_OFFSET(field_0x7cd, 0x7cd)
        ENSURE_OFFSET(m_SuspendMode, 0x07EC)
        ENSURE_OFFSET(m_pCPreUseItem, 0x7f4)
    END_FIXTURE()

    RUN_FIXTURE(CGInterface)

IDirect3DBaseTexture9* CreateTexture(std::string path);
};
