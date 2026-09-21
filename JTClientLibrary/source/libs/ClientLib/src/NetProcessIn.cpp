#include "NetProcessIn.h"
#include "unsorted.h"
#include "ICPlayer.h"
#include <BSLib/multibyte.h>
#include <BSLib/Debug.h>
#include <Menu/IFTitleManager.h>
#include <sstream>
#include <CustomData/CustomDataManager.h>
#include <CustomInterface/IFLuckySpinWnd.h>
#include <CustomInterface/IFSpecialOffersWnd.h>
#include <CustomInterface/IFKillerAnimationWnd.h>
#include "KillerAnimationPlayer.h"
#include <CustomInterface/IFPvpChallengeWnd.h>
#include <CustomInterface/IFTradeCaptchaWnd.h>
#include <CustomInterface/IFOfflineStall.h>
#include <CustomInterface/IFDropLogWnd.h>
#include <CustomInterface/IFDropLogsWnd.h>
#include <Menu/IFIconManager.h>
#include <Menu/IFDynamicRanking.h>
#include <Menu/IFUniqueHistory.h>
#include <Menu/IFEventRegister.h>
#include <Menu/IFEventSchedule.h>
#include <CustomData/CustomSettingManager.h>
#include <Guides/IFChest.h>
#include <Menu/IFAchievements.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomInterface/IFPartyMemberViewer.h>
#include <CustomInterface/IFSavedLocation.h>
#include <NewItemMall/IFVItemMall.h>
#include <DailyLogin/IFDailyLogin.h>
#include <WebViewer/WebViewerConfig.h>
#include <CustomInterface/IFSoxEffect.h>
#include <Macro/IFMacroMenu.h>
#include <Macro/IFMacro.h>
#include <MacroAlchemy/IFAlchemyMacro.h>
#include <GlobalItemLinking/GlobalItemLinking.h>
#include <CustomInterface/IFKillCounter.h>
#include <Menu/IFChangelog.h>
#include <CustomInterface/IFCustomMessageBox.h>
#include <Menu/IFMenu.h>
#include <Menu/IFGrantName.h>
#include <Social/IFSocial.h>
#include <NewItemMall/IFVSelectMall.h>
#include <NewItemMall/IFVAvatarMallBuyItemList.h>
#include <SecondPW/IFSecondaryPassword.h>
#include "SkillAutomationController.h"

extern bool g_bCurrentStallUsesSilk;
#include <NewItemMall/IFVItemMallBuyItem.h>
#include <ctime>
#include <CustomInterface/IFTeamCounter.h>
#include <CustomInterface/IFJobCounter.h>
#include <LockItems/IFItemLocker.h>
#include <LockItems/IFItemUnlocker.h>
#include <LockItems/IFNewMsgBox.h>
#include <CustomInterface/IFFortressWar.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include <ExtraUI/IFItemTranslationWnd.h>
#include <DiscordRichPresence/DiscordManager.h>
#include <ExtraUI/IFTargetPlayerEquip.h>
#include <ExtraUI/IFMSFPS.h>
#include <ExtraUI/IFSettings.h>
#include <ExtraUI/IFCounterWnd.h>
#include "CharacterDependentData.h"
#include "TextStringManager.h"
#include "Game.h"
#include "PSMission.h"
#include "GInterface.h"
#include "GlobalDataManager.h"
#include "GlobalDataManager.h"
#include "IFMagicStateBoard.h"
#include "CustomInterface/IFDps.h"
#include "IFChatViewer.h"
#include "ICMonster.h"
#include "ICCos.h"
#include "IFTargetWindow.h"
#ifdef CONFIG_DEBUG_NET_RECEIVE
#define DEBUG_PRINT_CALL() printf("%s\n", __FUNCTION__);
#else
#define DEBUG_PRINT_CALL()
#endif

namespace {
struct CMsgStreamReadState {
    explicit CMsgStreamReadState(const CMsgStreamBuffer &msg)
        : currentReadBytes(msg.m_currentReadBytes),
          field0C(msg.field_0xc),
          field0D(msg.field_0xd),
          field0E(msg.field_0xe),
          field0F(msg.field_0xf),
          node1(msg.m_node1),
          node2(msg.m_node2) {
    }

    void Restore(CMsgStreamBuffer &msg) const {
        msg.m_currentReadBytes = currentReadBytes;
        msg.field_0xc = field0C;
        msg.field_0xd = field0D;
        msg.field_0xe = field0E;
        msg.field_0xf = field0F;
        msg.m_node1 = node1;
        msg.m_node2 = node2;
    }

private:
    unsigned int currentReadBytes;
    unsigned char field0C;
    unsigned char field0D;
    unsigned char field0E;
    unsigned char field0F;
    CMsgStreamBuffer::SMsgStreamNode *node1;
    CMsgStreamBuffer::SMsgStreamNode *node2;
};
}
#define DEFAULT_SYSTEM_COLOR 0xFFDBC99B

CNetProcessIn* p_CNetProcessIn = NULL;

namespace {
    int GetEventSuitClothIdForCurrentPlayer(const CustomDataManager::SEventMapSettings &settings)
    {
        if (!g_pMyPlayerObj)
            return -1;

        const int cape = g_pMyPlayerObj->m_PVPCapeState;
        if (settings.RegionType == 0)
        {
            if (cape == 1)
                return 3;
            if (cape == 3 || cape == 4 || cape == 5)
                return 4;
            return 0;
        }

        if (cape == 1)
            return 3;
        if (cape == 3)
            return 4;
        return 0;
    }

    void ApplyEventSuitToCurrentPlayer()
    {
    }

    void RefreshVisibleTargetHpPercent()
    {
        if (g_pCGInterface == NULL)
            return;

        CIFTargetWindow* targetWindow = g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(GDR_TARGETWINDOW, 1);
        if (targetWindow == NULL || !targetWindow->IsVisible())
            return;

        if (targetWindow->m_pGDR_TW_COMMONENEMY != NULL && targetWindow->m_pGDR_TW_COMMONENEMY->IsVisible())
            targetWindow->m_pGDR_TW_COMMONENEMY->RefreshHpPercentText();

        if (targetWindow->m_pGDR_TW_SPECIALMOBWND != NULL && targetWindow->m_pGDR_TW_SPECIALMOBWND->IsVisible())
            targetWindow->m_pGDR_TW_SPECIALMOBWND->RefreshHpPercentText();
    }

    void RefreshVisibleTargetHpPercentForObjectHp(unsigned int objectId, unsigned int currentHp)
    {
        if (g_pCGInterface == NULL)
            return;

        CIFTargetWindow* targetWindow = g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(GDR_TARGETWINDOW, 1);
        if (targetWindow == NULL || !targetWindow->IsVisible())
            return;

        bool refreshed = false;
        if (targetWindow->m_pGDR_TW_COMMONENEMY != NULL && targetWindow->m_pGDR_TW_COMMONENEMY->IsVisible())
            refreshed = targetWindow->m_pGDR_TW_COMMONENEMY->RefreshHpPercentTextForObjectHp(objectId, currentHp) || refreshed;

        if (targetWindow->m_pGDR_TW_SPECIALMOBWND != NULL && targetWindow->m_pGDR_TW_SPECIALMOBWND->IsVisible())
            refreshed = targetWindow->m_pGDR_TW_SPECIALMOBWND->RefreshHpPercentTextForObjectHp(objectId, currentHp) || refreshed;

        if (!refreshed)
            RefreshVisibleTargetHpPercent();
    }

    CIFMacroMenu* GetReadyMacroMenu()
    {
        if (g_pCGInterface == NULL)
            return NULL;

        CIFMacroMenu* macroMenu = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
        if (macroMenu == NULL ||
            macroMenu->AutoPotionSlot == NULL ||
            macroMenu->AutoSkillSlot == NULL ||
            macroMenu->AutoHuntSlot == NULL ||
            macroMenu->PickupFilterSlot == NULL ||
            macroMenu->AutoScrollSlot == NULL)
            return NULL;

        return macroMenu;
    }
}

void StringReplaceAll(std::n_wstring &Value, const std::wstring &From, const std::wstring &To) {
    if (From.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = Value.find(From.c_str(), start_pos)) != std::wstring::npos) {
        Value.replace(start_pos, From.length(), To.c_str());
        start_pos += To.length();// Increase the same length in case 'To' contains 'From', like replacing 'x' with 'yx'
    }
}


std::vector<std::n_wstring> wsplit(const std::n_wstring &InputString, wchar_t delimiterChar) {
    std::vector<std::n_wstring> ResultVec;
    size_t start = 0;
    size_t end = InputString.find(delimiterChar);

    while (end != std::n_wstring::npos) {
        ResultVec.push_back(InputString.substr(start, end - start));
        start = end + 1;
        end = InputString.find(delimiterChar, start);
    }
    ResultVec.push_back(InputString.substr(start));

    return ResultVec;
}


void CNetProcessIn::RegisterPacketHandlers() {
    p_CNetProcessIn = this;
    BS_DEBUG("Registering Handlers");
    m_handlers.insert(std::make_pair(0x3027, &CNetProcessIn::OnUpdateGameTime));
    m_handlers.insert(std::make_pair(0x3809, &CNetProcessIn::On3809));
    m_handlers.insert(std::make_pair(0x3055, &CNetProcessIn::OnResetClient));
    m_handlers.insert(std::make_pair(0xb005, &CNetProcessIn::OnLogoutAck));
    m_handlers.insert(std::make_pair(0xb006, &CNetProcessIn::Onb006));
    m_handlers.insert(std::make_pair(0x300a, &CNetProcessIn::On300a));
    m_handlers.insert(std::make_pair(0x3405, &CNetProcessIn::On3405));
    m_handlers.insert(std::make_pair(0x34b5, &CNetProcessIn::On34b5));
    m_handlers.insert(std::make_pair(0x300c, &CNetProcessIn::On300c));
    m_handlers.insert(std::make_pair(0x34a5, &CNetProcessIn::On34a5));
    m_handlers.insert(std::make_pair(0x3013, &CNetProcessIn::OnMyCharacterData));
    m_handlers.insert(std::make_pair(0x34a6, &CNetProcessIn::On34a6));
    m_handlers.insert(std::make_pair(0x3020, &CNetProcessIn::On3020));
    m_handlers.insert(std::make_pair(0x3017, &CNetProcessIn::OnObjectListBegin));
    m_handlers.insert(std::make_pair(0x3019, &CNetProcessIn::OnObjectListData));
    m_handlers.insert(std::make_pair(0x3018, &CNetProcessIn::OnObjectListEnd));
    m_handlers.insert(std::make_pair(0x3047, &CNetProcessIn::OnChestDataBegin));
    m_handlers.insert(std::make_pair(0x3049, &CNetProcessIn::OnChestDataData));
    m_handlers.insert(std::make_pair(0x3048, &CNetProcessIn::OnChestDataEnd));
    m_handlers.insert(std::make_pair(0x3015, &CNetProcessIn::OnObjectEntered));
    m_handlers.insert(std::make_pair(0x3016, &CNetProcessIn::OnObjectLeft));
    m_handlers.insert(std::make_pair(0x3076, &CNetProcessIn::On3076));
    m_handlers.insert(std::make_pair(0x30bf, &CNetProcessIn::On30bf));
    m_handlers.insert(std::make_pair(0xb0cb, &CNetProcessIn::Onb0cb));
    m_handlers.insert(std::make_pair(0xb03c, &CNetProcessIn::Onb03c));
    m_handlers.insert(std::make_pair(0x3092, &CNetProcessIn::On3092));
    m_handlers.insert(std::make_pair(0x30df, &CNetProcessIn::On30df));
    m_handlers.insert(std::make_pair(0x30cd, &CNetProcessIn::On30cd));
    m_handlers.insert(std::make_pair(0x30ce, &CNetProcessIn::On30ce));
    m_handlers.insert(std::make_pair(0x30d3, &CNetProcessIn::On30d3));
    m_handlers.insert(std::make_pair(0x30C1, &CNetProcessIn::On30C1));
    m_handlers.insert(std::make_pair(0x30C2, &CNetProcessIn::On30C2));
    m_handlers.insert(std::make_pair(0x30C3, &CNetProcessIn::On30C3));
    m_handlers.insert(std::make_pair(0x30C8, &CNetProcessIn::On30C8));
    m_handlers.insert(std::make_pair(0xB0C6, &CNetProcessIn::OnB0C6));
    m_handlers.insert(std::make_pair(0x30C9, &CNetProcessIn::On30C9));
    m_handlers.insert(std::make_pair(0x30E7, &CNetProcessIn::On30E7));
    m_handlers.insert(std::make_pair(0xB0C7, &CNetProcessIn::OnB0C7));
    m_handlers.insert(std::make_pair(0x30CA, &CNetProcessIn::On30CA));
    m_handlers.insert(std::make_pair(0xB116, &CNetProcessIn::OnB116));
    m_handlers.insert(std::make_pair(0xB117, &CNetProcessIn::OnB117));
    m_handlers.insert(std::make_pair(0xB420, &CNetProcessIn::OnB420));
    m_handlers.insert(std::make_pair(0x30D4, &CNetProcessIn::On30D4));
    m_handlers.insert(std::make_pair(0x3054, &CNetProcessIn::On3054));
    m_handlers.insert(std::make_pair(0x3056, &CNetProcessIn::On3056));
    m_handlers.insert(std::make_pair(0x304E, &CNetProcessIn::On304E));
    m_handlers.insert(std::make_pair(0x3052, &CNetProcessIn::On3052));
    m_handlers.insert(std::make_pair(0x3057, &CNetProcessIn::On3057));
    m_handlers.insert(std::make_pair(0xB045, &CNetProcessIn::OnB045));
    m_handlers.insert(std::make_pair(0x303D, &CNetProcessIn::On303D));
    m_handlers.insert(std::make_pair(0x30D2, &CNetProcessIn::On30D2));
    m_handlers.insert(std::make_pair(0x30CF, &CNetProcessIn::On30CF));
    m_handlers.insert(std::make_pair(0x3011, &CNetProcessIn::On3011));
    m_handlers.insert(std::make_pair(0x3058, &CNetProcessIn::On3058));
    m_handlers.insert(std::make_pair(0xB025, &CNetProcessIn::OnB025));
    m_handlers.insert(std::make_pair(0x3026, &CNetProcessIn::OnChatReceive));
    m_handlers.insert(std::make_pair(0x3091, &CNetProcessIn::On3091));
    m_handlers.insert(std::make_pair(0x302D, &CNetProcessIn::On302D));
    m_handlers.insert(std::make_pair(0xB021, &CNetProcessIn::OnB021));
    m_handlers.insert(std::make_pair(0xB024, &CNetProcessIn::OnB024));
    m_handlers.insert(std::make_pair(0xB023, &CNetProcessIn::OnB023));
    m_handlers.insert(std::make_pair(0x3028, &CNetProcessIn::On3028));
    m_handlers.insert(std::make_pair(0xB04F, &CNetProcessIn::OnB04F));
    m_handlers.insert(std::make_pair(0x30D0, &CNetProcessIn::On30D0));
    m_handlers.insert(std::make_pair(0x3200, &CNetProcessIn::On3200));
    m_handlers.insert(std::make_pair(0xB070, &CNetProcessIn::OnB070));
    m_handlers.insert(std::make_pair(0xB071, &CNetProcessIn::OnB071));
    m_handlers.insert(std::make_pair(0xB0BD, &CNetProcessIn::OnB0BD));
    m_handlers.insert(std::make_pair(0x330F, &CNetProcessIn::On330F));
    m_handlers.insert(std::make_pair(0xB0BE, &CNetProcessIn::OnB0BE));
    m_handlers.insert(std::make_pair(0xB0BC, &CNetProcessIn::OnB0BC));
    m_handlers.insert(std::make_pair(0xB072, &CNetProcessIn::OnB072));
    m_handlers.insert(std::make_pair(0x30A6, &CNetProcessIn::On30A6));
    m_handlers.insert(std::make_pair(0x30D1, &CNetProcessIn::On30D1));
    m_handlers.insert(std::make_pair(0x3206, &CNetProcessIn::On3206));
    m_handlers.insert(std::make_pair(0xB0A1, &CNetProcessIn::OnB0A1));
    m_handlers.insert(std::make_pair(0xB0A2, &CNetProcessIn::OnB0A2));
    m_handlers.insert(std::make_pair(0xB203, &CNetProcessIn::OnB203));
    m_handlers.insert(std::make_pair(0xB202, &CNetProcessIn::OnB202));
    m_handlers.insert(std::make_pair(0xB034, &CNetProcessIn::OnB034));
    m_handlers.insert(std::make_pair(0x3038, &CNetProcessIn::On3038));
    m_handlers.insert(std::make_pair(0x3039, &CNetProcessIn::On3039));
    m_handlers.insert(std::make_pair(0x3036, &CNetProcessIn::On3036));
    m_handlers.insert(std::make_pair(0xB03E, &CNetProcessIn::OnB03E));
    m_handlers.insert(std::make_pair(0xB03F, &CNetProcessIn::OnB03F));
    m_handlers.insert(std::make_pair(0x304D, &CNetProcessIn::On304D));
    m_handlers.insert(std::make_pair(0xB04C, &CNetProcessIn::OnB04C));
    m_handlers.insert(std::make_pair(0x3068, &CNetProcessIn::On3068));
    m_handlers.insert(std::make_pair(0x305C, &CNetProcessIn::On305C));
    m_handlers.insert(std::make_pair(0x30E0, &CNetProcessIn::On30E0));
    m_handlers.insert(std::make_pair(0x3040, &CNetProcessIn::On3040));
    m_handlers.insert(std::make_pair(0x3201, &CNetProcessIn::On3201));
    m_handlers.insert(std::make_pair(0x3041, &CNetProcessIn::On3041));
    m_handlers.insert(std::make_pair(0x3042, &CNetProcessIn::On3042));
    m_handlers.insert(std::make_pair(0x3153, &CNetProcessIn::On3153));
    m_handlers.insert(std::make_pair(0x3154, &CNetProcessIn::On3154));
    m_handlers.insert(std::make_pair(0xB046, &CNetProcessIn::OnB046));
    m_handlers.insert(std::make_pair(0xB04B, &CNetProcessIn::OnB04B));
    m_handlers.insert(std::make_pair(0xB0C0, &CNetProcessIn::OnB0C0));
    m_handlers.insert(std::make_pair(0xB050, &CNetProcessIn::OnB050));
    m_handlers.insert(std::make_pair(0xB051, &CNetProcessIn::OnB051));
    m_handlers.insert(std::make_pair(0xB0C5, &CNetProcessIn::OnB0C5));
    m_handlers.insert(std::make_pair(0xB059, &CNetProcessIn::OnB059));
    m_handlers.insert(std::make_pair(0xB05A, &CNetProcessIn::OnB05A));
    m_handlers.insert(std::make_pair(0xB074, &CNetProcessIn::OnB074));
    m_handlers.insert(std::make_pair(0x30EB, &CNetProcessIn::On30EB));
    m_handlers.insert(std::make_pair(0xB0EA, &CNetProcessIn::OnB0EA));
    m_handlers.insert(std::make_pair(0xB05B, &CNetProcessIn::OnB05B));
    m_handlers.insert(std::make_pair(0x34B1, &CNetProcessIn::On34B1));
    m_handlers.insert(std::make_pair(0xB060, &CNetProcessIn::OnB060));
    m_handlers.insert(std::make_pair(0x3864, &CNetProcessIn::On3864));
    m_handlers.insert(std::make_pair(0x3065, &CNetProcessIn::On3065));
    m_handlers.insert(std::make_pair(0xB062, &CNetProcessIn::OnB062));
    m_handlers.insert(std::make_pair(0xB067, &CNetProcessIn::OnB067));
    m_handlers.insert(std::make_pair(0xB069, &CNetProcessIn::OnB069));
    m_handlers.insert(std::make_pair(0xB06A, &CNetProcessIn::OnB06A));
    m_handlers.insert(std::make_pair(0xB06B, &CNetProcessIn::OnB06B));
    m_handlers.insert(std::make_pair(0xB06C, &CNetProcessIn::OnB06C));
    m_handlers.insert(std::make_pair(0x706D, &CNetProcessIn::On706D));
    m_handlers.insert(std::make_pair(0xB06D, &CNetProcessIn::OnB06D));
    m_handlers.insert(std::make_pair(0xB081, &CNetProcessIn::OnB081));
    m_handlers.insert(std::make_pair(0x3080, &CNetProcessIn::On3080));
    m_handlers.insert(std::make_pair(0x3085, &CNetProcessIn::On3085));
    m_handlers.insert(std::make_pair(0xB082, &CNetProcessIn::OnB082));
    m_handlers.insert(std::make_pair(0xB083, &CNetProcessIn::OnB083));
    m_handlers.insert(std::make_pair(0xB084, &CNetProcessIn::OnB084));
    m_handlers.insert(std::make_pair(0x3089, &CNetProcessIn::On3089));
    m_handlers.insert(std::make_pair(0x3086, &CNetProcessIn::On3086));
    m_handlers.insert(std::make_pair(0x3087, &CNetProcessIn::On3087));
    m_handlers.insert(std::make_pair(0x3088, &CNetProcessIn::On3088));
    m_handlers.insert(std::make_pair(0x30E8, &CNetProcessIn::On30E8));
    m_handlers.insert(std::make_pair(0x308C, &CNetProcessIn::On308C));
    m_handlers.insert(std::make_pair(0xB0B1, &CNetProcessIn::OnB0B1));
    m_handlers.insert(std::make_pair(0x30B8, &CNetProcessIn::On30B8));
    m_handlers.insert(std::make_pair(0xB0B2, &CNetProcessIn::OnB0B2));
    m_handlers.insert(std::make_pair(0x30B9, &CNetProcessIn::On30B9));
    m_handlers.insert(std::make_pair(0x30B7, &CNetProcessIn::On30B7));
    m_handlers.insert(std::make_pair(0xB0B3, &CNetProcessIn::OnB0B3));
    m_handlers.insert(std::make_pair(0xB0B5, &CNetProcessIn::OnB0B5));
    m_handlers.insert(std::make_pair(0xB0B4, &CNetProcessIn::OnB0B4));
    m_handlers.insert(std::make_pair(0xB0BA, &CNetProcessIn::OnB0BA));
    m_handlers.insert(std::make_pair(0x30BB, &CNetProcessIn::On30BB));
    m_handlers.insert(std::make_pair(0xB0A7, &CNetProcessIn::OnB0A7));
    m_handlers.insert(std::make_pair(0xB010, &CNetProcessIn::OnB010));
    m_handlers.insert(std::make_pair(0x30D6, &CNetProcessIn::On30D6));
    m_handlers.insert(std::make_pair(0x30D7, &CNetProcessIn::On30D7));
    m_handlers.insert(std::make_pair(0x30D5, &CNetProcessIn::On30D5));
    m_handlers.insert(std::make_pair(0xB0D8, &CNetProcessIn::OnB0D8));
    m_handlers.insert(std::make_pair(0xB0D9, &CNetProcessIn::OnB0D9));
    m_handlers.insert(std::make_pair(0x30EC, &CNetProcessIn::On30EC));
    m_handlers.insert(std::make_pair(0x30DC, &CNetProcessIn::On30DC));
    m_handlers.insert(std::make_pair(0xB515, &CNetProcessIn::OnB515));
    m_handlers.insert(std::make_pair(0x3514, &CNetProcessIn::On3514));
    m_handlers.insert(std::make_pair(0x3CA2, &CNetProcessIn::On3CA2));
    m_handlers.insert(std::make_pair(0x30E6, &CNetProcessIn::On30E6));
    m_handlers.insert(std::make_pair(0xB0E1, &CNetProcessIn::OnB0E1));
    m_handlers.insert(std::make_pair(0xB0E2, &CNetProcessIn::OnB0E2));
    m_handlers.insert(std::make_pair(0xB0E3, &CNetProcessIn::OnB0E3));
    m_handlers.insert(std::make_pair(0xB0E4, &CNetProcessIn::OnB0E4));
    m_handlers.insert(std::make_pair(0xB0E5, &CNetProcessIn::OnB0E5));
    m_handlers.insert(std::make_pair(0xB0E6, &CNetProcessIn::OnB0E6));
    m_handlers.insert(std::make_pair(0xB0F0, &CNetProcessIn::OnB0F0));
    m_handlers.insert(std::make_pair(0x38F5, &CNetProcessIn::On38F5));
    m_handlers.insert(std::make_pair(0xB0F1, &CNetProcessIn::OnB0F1));
    m_handlers.insert(std::make_pair(0xB0F8, &CNetProcessIn::OnB0F8));
    m_handlers.insert(std::make_pair(0xB0F3, &CNetProcessIn::OnB0F3));
    m_handlers.insert(std::make_pair(0x30FF, &CNetProcessIn::On30FF));
    m_handlers.insert(std::make_pair(0x3100, &CNetProcessIn::On3100));
    m_handlers.insert(std::make_pair(0xB0F2, &CNetProcessIn::OnB0F2));
    m_handlers.insert(std::make_pair(0xB0F4, &CNetProcessIn::OnB0F4));
    m_handlers.insert(std::make_pair(0x34B3, &CNetProcessIn::On34B3));
    m_handlers.insert(std::make_pair(0x3101, &CNetProcessIn::On3101));
    m_handlers.insert(std::make_pair(0x34B4, &CNetProcessIn::On34B4));
    m_handlers.insert(std::make_pair(0xB0F9, &CNetProcessIn::OnB0F9));
    m_handlers.insert(std::make_pair(0xB0F6, &CNetProcessIn::OnB0F6));
    m_handlers.insert(std::make_pair(0xB0FA, &CNetProcessIn::OnB0FA));
    m_handlers.insert(std::make_pair(0xB103, &CNetProcessIn::OnB103));
    m_handlers.insert(std::make_pair(0xB104, &CNetProcessIn::OnB104));
    m_handlers.insert(std::make_pair(0xB105, &CNetProcessIn::OnB105));
    m_handlers.insert(std::make_pair(0xB106, &CNetProcessIn::OnB106));
    m_handlers.insert(std::make_pair(0x3908, &CNetProcessIn::On3908));
    m_handlers.insert(std::make_pair(0xB107, &CNetProcessIn::OnB107));
    m_handlers.insert(std::make_pair(0xB0FB, &CNetProcessIn::OnB0FB));
    m_handlers.insert(std::make_pair(0x3102, &CNetProcessIn::On3102));
    m_handlers.insert(std::make_pair(0xB0FC, &CNetProcessIn::OnB0FC));
    m_handlers.insert(std::make_pair(0xB0FD, &CNetProcessIn::OnB0FD));
    m_handlers.insert(std::make_pair(0x3109, &CNetProcessIn::On3109));
    m_handlers.insert(std::make_pair(0x7110, &CNetProcessIn::On7110));
    m_handlers.insert(std::make_pair(0xB110, &CNetProcessIn::OnB110));
    m_handlers.insert(std::make_pair(0xB112, &CNetProcessIn::OnB112));
    m_handlers.insert(std::make_pair(0xB114, &CNetProcessIn::OnB114));
    m_handlers.insert(std::make_pair(0xB113, &CNetProcessIn::OnB113));
    m_handlers.insert(std::make_pair(0xB501, &CNetProcessIn::OnB501));
    m_handlers.insert(std::make_pair(0x30EF, &CNetProcessIn::On30EF));
    m_handlers.insert(std::make_pair(0x3305, &CNetProcessIn::On3305));
    m_handlers.insert(std::make_pair(0xB302, &CNetProcessIn::OnB302));
    m_handlers.insert(std::make_pair(0xB304, &CNetProcessIn::OnB304));
    m_handlers.insert(std::make_pair(0x7302, &CNetProcessIn::On7302));
    m_handlers.insert(std::make_pair(0xB30B, &CNetProcessIn::OnB30B));
    m_handlers.insert(std::make_pair(0xB308, &CNetProcessIn::OnB308));
    m_handlers.insert(std::make_pair(0xB309, &CNetProcessIn::OnB309));
    m_handlers.insert(std::make_pair(0xB30A, &CNetProcessIn::OnB30A));
    m_handlers.insert(std::make_pair(0x3B07, &CNetProcessIn::On3B07));
    m_handlers.insert(std::make_pair(0x3204, &CNetProcessIn::On3204));
    m_handlers.insert(std::make_pair(0xB155, &CNetProcessIn::OnB155));
    m_handlers.insert(std::make_pair(0x3156, &CNetProcessIn::On3156));
    m_handlers.insert(std::make_pair(0xB157, &CNetProcessIn::OnB157));
    m_handlers.insert(std::make_pair(0xB150, &CNetProcessIn::OnB150));
    m_handlers.insert(std::make_pair(0xB151, &CNetProcessIn::OnB151));
    m_handlers.insert(std::make_pair(0xB16A, &CNetProcessIn::OnB16A));
    m_handlers.insert(std::make_pair(0x3207, &CNetProcessIn::On3207));
    m_handlers.insert(std::make_pair(0xB0ED, &CNetProcessIn::OnB0ED));
    m_handlers.insert(std::make_pair(0x34BA, &CNetProcessIn::On34BA));
    m_handlers.insert(std::make_pair(0xB402, &CNetProcessIn::OnB402));
    m_handlers.insert(std::make_pair(0xFFC, &CNetProcessIn::OnFFC));
    m_handlers.insert(std::make_pair(0x2000, &CNetProcessIn::On2000));
    m_handlers.insert(std::make_pair(0xFFD, &CNetProcessIn::OnFFD));
    m_handlers.insert(std::make_pair(0xB05E, &CNetProcessIn::OnB05E));
    m_handlers.insert(std::make_pair(0x3078, &CNetProcessIn::On3078));
    m_handlers.insert(std::make_pair(0x200A, &CNetProcessIn::CharTitleManager));
    m_handlers.insert(std::make_pair(0x200B, &CNetProcessIn::CharTitleManagerColors));
    m_handlers.insert(std::make_pair(0x168B, &CNetProcessIn::AddTitleToTitleManager));
    m_handlers.insert(std::make_pair(0x168C, &CNetProcessIn::AddColorToTitleManagerColor));
    m_handlers.insert(std::make_pair(0x170A, &CNetProcessIn::UpdateTitleColor));
    m_handlers.insert(std::make_pair(0x170B, &CNetProcessIn::RemoveTitleColor));
    m_handlers.insert(std::make_pair(0x201B, &CNetProcessIn::CharacterIconManager));
    m_handlers.insert(std::make_pair(0x168D, &CNetProcessIn::AddIconToIconManager)); // done

    m_handlers.insert(std::make_pair(0x173F, &CNetProcessIn::UpdateCharacterIconLeft));
    m_handlers.insert(std::make_pair(0x174A, &CNetProcessIn::RemoveCharacterIconLeft));
    m_handlers.insert(std::make_pair(0x174B, &CNetProcessIn::UpdateCharacterIconRight));
    m_handlers.insert(std::make_pair(0x174E, &CNetProcessIn::RemoveCharacterIconRight));
    m_handlers.insert(std::make_pair(0x170F, &CNetProcessIn::RankCategories));
    m_handlers.insert(std::make_pair(0x170E, &CNetProcessIn::LoadRank));
    m_handlers.insert(std::make_pair(0x171A, &CNetProcessIn::LoadOwnRank));

    m_handlers.insert(std::make_pair(0x208A, &CNetProcessIn::UpdateUniqueHistory));
    m_handlers.insert(std::make_pair(0x171B, &CNetProcessIn::LoadUniqueHistory));
    m_handlers.insert(std::make_pair(0x171C, &CNetProcessIn::LoadEventRegister));
    m_handlers.insert(std::make_pair(0x171E, &CNetProcessIn::LoadEventSchedule));
    m_handlers.insert(std::make_pair(0x168A, &CNetProcessIn::NoticeSystem));
    m_handlers.insert(std::make_pair(0x202B, &CNetProcessIn::ActivateTag));
    m_handlers.insert(std::make_pair(0x202C, &CNetProcessIn::DeactivateTag));
    m_handlers.insert(std::make_pair(0x205E, &CNetProcessIn::LoadChest));
    m_handlers.insert(std::make_pair(0x208D, &CNetProcessIn::LoadChestV2));
    m_handlers.insert(std::make_pair(0x203B, &CNetProcessIn::AddItemToChest));
    m_handlers.insert(std::make_pair(0x177A, &CNetProcessIn::LoadAchievement));
    m_handlers.insert(std::make_pair(0x177B, &CNetProcessIn::LoadAchievementCondition));
    m_handlers.insert(std::make_pair(0x177C, &CNetProcessIn::LoadAchievementCondition));
    m_handlers.insert(std::make_pair(0x177E, &CNetProcessIn::UpdateAchievementCondition));
    m_handlers.insert(std::make_pair(0x177F, &CNetProcessIn::LiveUniqueDPS));
    m_handlers.insert(std::make_pair(0x178E, &CNetProcessIn::PartyMemberViewerPacket));
    m_handlers.insert(std::make_pair(0x180C, &CNetProcessIn::SetSavedLocation));
    m_handlers.insert(std::make_pair(0x206C, &CNetProcessIn::LoadSavedLocations));
    m_handlers.insert(std::make_pair(0x206F, &CNetProcessIn::LoadLuckySpinRewards));
    m_handlers.insert(std::make_pair(0x2070, &CNetProcessIn::LuckySpinResult));
    m_handlers.insert(std::make_pair(0x2071, &CNetProcessIn::TradeSellCaptchaRequest));
    m_handlers.insert(std::make_pair(0x2072, &CNetProcessIn::LoadSpecialOffers));
    m_handlers.insert(std::make_pair(0x2073, &CNetProcessIn::SpecialOfferPurchaseResult));
    m_handlers.insert(std::make_pair(0x2077, &CNetProcessIn::LoadKillerAnimations));
    m_handlers.insert(std::make_pair(0x2078, &CNetProcessIn::KillerAnimationActionResult));
    m_handlers.insert(std::make_pair(0x2079, &CNetProcessIn::TriggerKillerAnimation));
    m_handlers.insert(std::make_pair(0x2074, &CNetProcessIn::DropLogsResponse));
    m_handlers.insert(std::make_pair(0x2075, &CNetProcessIn::MonsterPossibleDropsResponse));
    m_handlers.insert(std::make_pair(0x2076, &CNetProcessIn::PvpChallengePacket));
    m_handlers.insert(std::make_pair(0x186D, &CNetProcessIn::UpdateStallPriceType));
    m_handlers.insert(std::make_pair(OFFLINE_STALL_ACTIVATE_RESULT_OPCODE, &CNetProcessIn::OfflineStallActivationResult));
    m_handlers.insert(std::make_pair(0x169C, &CNetProcessIn::CustomNoticesTextVariabla));
    m_handlers.insert(std::make_pair(0x209C, &CNetProcessIn::LoadSilkRank));
    m_handlers.insert(std::make_pair(0x208C, &CNetProcessIn::LoadAttendance));
    m_handlers.insert(std::make_pair(0x209A, &CNetProcessIn::LoadAttendanceRewardState));
    m_handlers.insert(std::make_pair(0x208B, &CNetProcessIn::LoadAttendanceReward));
    m_handlers.insert(std::make_pair(0xA4F0, &CNetProcessIn::WebViewerConfig));
    m_handlers.insert(std::make_pair(0x204A, &CNetProcessIn::LoadMacroAutoPotionData));
    m_handlers.insert(std::make_pair(0x204B, &CNetProcessIn::LoadMacroSetting));
    m_handlers.insert(std::make_pair(0x179B, &CNetProcessIn::GlobalChatting));
    m_handlers.insert(std::make_pair(0x209F, &CNetProcessIn::AlchemyLink));
    m_handlers.insert(std::make_pair(0x220A, &CNetProcessIn::SetEventTimer));
    m_handlers.insert(std::make_pair(0x207A, &CNetProcessIn::CreateKillCounter));
    m_handlers.insert(std::make_pair(0x207C, &CNetProcessIn::IncreaseKillCounter));
    m_handlers.insert(std::make_pair(0x182C, &CNetProcessIn::FellowPetAnimation));
    m_handlers.insert(std::make_pair(0x189A, &CNetProcessIn::CreateTeamKillCounter));
    m_handlers.insert(std::make_pair(0x189B, &CNetProcessIn::IncreaseTeamKillCounter));
    m_handlers.insert(std::make_pair(0x189E, &CNetProcessIn::UpdateEventSuit));
    m_handlers.insert(std::make_pair(0x189C, &CNetProcessIn::CreateJobKillCounter));
    m_handlers.insert(std::make_pair(0x189D, &CNetProcessIn::IncreaseJobKillCounter));
    m_handlers.insert(std::make_pair(0x206B, &CNetProcessIn::LoadFellowData));

    m_handlers.insert(std::make_pair(0xF200, &CNetProcessIn::LockItemInfo));
    m_handlers.insert(std::make_pair(0x5028, &CNetProcessIn::UnLockItemInfo));
    m_handlers.insert(std::make_pair(0x5025, &CNetProcessIn::TimedItemInfo));
    m_handlers.insert(std::make_pair(0xF201, &CNetProcessIn::ItemLockUnlockNotice));
    m_handlers.insert(std::make_pair(0x5015, &CNetProcessIn::ItemLockUnlockNotice2));

    m_handlers.insert(std::make_pair(0x193F, &CNetProcessIn::IncreateFtwKillCounter));
    m_handlers.insert(std::make_pair(0x193E, &CNetProcessIn::GetMapPing));
    m_handlers.insert(std::make_pair(0x705E, &CNetProcessIn::OnChatReceiveItemLink));
    m_handlers.insert(std::make_pair(0x5017, &CNetProcessIn::NewAlchemyResult));
    m_handlers.insert(std::make_pair(0x1209, &CNetProcessIn::GetSomeInfos));
    m_handlers.insert(std::make_pair(0x5039, &CNetProcessIn::TargetPlayerItemInfo));
    m_handlers.insert(std::make_pair(0x5040, &CNetProcessIn::UpdateCounterWnd));


    m_handlers.insert(std::make_pair(0x5114, &CNetProcessIn::HandleCircle));
    m_handlers.insert(std::make_pair(0xA405, &CNetProcessIn::UpdateNameColor));
    m_handlers.insert(std::make_pair(0xA406, &CNetProcessIn::RemoveNameColor));

    m_handlers.insert(std::make_pair(0x3563, &CNetProcessIn::SERVER_HANDLER_BLOCKATTACK));

}

void CNetProcessIn::UpdateStallPriceType(CMsgStreamBuffer &msg)
{
    BYTE isSilkStall = 0;
    msg >> isSilkStall;

    g_bCurrentStallUsesSilk = isSilkStall != 0;
}

void CNetProcessIn::OfflineStallActivationResult(CMsgStreamBuffer &msg)
{
    if (msg.m_currentReadBytes > msg.m_availableBytesForReading ||
        msg.m_availableBytesForReading - msg.m_currentReadBytes < sizeof(DWORD) + sizeof(BYTE) + sizeof(WORD)) {
        msg.FlushRemaining();
        return;
    }

    DWORD nonce = 0;
    BYTE success = 0;
    std::n_string message;
    msg >> nonce >> success >> message;
    if (message.length() > 256) {
        message.resize(256);
    }
    OfflineStall_HandleActivationResult(nonce, success != 0, message);
    msg.FlushRemaining();
}

void CNetProcessIn::RemoveNameColor(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    msg >> CharName167;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    if (m_CustomDataManager->_ActiveNameColors.find(CharName16) != m_CustomDataManager->_ActiveNameColors.end())
    {
        m_CustomDataManager->_ActiveNameColors.erase(CharName16);
    }
    CLASSLINK_LOOP_BEGIN(CICUser)
        if (wcscmp(obj->GetName().c_str(), CharName16.c_str()) == 0)
            obj->UpdateNameColor(0xffffff);
    CLASSLINK_LOOP_END(CICUser)
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateNameColor(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    unsigned __int32 NewColor;
    msg >> CharName167 >> NewColor;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    CLASSLINK_LOOP_BEGIN(CICUser)
        if (wcscmp(obj->GetName().c_str(), CharName16.c_str()) == 0)
            obj->UpdateNameColor(NewColor);
    CLASSLINK_LOOP_END(CICUser)

    if (m_CustomDataManager->_ActiveNameColors.find(CharName16.c_str()) != m_CustomDataManager->_ActiveNameColors.end()) {
        m_CustomDataManager->_ActiveNameColors[CharName16] = NewColor;
    } else {

        m_CustomDataManager->_ActiveNameColors.insert((std::make_pair(CharName16, NewColor)));
    }
    msg.FlushRemaining();
}
void CNetProcessIn::ProcessItemMovementResult(void* incomingPacket)
{
    reinterpret_cast<void(__thiscall*)(CNetProcessIn*, void*)>(0x0087F370)(this, incomingPacket);
}
void CNetProcessIn::HandleCircle(CMsgStreamBuffer &msg)
{
    byte CurStage = 0;
    byte ShowHide = 0;
    int Timer = 0 ;
    msg >> CurStage >> ShowHide >> Timer;


    switch (CurStage)
    {

        case 0:
        {
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleRegionStartTime();
        }
            break;
        case 1:
        {
            //Rebot::Config.CircleShow = ShowHide;
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_1_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 2:
        {
            //Rebot::Config.CircleShow = ShowHide;
            //g_pCGInterface->GetCIFSTEffectGUI()->SetBattleRoyaleStage_2_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 3:
        {
            //Rebot::Config.CircleShow = ShowHide;
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_2_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 5:
        {
            //Rebot::Config.CircleShow = ShowHide;
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_3_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 7:
        {
            //Rebot::Config.CircleShow = ShowHide;
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_4_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 9:
        {
            //Rebot::Config.CircleShow = ShowHide;
            g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleStage_5_Time(1);
            m_CustomDataManager->CircleGreenShow = ShowHide;
        }
            break;
        case 16:
        {
           m_CustomDataManager->x1 = 0;
           m_CustomDataManager->x2 = 0;
           m_CustomDataManager->y1 = 0;
           m_CustomDataManager->y2 = 0;
           m_CustomDataManager->cercle = -47;
           m_CustomDataManager->cercleX = 321;
           m_CustomDataManager->cercleY = 202;
           m_CustomDataManager->cercleRadius = 176;
           m_CustomDataManager->CircleShow = false;
           m_CustomDataManager->CircleGreenShow = false;
           g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->SetBattleRoyaleRegionKillTime();
            // g_pCGInterface->GetCIFSTBloodEffectGUI()->ShowGWnd(false);
        }
            break;
        case 17:
        {
            // g_pCGInterface->GetCIFSTBloodEffectGUI()->ShowGWnd(ShowHide);
        }
            break;
    }
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateCounterWnd(CMsgStreamBuffer &msg)
{
    int Left, Right;
    msg >> Left >> Right;

    g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->UpdateCounter(Left, Right);
    g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->UpdateMenuSize();
    g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->ShowGWnd(true);

    msg.FlushRemaining();
}
void CNetProcessIn::TargetPlayerItemInfo(CMsgStreamBuffer &msg)
{
    byte ItemSlot;
    msg >> ItemSlot;

    int Len;
    msg >> Len;
    std::list<BYTE> listBytes;

    for (int i = 0; i < Len; i++) {

        byte bytesa;
        msg >> bytesa;
        listBytes.push_back(bytesa);
    }

    int ItemID;
    msg >> ItemID;

    std::n_string TargetCharName;
    msg >> TargetCharName;
    if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1) != NULL)
    {
        if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND != NULL)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->UpdateCharName(TargetCharName);
        }
    }

    CMsgStreamBuffer buf(0xB034);
    for (std::list<BYTE>::iterator it = listBytes.begin(); it != listBytes.end(); ++it) {
        buf << BYTE(*it);
    }

    //reading the item information directly through a packet by sector1337 (no need for inv slot anymore)
    CSOItem *tempItemInfo = new CSOItem();
    tempItemInfo->ReadFromPacket(&buf, 1);
    tempItemInfo->SetEnabled(true);

    if (tempItemInfo) {
        if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1) != NULL)
        {
            if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND != NULL)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->UpdateItemSlot(ItemSlot, tempItemInfo, TargetCharName);
            }
        }

    }

    msg.FlushRemaining();
}

void CNetProcessIn::SERVER_HANDLER_BLOCKATTACK(CMsgStreamBuffer &msg) {
    byte type;
    msg >> type;

    D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 0, 0);
    CIFSystemMessage *systemmessage = g_pCGInterface->GetSystemMessageView();

    if (type == 0) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_CAN_T_ATTACK_STR_UNIQUES"), 7, 7);
    } else if (type == 1) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_CAN_T_ATTACK_INT_UNIQUES"), 7, 7);
    }
    else if (type == 2) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_MUST_BE_ON_JOB_TO_ATTACK_THIS_UNIQUE"), 7, 7);
    } else if (type == 3) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_MUST_REMOVE_JOB_SUIT_TO_ATTACK_THIS_UNIQUE"), 7, 7);
    } else if (type == 4) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOUR_JOB_TYPE_IS_NOT_ALLOWED_FOR_THIS_UNIQUE"), 7, 7);
    } else if (type == 5) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOUR_CAPE_IS_NOT_ALLOWED_FOR_THIS_UNIQUE"), 7, 7);
    } else if (type == 6) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOUR_RACE_IS_NOT_ALLOWED_FOR_THIS_UNIQUE"), 7, 7);
    } else if (type == 7) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_MUST_BE_IN_A_PARTY_TO_ATTACK_THIS_UNIQUE"), 7, 7);
    } else if (type == 8) {
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_YOU_MUST_BE_IN_A_GUILD_TO_ATTACK_THIS_UNIQUE"), 7, 7);
    }

    msg.FlushRemaining();
}

void CNetProcessIn::GetSomeInfos(CMsgStreamBuffer & msg)
{
    byte type;
    msg >> type;
    if(type == 0)
    {
    }
    msg.FlushRemaining();
}


void CNetProcessIn::NewAlchemyResult(CMsgStreamBuffer &msg)
{
    enum
    {
        SUCCESS = 0,
        FAIL = 1,
        ALC_ERROR = 2,
    };
    byte Type;
    msg >> Type;

    if(Type == 0) /// W PROOF
    {

        byte result;
        msg >> result;
        if (result == SUCCESS)
        {
            byte ItemSlot;
            msg >> ItemSlot;

            byte EnhancerSlot;
            msg >> EnhancerSlot;

            byte ProofSlot;
            msg >> ProofSlot;

            byte btNewOptLevel;
            msg >> btNewOptLevel;

            int nRefItemID;
            msg >> nRefItemID;

            byte AdvPlus;
            msg >> AdvPlus;

            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();

            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsSucces();
            }
            CSOItem*enhancercheck = inventory->GetItemBySlot(EnhancerSlot-13);
            if(enhancercheck == NULL)
            {
                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                {
                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->ClearProofSlot();
                }

            }
            wchar_t Buffers[250];
            swprintf(Buffers, sizeof(Buffers), TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_SUCCESS"), btNewOptLevel + AdvPlus);
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, Buffers);
        }
        else if(result == FAIL)
        {
            byte ItemSlot;
            msg >> ItemSlot;

            byte EnhancerSlot;
            msg >> EnhancerSlot;

            byte ProofSlot;
            msg >> ProofSlot;

            byte btNewOptLevel;
            msg >> btNewOptLevel;

            int nRefItemID;
            msg >> nRefItemID;

            byte AdvPlus;
            msg >> AdvPlus;

            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, KmtGetText(L"UIIT_KMT_THE_ENHANCEMENT_LEVEL_ON_THE_EQUIPMENT_IS_GONE_1_BECAUSE_THE_ALCHEMY_ENHANCEMENT_FAILED"));
            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsFailed();
            }
        }
        else if(result == ALC_ERROR)
        {
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, KmtGetText(L"UIIT_KMT_THE_ENHANCEMENT_STOPPED_CHECK_YOUR_ITEMS"));
            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsFailed();
            }
        }
    }
    else if(Type == 1) /// W(O PROOF
    {
        byte result;
        msg >> result;
        if (result == SUCCESS)
        {
            byte ItemSlot;
            msg >> ItemSlot;

            byte EnhancerSlot;
            msg >> EnhancerSlot;

            byte btNewOptLevel;
            msg >> btNewOptLevel;
            int nRefItemID;
            msg >> nRefItemID;
            byte AdvPlus;
            msg >> AdvPlus;
            CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();

            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsSucces();
            }
            CSOItem*enhancercheck = inventory->GetItemBySlot(EnhancerSlot-13);
            if(enhancercheck == NULL)
            {
                if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
                {
                    g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->ClearProofSlot();
                }

            }
            wchar_t Buffers[250];
            swprintf(Buffers, sizeof(Buffers), TSM_GETTEXTPTR(L"UIIT_MSG_REINFORCERR_SUCCESS"), btNewOptLevel + AdvPlus);
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, Buffers);
        }
        else if(result == FAIL)
        {
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, KmtGetText(L"UIIT_KMT_THE_ENHANCEMENT_LEVEL_ON_THE_EQUIPMENT_IS_GONE_1_BECAUSE_THE_ALCHEMY_ENHANCEMENT_FAILED"));
            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsFailed();
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->PressNewAlchemyTab();
            }
        }
        else if(result == ALC_ERROR)
        {
            g_pCGInterface->WriteSystemMessage(SYSLOG_NONE, KmtGetText(L"UIIT_KMT_THE_ENHANCEMENT_STOPPED_CHECK_YOUR_ITEMS"));
            if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
            {
                g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168)->AlchemyIsFailed();
            }
        }
    }



    msg.FlushRemaining();
}
void CNetProcessIn::OnChatReceiveItemLink(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
#if 1
    std::n_wstring senderW;
    std::n_wstring messageW;
    std::n_string messageA;
    std::n_string senderA;

    int entityId;
    bool bIsFromMyself = false;

    ChatType chatType;

    msg >> chatType;

    switch (chatType) {
        case CHAT_All:
        case CHAT_AllGM: {
            msg >> entityId >> messageA;

            messageW = acp_n_decode(messageA);

            CIGIDObject *pObject = GetCharacterObjectByID_MAYBE(entityId);

            if (pObject == NULL) {
                senderW = L"???";
            } else {
                if (pObject->IsSame(GFX_RUNTIME_CLASS(CICPlayer)))
                    return;

                senderW = pObject->GetName();

                if (pObject->IsKindOf(GFX_RUNTIME_CLASS(CICUser))) {
                    BS_DEBUG("Job type is %d", reinterpret_cast<CICUser *>(pObject)->GetJobType());

                    if (reinterpret_cast<CICUser *>(pObject)->GetJobType() != TRIJOB_NOJOB) {
                        senderW.insert(0, L"*");
                    }
                }
            }
            break;
        }

        default: {
            msg >> senderA >> messageA;

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (senderW == g_pMyPlayerObj->GetCharName())
            {
                bIsFromMyself = true;
            }


            break;
        }

        case CHAT_Party : {
            msg >> senderA >> messageA;

            senderW = acp_n_decode(senderA);
            messageW = acp_n_decode(messageA);

            if (senderW == g_pMyPlayerObj->GetCharName())
            {
                bIsFromMyself = true;
            }


            if(g_pMyPlayerObj->GetJobType() != TRIJOB_NOJOB) {
                senderW.insert(0, L"*");
            }
            break;
        }

        case CHAT_Global: {
            msg >> senderA >> messageA;

            messageW = acp_n_decode(messageA);

            if (senderA.length() == 0) {
                senderW = L"???";
            } else {
                senderW = acp_n_decode(senderA);
            }

            if (g_pMyPlayerObj->GetCharName() == senderW) {
                bIsFromMyself = true;
            }

            break;
        }

        case CHAT_Notice: {
            msg >> messageA;
            messageW = acp_n_decode(messageA);
            bIsFromMyself = false;
            break;
        }

        case CHAT_Stall: {
            msg >> senderA >> messageA;

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (senderW == g_pMyPlayerObj->GetCharName())
            {
                bIsFromMyself = true;
            }


            break;
        }

        case CHAT_NPC: {
            msg >> entityId >> messageA;

            CIGIDObject *pObject = GetCharacterObjectByID_MAYBE(entityId);

            if (pObject == NULL) {
                senderW = L"???";
            } else {
                if (pObject->IsSame(GFX_RUNTIME_CLASS(CICPlayer))) {
                    senderW = reinterpret_cast<CICPlayer *>(pObject)->GetCharName();
                    bIsFromMyself = true;
                } else {
                    senderW = pObject->GetName();
                }
            }

            messageW = g_CTextStringManager->FUN_008c9bb0(messageW);

            if (pObject->IsKindOf(GFX_RUNTIME_CLASS(CICUser))) {
                chatType = CHAT_All;
            }

            break;
        }

        case CHAT_Academy: {
            msg >> senderA >> messageA;

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (senderW == g_pMyPlayerObj->GetCharName())
            {
                bIsFromMyself = true;
            }


        }

    }

    int Len;
    msg >> Len;
    std::list<BYTE> listBytes;

    for (int i = 0; i < Len; i++) {

        byte bytesa;
        msg >> bytesa;
        listBytes.push_back(bytesa);
    }

    if(chatType == CHAT_PM)
    {
        GlobalChat info = GlobalChat();
        //CIFChatViewer* main12 = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
        CIFChatViewer *chatView = (CIFChatViewer *) g_pCGInterface->m_IRM.GetResObj(1, 1);
        int index = chatView->m_ChatLists[0]->GetNumberOfItems();
        if (index == 100) {
            index = 99;
            for (std::list<GlobalChat>::iterator currentGlobalData = g_global->GlobalChatLog.begin(); currentGlobalData != g_global->GlobalChatLog.end(); currentGlobalData++) {
                if (currentGlobalData->completed) {
                    currentGlobalData->index -= 1;//99 =98
                }
            }
        }


        info.index = index;
        info.listBytes = listBytes;
        info.Message = messageW;

        info.completed = true;


        g_global->GlobalChatLog.push_back(info);
    }

    if (g_pMyPlayerObj == NULL)
        return;


    if (senderW.length() == 0) {
        senderW = acp_n_decode(senderA);
    }

    if (messageW.length() == 0) {
        messageW = acp_n_decode(messageA);
    }

    WriteToChatWindow(chatType, senderW, entityId, messageW, bIsFromMyself);

#else
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00877810)(this, msg);
#endif
}
void CNetProcessIn::GetMapPing(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;
    if(type == 0)
    {
        int RegionID, PosX, PosY, PosZ, Seconds;
        msg >> RegionID >> PosX >> PosY >> PosZ >> Seconds;

        CIFMenu * menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        if(m_Settings->ShowGuideMapLocation)
            g_pCGInterface->GetAlarmManager()->CreateGuideIcon(GDR_WEB_GUIDE);
    }
    else if(type == 1)
    {
        int RegionID, PosX, PosY, PosZ, Seconds, MapT;
        msg >> RegionID >> PosX >> PosY >> PosZ >> Seconds >> MapT;
        CIFMenu * menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        menu->PingedRegionID = RegionID;
        menu->PingedPosX = PosX;
        menu->PingedPosY = PosY;
        menu->PingedPosZ = PosZ;
        menu->MapIndex = Seconds;
        menu->MapType = MapT;
        if(m_Settings->ShowGuideMapLocation)
            g_pCGInterface->GetAlarmManager()->CreateGuideIcon(GDR_WEB_GUIDE);

    }


    msg.FlushRemaining();
}
void CNetProcessIn::IncreateFtwKillCounter(CMsgStreamBuffer &msg)
{

    byte Type;
    msg >> Type;
    if(Type == 0)
    {
        BYTE sCount;
        std::n_string Player;
        int Kills;
        msg >> sCount;
        BYTE i = 0;
        g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ClearPlayer();

        while (i < sCount && sCount > 0) {
            i++;
            msg >> Player >> Kills;
            wchar_t Buffers[250];
            swprintf(Buffers, sizeof(Buffers), L"%d", Kills);
            if (Kills == -1) {
                swprintf(Buffers, sizeof(Buffers), L"-");
            }
            std::n_wstring Name = TO_NWSTRING(Player);
            g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->WritePlayerKill(i, Name.c_str(), Buffers);

        }
    }
    if(Type == 1)
    {
        BYTE sCount;
        std::n_string Player;
        int Kills;
        msg >> sCount;
        BYTE i = 0;
        g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ClearGuild();

        while (i < sCount && sCount > 0) {
            i++;
            msg >> Player >> Kills;
            wchar_t Buffers[250];
            swprintf(Buffers, sizeof(Buffers), L"%d", Kills);
            if (Kills == -1) {
                swprintf(Buffers, sizeof(Buffers), L"-");
            }
            std::n_wstring Name = TO_NWSTRING(Player);
            g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->WriteGuildKill(i, Name.c_str(), Buffers);

        }
    }
    if(Type == 2)
    {
        BYTE sCount;
        std::n_string Player;
        int Kills;
        msg >> sCount;
        BYTE i = 0;
        g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ClearUnion();

        while (i < sCount && sCount > 0) {
            i++;
            msg >> Player >> Kills;
            wchar_t Buffers[250];
            swprintf(Buffers, sizeof(Buffers), L"%d", Kills);
            if (Kills == -1) {
                swprintf(Buffers, sizeof(Buffers), L"-");
            }
            std::n_wstring Name = TO_NWSTRING(Player);
            g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->WriteUnionKill(i, Name.c_str(), Buffers);

        }
    }
    g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->UpdateMenuSize();
    if(!g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ShowGWnd(true);
    }
    msg.FlushRemaining();
}

void CNetProcessIn::ItemLockUnlockNotice2(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;

    if(type == 0)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_LOCKED_ITEM"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_LOCKED_ITEM"));

    }
    else if(type == 1)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_LOCKED_ITEM"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_LOCKED_ITEM"));
    }
    else if(type == 3)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_GOLD_AMOUNT_IS_NOT_ENOUGH_FOR_EQUIP_TRANSLATION"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_GOLD_AMOUNT_IS_NOT_ENOUGH_FOR_EQUIP_TRANSLATION"));
    }
    else if(type == 4)
    {
        if (g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->EnableStateLockButton();
        }
        if (g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->EnableStateLockButton();
        }
    }
    else if(type == 5)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_UNLOCKED_ITEM"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_YOU_CANNOT_PROCEED_THIS_ACTION_ON_A_UNLOCKED_ITEM"));

    }
    else if(type == 6)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_SILK_AMOUNT_IS_NOT_ENOUGH_FOR_EQUIP_TRANSLATION"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_SILK_AMOUNT_IS_NOT_ENOUGH_FOR_EQUIP_TRANSLATION"));
    }
    else if(type == 7)
    {
        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THE_TARGET_CHARACTER_IS_HIDE_SELF_ITEMS"));
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, KmtGetText(L"UIIT_KMT_THE_TARGET_CHARACTER_IS_HIDE_SELF_ITEMS"));
       /* if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1) != NULL)
        {
            if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND != NULL)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->Hide();
            }
        }*/
    }
    else if(type == 8)
    {
        if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1) != NULL)
        {
            if(g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND != NULL)
            {
                g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->ClearTargetEquipment();
                g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->ShowTargetWnd();
                g_pCGInterface->m_IRM.GetResObj<CIFTargetWindow>(16, 1)->m_pGDR_TW_PLAYERWND->UpdateRenderStatic();
            }
        }
    }
    else if(type == 9)
    {
       bool Value;
       msg >> Value;
       bool RememberPC;
       msg >> RememberPC;
        CIFSettings* setting = g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1);
        if(setting != NULL)
        {
            setting->HideCharInfo->SetCheckBoxState(Value);
            setting->RememberPC->SetCheckBoxState(RememberPC);
            setting->HideCharInfoSetting = Value;
            setting->RememberPCSetting = RememberPC;

        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::ItemLockUnlockNotice(CMsgStreamBuffer &msg)
{
    std::n_wstring MsgNotice;
    msg >> MsgNotice;

    byte itemslot;
    msg >> itemslot;

    if (itemslot >= 13) {
        CSOItem *p = g_pCGInterface->GetMainPopup()->GetInventory()->GetItemBySlot(itemslot-13);

        if (p != NULL) {

            const SItemData *data = &g_CGlobalDataManager->GetItemData(p->m_refObjItemId);
            if(data != NULL)
            {
                std::wstring ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();

                StringReplaceAll(MsgNotice, L"{wpname}", ItemNames);
            }
        }
    }


    g_pCGInterface->ShowMessage_Warning(MsgNotice);
    g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, MsgNotice.c_str());

    if (g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->ShowGWnd(false);
        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->Clear();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->ShowGWnd(false);
        g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->Clear();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }

    msg.FlushRemaining();
}
void CNetProcessIn::TimedItemInfo(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;
    byte targetslotnum;
    msg >> targetslotnum;

    if(type == 0)
    {
        long min;
        msg >> min;
        if (targetslotnum >= 13) {
            CSOItem *p = g_pCGInterface->GetMainPopup()->GetInventory()->GetItemBySlot(targetslotnum-13);

            if (p != NULL) {
                p->itemtimes = min;
            }
        }
        else if (targetslotnum <= 12) {
            g_pCGInterface->GetMainPopup()->GetEquipment()->SetItemTime(targetslotnum, min);
        }
    }
    else if(type == 1)
    {
        if (targetslotnum >= 13) {
            CSOItem *p = g_pCGInterface->GetMainPopup()->GetInventory()->GetItemBySlot(targetslotnum-13);
            if (p != NULL) {
                p->itemtimes = 0;
            }
        }
        else if (targetslotnum <= 12) {
            g_pCGInterface->GetMainPopup()->GetEquipment()->SetItemTime(targetslotnum, 0);

        }
    }
    else if(type == 2)
    {
        long min;
        msg >> min;
        CSOItem *p = g_pCGInterface->GetMainPopup()->GetEquipment()->Test4(targetslotnum-1);
        if (p != NULL) {
            //    printf("%ls \n", p->GetItemData()->CodeName.c_str());
            p->itemtimes = min;
        }
    }
    else if(type == 3)
    {
        g_pCGInterface->GetMainPopup()->GetEquipment()->SetItemTime(20, 0);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::UnLockItemInfo(CMsgStreamBuffer &msg)
{
    byte targetslotnum;
    msg >> targetslotnum; /// its slot number

    if(targetslotnum < 13)
    {
        if(g_pCGInterface->GetMainPopup()->GetEquipment() != NULL)
        {
            g_pCGInterface->GetMainPopup()->GetEquipment()->SetSlotUnLock(targetslotnum);
        }
    }
    else if(targetslotnum > 12)
    {
        if(g_pCGInterface->GetMainPopup()->GetInventory() != NULL)
        {
            CSOItem *p = g_pCGInterface->GetMainPopup()->GetInventory()->GetItemBySlot(targetslotnum-13);
            if (p != NULL) {
                p->ItemLocked = 0;
            }
        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LockItemInfo(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type; /// type 0 is equipment and inventory /// type 1 is avatar
    byte targetslotnum;
    msg >> targetslotnum; /// its slot number
    if(type == 0)
    {
        if(targetslotnum < 13)
        {
            if(g_pCGInterface->GetMainPopup()->GetEquipment() != NULL)
            {
                g_pCGInterface->GetMainPopup()->GetEquipment()->SetSlotLock(targetslotnum);
            }
        }
        else if(targetslotnum > 12)
        {
            if(g_pCGInterface->GetMainPopup()->GetInventory() != NULL)
            {
                CSOItem *p = g_pCGInterface->GetMainPopup()->GetInventory()->GetItemBySlot(targetslotnum-13);
                if (p != NULL) {
                    p->ItemLocked = 1;
                }
            }
        }

    }
    else if(type == 1)
    {
        CSOItem *p = g_pCGInterface->GetMainPopup()->GetEquipment()->Test4(targetslotnum-1);
        if (p != NULL) {
            if(p->GetItemData() != NULL)
            {
                p->ItemLocked = 1;
            }
            //    printf("%ls \n", p->GetItemData()->CodeName.c_str());

        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LoadFellowData(CMsgStreamBuffer &msg)
{
    m_Player->m_FellowSkillData.clear();
    byte Count;
    msg >> Count;
    byte i = 0;
    while (i < Count && Count > 0) {
        i++;
        __int64 ID64;
        msg >> ID64;

        byte Enable_Skill_1, Enable_Skill_2,Enable_Skill_3,Enable_Skill_4,Enable_Skill_5;
        msg >> Enable_Skill_1 >> Enable_Skill_2 >> Enable_Skill_3 >> Enable_Skill_4 >> Enable_Skill_5;

        CustomCICPlayer::Fellow_SkillData Data = CustomCICPlayer::Fellow_SkillData();
        Data.ID64 = ID64;
        Data.Enable_Skill_1 = Enable_Skill_1;
        Data.Enable_Skill_2 = Enable_Skill_2;
        Data.Enable_Skill_3 = Enable_Skill_3;
        Data.Enable_Skill_4 = Enable_Skill_4;
        Data.Enable_Skill_5 = Enable_Skill_5;
        m_Player->m_FellowSkillData.push_back(Data);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::IncreaseJobKillCounter(CMsgStreamBuffer &msg)
{
    BYTE sCount;
    msg >> sCount;

    std::n_string Players;
    byte Team;
    int Kills;
    int RedTeamKills;
    int BlueTeamKills;

    BYTE i = 0;
    g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();

    while (i < sCount && sCount > 0) {
        i++;
        msg >>  Players >> Team >> Kills >> RedTeamKills >> BlueTeamKills;

        std::n_wstring Player = TO_NWSTRING(Players);

        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->WriteLine(i, Team, Player.c_str(), Kills, RedTeamKills, BlueTeamKills);

    }
    g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->UpdateMenuSize();
    if(!g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->ShowGWnd(true);
    }

    msg.FlushRemaining();
}
void CNetProcessIn::CreateJobKillCounter(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;

    if (type == 0)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();
    }
    else if(type == 1)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();
        if(!g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->ShowGWnd(true);
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->UpdateMenuSize();
        }
    }
    else if(type == 2)
    {
        if(!g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->ShowGWnd(true);
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->UpdateMenuSize();
        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::IncreaseTeamKillCounter(CMsgStreamBuffer &msg)
{
    BYTE sCount;
    msg >> sCount;

    std::n_string Players;
    byte Team;
    int Kills;
    int RedTeamKills;
    int BlueTeamKills;

    BYTE i = 0;
    g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->Clear();

    while (i < sCount && sCount > 0) {
        i++;
        msg >>  Players >> Team >> Kills >> RedTeamKills >> BlueTeamKills;

        std::n_wstring Player = TO_NWSTRING(Players);

        g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->WriteLine(i, Team, Player.c_str(), Kills, RedTeamKills, BlueTeamKills);

    }
    g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->UpdateMenuSize();
    if(!g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->ShowGWnd(true);
    }
    msg.FlushRemaining();
}

void CNetProcessIn::UpdateEventSuit(CMsgStreamBuffer &msg)
{
    int count;
    msg >> count;

    for (int i = 0; i < count; ++i)
    {
        int RegionID;
        bool EventSuit;
        bool HideBuffViewer;
        bool DisablePetSpawn;
        bool DisableParty;
        bool AutoCape;
        bool HideMiniMap;
        byte RegionType;

        msg >> RegionID;
        msg >> EventSuit;
        msg >> HideBuffViewer;
        msg >> DisablePetSpawn;
        msg >> DisableParty;
        msg >> AutoCape;
        msg >> HideMiniMap;
        msg >> RegionType;

        CustomDataManager::SEventMapSettings settings;
        settings.RegionID = RegionID;
        settings.EventSuit = EventSuit;
        settings.HideBuffViewer = HideBuffViewer;
        settings.DisablePetSpawn = DisablePetSpawn;
        settings.DisableParty = DisableParty;
        settings.AutoCape = AutoCape;
        settings.HideMiniMap = HideMiniMap;
        settings.RegionType = RegionType;

        m_CustomDataManager->m_EventMapSettings[RegionID] = settings;
    }

    ApplyEventSuitToCurrentPlayer();
    msg.FlushRemaining();
}

void CNetProcessIn::CreateTeamKillCounter(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;

    if (type == 0)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->Clear();
    }
    else if(type == 1)
    {
        if(!g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->ShowGWnd(true);
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->UpdateMenuSize();
        }
    }
    else if(type == 2)
    {
        if(!g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->ShowGWnd(true);
        }
        else
        {
           g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->Clear();
           g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->UpdateMenuSize();
        }
    }
    msg.FlushRemaining();
}
time_t LastAniTime;
void CNetProcessIn::FellowPetAnimation(CMsgStreamBuffer &msg) {
    unsigned int uqId;
    msg >> uqId;
    byte AniID;
    msg >> AniID;
    CICCos *pUser = static_cast<CICCos *>(GetCharacterObjectByID_MAYBE(uqId));
    if (pUser != NULL && pUser->GetCommonData() != NULL && m_CustomDataManager != NULL)
    {
        if (m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end()) {
            KillerAnimationPlayer::QueueOnObjectUniqueId(uqId, AniID, 400, 400, 5000);
        }
    }

    msg.FlushRemaining();
}
void CNetProcessIn::CreateKillCounter(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;
    std::n_string Titlex;
    msg >> Titlex;
    std::n_wstring Title = TO_NWSTRING(Titlex);
    if (type == 0)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->m_pTitleText->SetText(Title.c_str());
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->Clear();
    }
    else if(type == 1)
    {
        if(!g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->m_pTitleText->SetText(Title.c_str());
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->ShowGWnd(true);
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->m_pTitleText->SetText(Title.c_str());
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->UpdateMenuSize();
        }
    }
    else if(type == 2)
    {
        if(!g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->m_pTitleText->SetText(Title.c_str());
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->ShowGWnd(true);
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->Clear();
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->m_pTitleText->SetText(Title.c_str());
            g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->UpdateMenuSize();
        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::IncreaseKillCounter(CMsgStreamBuffer &msg)
{
    BYTE sCount;
    msg >> sCount;

    BYTE i = 0;
    g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->Clear();

    while (i < sCount && sCount > 0) {
        i++;

        std::n_string CharName166;
        int Kills;
        msg >> CharName166;
        msg >> Kills;

        std::n_wstring CharName16 = TO_NWSTRING(CharName166);
        wchar_t Buffers[250];
        swprintf(Buffers, sizeof(Buffers), L"%d", Kills);

        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->WriteLine(i, CharName16.c_str(), Buffers);
    }
    if(!g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->ShowGWnd(true);
    }
    msg.FlushRemaining();
}

void CNetProcessIn::SetEventTimer(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;
    if(type == 0)
    {
        int timesecond;
        msg >> timesecond;
        g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->ShowGWnd(true);
        g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->SetTimer(timesecond, 0);
        g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->StartTimerWnd2();
    }
    else if(type == 1)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTimerWnd>(410, 1)->ShowGWnd(false);
    }

    msg.FlushRemaining();
}
void CNetProcessIn::AlchemyLink(CMsgStreamBuffer &msg)
{
    std::n_wstring CharName16;
    msg >> CharName16;

    int itemID = 0;
    msg >> itemID;

    byte Plus;
    msg >> Plus;

    byte AdvLevel;
    msg >> AdvLevel;

    int Len;
    msg >> Len;
    std::list<BYTE> listBytes;

    for (int i = 0; i < Len; i++) {

        byte bytesa;
        msg >> bytesa;
        listBytes.push_back(bytesa);
    }

    const SItemData *data = &g_CGlobalDataManager->GetItemData(itemID);

    std::wstring ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();


    std::wstring Item = L"<" + ItemNames + L">";

    std::n_wstring Message;
    if(AdvLevel == 0)
    {
        Message = KmtGetText(L"UIIT_KMT_CHARNAME_HAS_SUCCEEDED_IN_AUGMENTING_WPNAME_S_PLUS_PLUS_LEVEL");
    }
    else
    {
        Message = KmtGetText(L"UIIT_KMT_CHARNAME_HAS_SUCCEEDED_IN_AUGMENTING_WPNAME_S_PLUS_PLUS_LEVEL_WITH_ADV_ELIXIR");
    }


    StringReplaceAll(Message, L"{wpname}", Item);

    StringReplaceAll(Message, L"{charname}", CharName16.c_str());

    std::wstringstream wss;
    wss << Plus + AdvLevel;
    std::wstring wstr = wss.str();


    StringReplaceAll(Message, L"{plus}", wstr);


    GlobalChat info = GlobalChat();
    //CIFChatViewer* main12 = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
    CIFChatViewer *chatView = (CIFChatViewer *) g_pCGInterface->m_IRM.GetResObj(1, 1);
    int index = chatView->m_ChatLists[0]->GetNumberOfItems();
    if (index == 100) {
        index = 99;
        for (std::list<GlobalChat>::iterator currentGlobalData = g_global->GlobalChatLog.begin(); currentGlobalData != g_global->GlobalChatLog.end(); currentGlobalData++) {
            if (currentGlobalData->completed) {
                currentGlobalData->index -= 1;//99 =98
            }
        }
    }


    info.index = index;
    info.listBytes = listBytes;
    info.Message = Message;

    info.completed = true;


    g_global->GlobalChatLog.push_back(info);

    D3DCOLOR color;
    color = D3DCOLOR_RGBA(0xff, 0xae, 0xc3, 255);

    g_pCGInterface->ShowMessage_Notice(Message.c_str());
    g_pCGInterface->FUN_00777c30(CHAT_Global, Message.c_str(), color, 1);



   /* AlchemyLogStr info = AlchemyLogStr();
    //CIFChatViewer* main12 = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
    CIFSystemMessage *chatView = (CIFSystemMessage *) g_pCGInterface->m_IRM.GetResObj(68, 1);
    int index = chatView->GetTextBox()->GetNumberOfItems();
    if (index == 100) {
        index = 99;
        for (std::list<AlchemyLogStr>::iterator currentGlobalData = g_global->AlchemyLog.begin(); currentGlobalData != g_global->AlchemyLog.end(); currentGlobalData++) {
            if (currentGlobalData->completed) {
                currentGlobalData->index -= 1;//99 =98
            }
        }
    }


    info.index = index;
    info.listBytes = listBytes;
    info.Message = Message;
    info.CosIDForPet = itemID;
    info.completed = true;


    g_global->AlchemyLog.push_back(info);
*/

  //  g_pCGInterface->ShowLogMessage(0x00cec8, Message.c_str());


    msg.FlushRemaining();
}
void CNetProcessIn::GlobalChatting(CMsgStreamBuffer &msg)
{
    byte GlobalType;
    msg >> GlobalType;

    std::n_wstring Message;
    msg >> Message;

    int color;
    msg >> color;
    if(GlobalType == 0)
    {
        g_pCGInterface->FUN_00777c30(CHAT_Global, Message.c_str(), color, 1);


        std::n_wstring newmsg = Message.c_str();
        std::vector<std::n_wstring> splitted = wsplit(newmsg, L':');
        std::n_wstring string = std::n_wstring(splitted[1]);
        std::n_wstring charname = std::n_wstring(splitted[0]);
        CICUser *entity = 0;

        CLASSLINK_LOOP_BEGIN(CICUser)
            if (obj->GetName() == charname.c_str()) {
                entity = obj;
                if (entity)
                    entity->ShowMessageAboveEntity(string, color);
            }
        CLASSLINK_LOOP_END(CICUser)
    }
    else if(GlobalType == 1)
    {
        int Len;
        msg >> Len;
        std::list<BYTE> listBytes;

        for (int i = 0; i < Len; i++) {

            byte bytesa;
            msg >> bytesa;
            listBytes.push_back(bytesa);
        }

        int itemID = 0;
        msg >> itemID;
        const SItemData *data = &g_CGlobalDataManager->GetItemData(itemID);
        std::wstring ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();


        std::wstring Item = L"<" + ItemNames + L">";
        std::wstring underline = L"\u035F";
        std::wstring ItemName;
        for (int i = 0; i < Item.size(); i++) {
            std::wstring bkp = Item.substr(i, 1);
            ItemName.append(underline);
            ItemName.append(bkp);
        }
        StringReplaceAll(Message, L"{wpname}", ItemName.c_str());

        GlobalChat info = GlobalChat();
        //CIFChatViewer* main12 = g_pCGInterface->m_IRM.GetResObj<CIFChatViewer>(1, 1);
        CIFChatViewer *chatView = (CIFChatViewer *) g_pCGInterface->m_IRM.GetResObj(1, 1);
        int index = chatView->m_ChatLists[0]->GetNumberOfItems();
        if (index == 100) {
            index = 99;
            for (std::list<GlobalChat>::iterator currentGlobalData = g_global->GlobalChatLog.begin(); currentGlobalData != g_global->GlobalChatLog.end(); currentGlobalData++) {
                if (currentGlobalData->completed) {
                    currentGlobalData->index -= 1;//99 =98
                }
            }
        }


        info.index = index;
        info.listBytes = listBytes;
        info.Message = Message;

        info.completed = true;


        g_global->GlobalChatLog.push_back(info);


        g_pCGInterface->FUN_00777c30(CHAT_Global, Message.c_str(), color, 1);
        //     g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());


        std::n_wstring newmsg = Message.c_str();
        std::vector<std::n_wstring> splitted = wsplit(newmsg, L':');
        std::n_wstring string = std::n_wstring(splitted[1]);
        std::n_wstring charname = std::n_wstring(splitted[0]);
        CICUser *entity = 0;


        CLASSLINK_LOOP_BEGIN(CICUser)
            if (obj->GetName() == charname.c_str()) {
                entity = obj;
                if (entity)
                    entity->ShowMessageAboveEntity(string, color);
            }
        CLASSLINK_LOOP_END(CICUser)
    }

    msg.FlushRemaining();
}
void CNetProcessIn::LoadMacroSetting(CMsgStreamBuffer &msg)
{

    CIFMacroMenu* MacroMenu = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);

    MacroMenu->AutoPotionSlot->ActivateTabPage(0);
    MacroMenu->AutoSkillSlot->ActivateTabPage(0);
    MacroMenu->AutoHuntSlot->ActivateTabPage(0);
    MacroMenu->PickupFilterSlot->ActivateTabPage(0);

    msg >> MacroMenu->AutoPotionSlot->Macro_AutoPotion;

    if(MacroMenu->AutoPotionSlot->Macro_AutoPotion)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ActiveAutoPotion();
    }

    msg >> MacroMenu->AutoSkillSlot->Macro_AutoSkill;

    if(MacroMenu->AutoSkillSlot->Macro_AutoSkill)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ActiveAutoSkill();
    }


    msg >> MacroMenu->AutoHuntSlot->Macro_AutoHunt;
    if(MacroMenu->AutoHuntSlot->Macro_AutoHunt)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ActiveAutoHunt();
    }



    msg >> MacroMenu->PickupFilterSlot->Macro_PetFilter;
    if(MacroMenu->PickupFilterSlot->Macro_PetFilter)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ActivePetFilter();
    }

    msg >> MacroMenu->AutoScrollSlot->Macro_AutoScroll;
    if(MacroMenu->AutoScrollSlot->Macro_AutoScroll)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ActiveAutoScroll();
    }

    msg.FlushRemaining();
}
void CNetProcessIn::LoadMacroAutoPotionData(CMsgStreamBuffer &msg)
{
    byte Count;
    msg >> Count;
    byte i = 0;
    while (i < Count && Count > 0) {
        i++;

        byte SlotSeq, Active, Value;
        msg >> SlotSeq >> Active >> Value;
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->LoadSettings(SlotSeq, Active, Value);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LoadAttendanceReward(CMsgStreamBuffer & msg){
    byte Count;
    msg >> Count;

    CIFDailyLogin* dailyLogin =
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1);
    if (dailyLogin == NULL) {
        msg.FlushRemaining();
        return;
    }

    dailyLogin->m_RefAttendanceRewards.clear();

    byte i = 0;
    while (i < Count) {
        int ID, ItemID, ItemCount, DayCount;
        msg >> ID;
        msg >> ItemID;
        msg >> ItemCount;
        msg >> DayCount;
        if (i < 50 && DayCount >= 1 && DayCount <= 35) {
            CIFDailyLogin::RefAttendanceEventStruct Data = CIFDailyLogin::RefAttendanceEventStruct();
            Data.ID = ID;
            Data.ItemID = ItemID;
            Data.ItemCount = ItemCount;
            Data.DayCount = DayCount;
            dailyLogin->m_RefAttendanceRewards.insert((std::make_pair(ID, Data)));
        }
        ++i;
    }
    dailyLogin->UpdateMenuSize();
    msg.FlushRemaining();
}

void CNetProcessIn::WebViewerConfig(CMsgStreamBuffer &msg) {
    unsigned short count = 0;
    msg >> count;

    if(count > WEBVIEWER_MAX_BUTTONS)
        count = WEBVIEWER_MAX_BUTTONS;

    std::vector<SWebViewerButtonConfig> buttons;
    for(unsigned short i = 0; i < count; ++i) {
        SWebViewerButtonConfig cfg;
        cfg.Id = WEBVIEWER_GUIDE_BASE_ID + (int)i;
        cfg.FrameWidth = 900;
        cfg.FrameHeight = 620;

        msg >> cfg.Name;
        msg >> cfg.IconPath;
        msg >> cfg.Url;
        msg >> cfg.FrameWidth;
        msg >> cfg.FrameHeight;

        if(cfg.FrameWidth < 320) cfg.FrameWidth = 320;
        if(cfg.FrameHeight < 240) cfg.FrameHeight = 240;
        if(cfg.FrameWidth > 1200) cfg.FrameWidth = 1200;
        if(cfg.FrameHeight > 900) cfg.FrameHeight = 900;

        if(cfg.Name.empty() || cfg.IconPath.empty() || cfg.Url.empty())
            continue;

        cfg.WideName.assign(cfg.Name.begin(), cfg.Name.end());
        cfg.WideUrl.assign(cfg.Url.begin(), cfg.Url.end());
        buttons.push_back(cfg);
    }

    if(g_pCGInterface != NULL && g_pCGInterface->GetAlarmManager() != NULL) {
        for(int i = 0; i < WEBVIEWER_MAX_BUTTONS; ++i) {
            g_pCGInterface->GetAlarmManager()->RemoveGuide(WEBVIEWER_GUIDE_BASE_ID + i);
        }
    }

    CWebViewerConfig::SetRuntimeButtons(buttons);

    if(g_pCGInterface == NULL || g_pCGInterface->GetAlarmManager() == NULL)
        return;

    if(!m_Settings->ShowGuideWebViewer) {
        g_pCGInterface->GetAlarmManager()->PrepareGuides();
        return;
    }

    for(int i = 0; i < CWebViewerConfig::GetCount(); ++i) {
        const SWebViewerButtonConfig* cfg = CWebViewerConfig::GetByIndex(i);
        if(cfg == NULL)
            continue;
        g_pCGInterface->GetAlarmManager()->CreateGuideIcon(cfg->Id);
    }
    g_pCGInterface->GetAlarmManager()->PrepareGuides();
}

void CNetProcessIn::LoadAttendanceRewardState(CMsgStreamBuffer & msg){
    BYTE sCount;
    msg >> sCount;

    CIFDailyLogin* dailyLogin =
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1);
    if (dailyLogin == NULL) {
        msg.FlushRemaining();
        return;
    }

    for (int j = 0; j < 50; ++j)
    {
        if (dailyLogin->rewardslot[j] != NULL)
            dailyLogin->rewardslot[j]->OpenReceiveButton(false);
    }

    int i = 0;
    while (i < sCount) {
        i++;
        int RefRewardID;
        msg >> RefRewardID;
        for (int x = 0; x < 50; ++x)
        {
            if(dailyLogin->rewardslot[x] != NULL &&
               dailyLogin->rewardslot[x]->RefRewardID == RefRewardID)
            {
                dailyLogin->rewardslot[x]->OpenReceiveButton(true);
            }
        }
    }

    msg.FlushRemaining();
}
void CNetProcessIn::LoadAttendance(CMsgStreamBuffer & msg){
    byte DayCount;
    msg >> DayCount;

    CIFDailyLogin* dailyLogin =
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1);
    if (dailyLogin == NULL) {
        msg.FlushRemaining();
        return;
    }

    if (DayCount > 35)
        DayCount = 35;

    for (int i = 0; i < 35; ++i)
    {
        if (dailyLogin->dayslots[i] != NULL)
            dailyLogin->dayslots[i]->ClearOK();
    }
    for(int i = 0; i < DayCount; i++)
    {
        if (dailyLogin->dayslots[i] != NULL)
            dailyLogin->dayslots[i]->ShowOK();
    }
    msg.FlushRemaining();
}

void CNetProcessIn::LoadSilkRank(CMsgStreamBuffer &msg)
{

    msg >>  g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->SilkHistory;
    msg >>  g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->SilkRank;

    msg.FlushRemaining();
}
void CNetProcessIn::CustomNoticesTextVariabla(CMsgStreamBuffer &msg)
{
    __int8 operators = 0;
    msg >> operators;
    std::n_wstring Message;
    msg >> Message;
    int ItemID;
    msg >> ItemID;


    //  g_CTextStringManager->GetString(&variable.c_str());
    std::n_wstring NoticeStr = Message.c_str();

    std::n_wstring ItemNames = std::n_wstring();
    const SItemData *data = &g_CGlobalDataManager->GetItemData(ItemID);
    if(data != NULL)
    {
        ItemNames = g_CTextStringManager->GetString2(data->NameStrID.c_str())->c_str();
    }

    wchar_t NoticeStr2[1000];
    swprintf_s(NoticeStr2, sizeof(NoticeStr2), Message.c_str(), ItemNames.c_str());


    D3DCOLOR color;
    color = D3DCOLOR_RGBA(0xff, 0xae, 0xc3, 255);
    if (operators == 1) {
        g_pCGInterface->ShowMessage_Quest(NoticeStr2);
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, NoticeStr2);
    } else if (operators == 2) {
        g_pCGInterface->ShowMessage_Notice(NoticeStr);
        g_pCGInterface->FUN_00777c30(CHAT_AllGM, NoticeStr2, color, 1);
    } else if (operators == 3) {
        g_pCGInterface->ShowMessage_Warning(NoticeStr);
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, NoticeStr2);
    } else if (operators == 4) {
        g_pCGInterface->ShowMessage_Notice(NoticeStr);
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, NoticeStr2);
    }
        /*else if (operators == 4) {
                g_pCGInterface->ShowMessage_Purble(Message.c_str());
                g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());
            }
            else if (operators == 5) {
                g_pCGInterface->ShowMessage_Orange(Message.c_str());
                g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());
            }*/
    else if (operators == 6) {
        g_pCGInterface->ShowMessage_Quest(NoticeStr2);
    } else if (operators == 7) {
        g_pCGInterface->ShowMessage_Notice(NoticeStr2);
    } else if (operators == 8) {
        g_pCGInterface->ShowMessage_Warning(NoticeStr2);
    } else if (operators == 9) {
        CIFSystemMessage *systemmessage = g_pCGInterface->GetSystemMessageView();
        systemmessage->WriteMessage(255, 0xFFFF00, NoticeStr2, 7, 7);
    } else if (operators == 11) {
        g_pCGInterface->ShowLogMessage(color, NoticeStr2);// RED SYSTEM MESSAGE
    } else if (operators == 12) {
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, NoticeStr2);// sadece default renk sağ taraf
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LoadSavedLocations(CMsgStreamBuffer &msg)
{
    byte Count;
    msg >> Count;
    byte i = 0;
    while (i < Count && Count > 0) {
        i++;
        byte LocationID;
        int RegionID;
        int WorldID;
        msg >> LocationID >> RegionID >> WorldID;
        CIFSavedLocation::SavedLocation_Data Data = CIFSavedLocation::SavedLocation_Data();
        Data.LocationID = LocationID;
        Data.RegionID = RegionID;
        Data.WorldID = WorldID;
        g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->m_SavedLocation.insert((std::make_pair(LocationID, Data)));
    }
    msg.FlushRemaining();
}
void CNetProcessIn::SetSavedLocation(CMsgStreamBuffer &msg)
{
    byte LocationID;
    int RegionID;
    msg >> LocationID >> RegionID;
    g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->SetButtonText(LocationID, RegionID);
    msg.FlushRemaining();
}
void CNetProcessIn::PartyMemberViewerPacket(CMsgStreamBuffer &msg)
{
    byte type;
    msg >> type;
    if (type == 0) /// send part info
    {
        std::n_string TargetChar;
        msg >> TargetChar;
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        CMsgStreamBuffer buf(0x185C);
        buf << (BYTE)0x0;
        buf << TargetChar;
        buf << (BYTE)(partyData.NumberOfMembers);
        for (int i = 0; i < partyData.NumberOfMembers; ++i)
        {

            const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
            //buf << std::n_string(TO_NSTRING(memberData.m_charactername));
            buf << (byte)memberData.currentLevel;
            buf << memberData.refobjID;
            buf << memberData.Mastery1;
            buf << memberData.Mastery2;
        }
        SendMsg(buf);
    }
    else if (type == 1) /// show and read party info
    {
        g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->Clear();
        BYTE count;
        msg >> count;

        int i = 0;
        while (i < count && count > 0) {
            i++;
            int Race;
            msg >> Race;
            byte Charlevel;
            msg >> Charlevel;
            int Mastery1;
            msg >> Mastery1;
            int Mastery2;
            msg >> Mastery2;

            g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->WriteLine(i, Race, Charlevel, Mastery1, Mastery2);
        }
        g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->ShowGWnd(true);

    }
    else if (type == 2)
    {
        std::n_string TargetChar;
        msg >> TargetChar;
        const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
        CMsgStreamBuffer buf(0x185C);
        buf << (BYTE)0x1;
        buf << TargetChar;

        // En yüksek iki btLevel değerlerini tutacak değişkenler
        unsigned char highest1 = 0;
        unsigned char highest2 = 0;

        // En yüksek iki btLevel değerine sahip nMasteryId'leri tutacak değişkenler
        int masteryId1 = 0;
        int masteryId2 = 0;


        // Map'i dolaşarak en yüksek iki btLevel değerine sahip MasteryID'leri bul
        std::map<int, sMasteryData *>::iterator it;
        for (it = g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_masteryData.begin(); it != g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.m_masteryData.end(); ++it) {
            sMasteryData *data = it->second;
            // Eğer mevcut MasteryID, en büyük MasteryID'den büyükse, ikinci en büyük MasteryID'yi güncelle
            if (data->btLevel > highest1) {
                highest2 = highest1;
                highest1 = data->btLevel;
                masteryId2 = masteryId1;
                masteryId1 = it->first;
            }
                // Eğer mevcut btLevel, ikinci en büyük btLevel'den büyükse, ikinci en büyük btLevel'i güncelle
            else if (data->btLevel > highest2) {
                highest2 = data->btLevel;
                masteryId2 = it->first;
            }
        }
        const SCommonData* playerCommonData = g_pMyPlayerObj->GetCommonData();
        if (playerCommonData == NULL)
            return;

        buf << playerCommonData->RefObjectId;
        buf << g_pMyPlayerObj->GetCurrentLevel();
        buf << masteryId1;
        buf << masteryId2;
        SendMsg(buf);
    }
    else if (type == 3)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->Clear();
        int Race;
        msg >> Race;

        byte Charlevel;
        msg >> Charlevel;

        int Mastery1;
        msg >> Mastery1;

        int Mastery2;
        msg >> Mastery2;


          g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->WriteLine(1, Race, Charlevel, Mastery1, Mastery2);
          g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->ShowGWnd(true);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LiveUniqueDPS(CMsgStreamBuffer &msg)
{
    std::n_string DMG;
    std::n_string PlayerID;
    int MobID;
    BYTE RecordCount;//BYTE a çevirdim az paket olsun diye :D iyi ypamışsın :D dene bakalım

    std::wstring UniqueName = L"";
    msg >> MobID >> RecordCount;
    BYTE i = 0;

    static const CCharacterData *uqdata = NULL;
    uqdata = g_CGlobalDataManager->GetCharacter(MobID);

    if(uqdata != NULL)
    {
        const std::n_wstring* translatedName =
                g_CTextStringManager->GetString2(uqdata->GetData().NameStrID.c_str());
        if (translatedName != NULL)
            UniqueName = translatedName->c_str();

        CIFDps* dps = g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1);
        CIFUniqueHistory* history =
                g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1);
        if (dps == NULL || history == NULL) {
            msg.FlushRemaining();
            return;
        }

        dps->Clear();
        dps->SetUniqueName(UniqueName.c_str());
        bool test = history->DpsEnabled();

        if (test == true)
        {
            dps->OpenDPS(30000);

        }

        while (i < RecordCount && RecordCount > 0) {
            i++;
            msg >> PlayerID >> DMG;
            if (i <= 8)
                dps->WriteLine(i, TO_WSTRING(PlayerID).c_str(), TO_WSTRING(DMG).c_str());
        }
    }
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateAchievementCondition(CMsgStreamBuffer &msg) {
    int RefAchievementID;
    int RefAchievementConditionID;
    __int64 ProgressCount;
    byte State;
    msg >> RefAchievementID >> RefAchievementConditionID >> ProgressCount >> State;


    if(m_Player->m_Achievements.find(RefAchievementID) != m_Player->m_Achievements.end())
    {
        m_Player->m_Achievements[RefAchievementID] = State;
    }
    if(m_Player->m_AchievementsCondition.find(RefAchievementConditionID) != m_Player->m_AchievementsCondition.end())
    {
        m_Player->m_AchievementsCondition[RefAchievementConditionID].ProgressCount = ProgressCount;
    }
    CIFAchievements* achievements = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)
        : 0;
    if (achievements) {
        achievements->OnListUpdated();
    }

}
void CNetProcessIn::LoadAchievementCondition(CMsgStreamBuffer &msg) {
    int count;
    msg >> count;

    m_Player->m_AchievementsCondition.clear();
    if (count < 0 || count > 2048) {
        msg.FlushRemaining();
        return;
    }

    int i = 0;
    while (i < count) {
        i++;
        int RefAchievementConditionID, RefAchievementID;
        __int64 ProgressCount;
        msg >> RefAchievementConditionID >> RefAchievementID >> ProgressCount;
        CustomCICPlayer::SAchievementsCondition data = CustomCICPlayer::SAchievementsCondition();
        data.RefAchievementConditionID = RefAchievementConditionID;
        data.RefAchievementID = RefAchievementID;
        data.ProgressCount = ProgressCount;
        m_Player->m_AchievementsCondition[data.RefAchievementConditionID] = data;
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LoadAchievement(CMsgStreamBuffer &msg) {
    int count;
    msg >> count;

    m_Player->m_Achievements.clear();
    if (count < 0 || count > 2048) {
        msg.FlushRemaining();
        return;
    }

    int i = 0;
    while (i < count) {
        i++;
            int RefAchievementID;
            byte State;
            msg >> RefAchievementID >> State;

            m_Player->m_Achievements[RefAchievementID] = State;
    }
    msg.FlushRemaining();
}
void CNetProcessIn::AddItemToChest(CMsgStreamBuffer &msg) {

    int ID;
    int ItemID;
    int Quantity;
    std::n_string Date;
    std::n_string Type;
    byte Plus;

    msg >> ID >> ItemID >> Quantity >> Date >> Type >> Plus;

    CIFChest::CharChest Cate = CIFChest::CharChest();
    Cate.DbID = ID;
    Cate.ItemID = ItemID;
    Cate.Quantity = Quantity;
    Cate.Date = TO_NWSTRING(Date);
    Cate.Type = TO_NWSTRING(Type);
    Cate.Plus = Plus;
    CIFChest* chest = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1) : NULL;
    if (chest) {
        chest->my_Chest[ID] = Cate;
        chest->UpdateRanks();
    }


    msg.FlushRemaining();
}
void CNetProcessIn::LoadChest(CMsgStreamBuffer &msg)
{
    CIFChest* chest = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1) : NULL;
    if (!chest) {
        msg.FlushRemaining();
        return;
    }

    int count;
    msg >> count;
    if (count < 0 || count > 500) {
        msg.FlushRemaining();
        return;
    }

    chest->BeginSnapshot(count);

    int i = 0;
    while (i < count && count > 0) {
        i++;
        int ID;
        int ItemID;
        int Quantity;
        std::n_string Date;
        std::n_string Type;
        byte Plus;

        msg >> ID >> ItemID >> Quantity >> Date >> Type >> Plus;

        chest->AddSnapshotItem(
            ID, ItemID, Quantity, TO_NWSTRING(Date), TO_NWSTRING(Type), Plus);
    }
    chest->EndSnapshot();
    msg.FlushRemaining();
}

void CNetProcessIn::LoadChestV2(CMsgStreamBuffer &msg)
{
    CIFChest* chest = g_pCGInterface
        ? g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1) : NULL;
    if (!chest || msg.m_currentReadBytes >= msg.m_availableBytesForReading) {
        msg.FlushRemaining();
        return;
    }

    byte phase;
    msg >> phase;

    if (phase == 0) {
        if (msg.m_availableBytesForReading - msg.m_currentReadBytes < sizeof(int)) {
            msg.FlushRemaining();
            return;
        }

        int expectedCount;
        msg >> expectedCount;
        if (expectedCount < 0 || expectedCount > 100000) {
            msg.FlushRemaining();
            return;
        }
        chest->BeginSnapshot(expectedCount);
    } else if (phase == 1) {
        if (msg.m_currentReadBytes >= msg.m_availableBytesForReading) {
            msg.FlushRemaining();
            return;
        }

        byte count;
        msg >> count;
        if (count > 50) {
            msg.FlushRemaining();
            return;
        }

        for (int i = 0; i < count; ++i) {
            const unsigned int remaining =
                msg.m_currentReadBytes <= msg.m_availableBytesForReading
                    ? msg.m_availableBytesForReading - msg.m_currentReadBytes : 0;
            if (remaining < sizeof(int) * 3 + sizeof(unsigned short) * 2 + sizeof(byte)) {
                msg.FlushRemaining();
                return;
            }

            int ID;
            int ItemID;
            int Quantity;
            std::n_string Date;
            std::n_string Type;
            byte Plus;
            msg >> ID >> ItemID >> Quantity >> Date >> Type >> Plus;
            chest->AddSnapshotItem(
                ID, ItemID, Quantity, TO_NWSTRING(Date), TO_NWSTRING(Type), Plus);
        }
    } else if (phase == 2) {
        chest->EndSnapshot();
    }

    msg.FlushRemaining();
}

void CNetProcessIn::LoadLuckySpinRewards(CMsgStreamBuffer &msg)
{
    int count;
    msg >> count;
    m_CustomDataManager->LuckySpinRewards.clear();

    for (int i = 0; i < count; ++i) {
        CustomDataManager::LuckySpinReward reward;
        msg >> reward.ItemID >> reward.Amount;
        if (reward.ItemID > 0 && reward.Amount > 0) {
            m_CustomDataManager->LuckySpinRewards.push_back(reward);
        }
    }

    CIFLuckySpinWnd* window = g_pCGInterface->GetGuiFromList<CIFLuckySpinWnd>(LUCKY_SPIN_WINDOW_ID);
    if (window && window->IsVisible()) {
        window->RefreshRewards();
    }
    msg.FlushRemaining();
}

void CNetProcessIn::LuckySpinResult(CMsgStreamBuffer &msg)
{
    int winningSlot;
    msg >> winningSlot;
    CIFLuckySpinWnd* window = g_pCGInterface->GetGuiFromList<CIFLuckySpinWnd>(LUCKY_SPIN_WINDOW_ID);
    if (window) {
        window->StartSpin(winningSlot);
    }
    msg.FlushRemaining();
}

void CNetProcessIn::TradeSellCaptchaRequest(CMsgStreamBuffer &msg)
{
    int code;
    int timeoutSeconds;
    int attemptsRemaining = 0;
    byte retry = 0;
    msg >> code >> timeoutSeconds;

    if (msg.m_availableBytesForReading - msg.m_currentReadBytes >= sizeof(attemptsRemaining)) {
        msg >> attemptsRemaining;
    }
    if (msg.m_availableBytesForReading - msg.m_currentReadBytes >= sizeof(retry)) {
        msg >> retry;
    }

    CIFTradeCaptchaWnd* window = g_pCGInterface->GetGuiFromList<CIFTradeCaptchaWnd>(TRADE_SELL_CAPTCHA_WINDOW_ID);
    if (window) {
        window->OpenChallenge(code, timeoutSeconds, attemptsRemaining, retry != 0);
    }

    msg.FlushRemaining();
}

void CNetProcessIn::LoadSpecialOffers(CMsgStreamBuffer &msg)
{
    int count;
    msg >> count;
    m_CustomDataManager->SpecialOffers.clear();

    for (int i = 0; i < count; ++i) {
        CustomDataManager::SpecialOfferItem offer;
        msg >> offer.ID;
        msg >> offer.ItemID;
        msg >> offer.ItemCount;
        msg >> offer.MainPrice;
        msg >> offer.SalePrice;
        msg >> offer.PaymentType;
        msg >> offer.PreviewMode;
        msg >> offer.PreviewRefObjID;
        msg >> offer.SortOrder;
        msg >> offer.Title;
        msg >> offer.PreviewImagePath;

        if (offer.ID > 0 && offer.ItemID > 0 && offer.ItemCount > 0 && offer.SalePrice > 0) {
            m_CustomDataManager->SpecialOffers.push_back(offer);
        }
    }

    CIFSpecialOffersWnd* window = g_pCGInterface->GetGuiFromList<CIFSpecialOffersWnd>(SPECIAL_OFFERS_WINDOW_ID);
    if (window && window->IsVisible()) {
        window->RefreshOffers();
    }

    msg.FlushRemaining();
}

void CNetProcessIn::SpecialOfferPurchaseResult(CMsgStreamBuffer &msg)
{
    byte success;
    int offerId;
    std::n_wstring message;
    msg >> success;
    msg >> offerId;
    msg >> message;

    CIFSpecialOffersWnd* window = g_pCGInterface->GetGuiFromList<CIFSpecialOffersWnd>(SPECIAL_OFFERS_WINDOW_ID);
    if (window) {
        window->HandlePurchaseResult(success != 0, offerId, message.c_str());
    } else {
        if (success) {
            g_pCGInterface->ShowMessage_Notice(message);
        } else {
            g_pCGInterface->ShowMessage_Warning(message);
        }
    }

    msg.FlushRemaining();
}

void CNetProcessIn::LoadKillerAnimations(CMsgStreamBuffer &msg)
{
    int count;
    int activeId;
    msg >> count;
    msg >> activeId;

    m_CustomDataManager->KillerAnimations.clear();
    m_CustomDataManager->ActiveKillerAnimationId = activeId;

    for (int i = 0; i < count; ++i) {
        CustomDataManager::KillerAnimation animation;
        byte owned;
        byte active;

        msg >> animation.ID;
        msg >> animation.AnimationID;
        msg >> animation.Price;
        msg >> animation.PaymentType;
        msg >> animation.SortOrder;
        msg >> owned;
        msg >> active;
        msg >> animation.DisplayName;

        animation.Owned = owned != 0;
        animation.IsActive = active != 0;

        if (animation.ID > 0 && KillerAnimationPlayer::IsValidAnimationId(animation.AnimationID)) {
            m_CustomDataManager->KillerAnimations.push_back(animation);
        }
    }

    CIFKillerAnimationWnd* window = g_pCGInterface->GetGuiFromList<CIFKillerAnimationWnd>(KILLER_ANIMATION_WINDOW_ID);
    if (window && window->IsVisible()) {
        window->RefreshAnimations();
    }

    msg.FlushRemaining();
}

void CNetProcessIn::KillerAnimationActionResult(CMsgStreamBuffer &msg)
{
    byte success;
    byte action;
    int animationId;
    std::n_wstring message;

    msg >> success;
    msg >> action;
    msg >> animationId;
    msg >> message;

    CIFKillerAnimationWnd* window = g_pCGInterface->GetGuiFromList<CIFKillerAnimationWnd>(KILLER_ANIMATION_WINDOW_ID);
    if (window) {
        window->HandleActionResult(success != 0, action, animationId, message.c_str());
    } else {
        if (success) {
            g_pCGInterface->ShowMessage_Notice(message);
        } else {
            g_pCGInterface->ShowMessage_Warning(message);
        }
    }

    msg.FlushRemaining();
}

void CNetProcessIn::TriggerKillerAnimation(CMsgStreamBuffer &msg)
{
    unsigned int killerUniqueId;
    int animationId;

    msg >> killerUniqueId;
    msg >> animationId;

    if (g_pMyPlayerObj && g_pMyPlayerObj->GetUniqueId() == killerUniqueId) {
        KillerAnimationPlayer::QueueOnCurrentPlayer(animationId);
    } else if (g_pMyPlayerObj) {
        KillerAnimationPlayer::QueueOnPlayerUniqueId(killerUniqueId, animationId);
    }

    msg.FlushRemaining();
}

void CNetProcessIn::DropLogsResponse(CMsgStreamBuffer &msg)
{
    byte success;
    int totalCount;
    int page;
    int pageSize;
    int rowCount;
    msg >> success;
    msg >> totalCount;
    msg >> page;
    msg >> pageSize;
    msg >> rowCount;

    std::vector<DropLogEntry> rows;
    if (rowCount < 0) {
        rowCount = 0;
    }
    if (rowCount > DROPLOG_VISIBLE_ROWS) {
        rowCount = DROPLOG_VISIBLE_ROWS;
    }

    for (int i = 0; i < rowCount; ++i) {
        DropLogEntry entry;
        msg >> entry.Player;
        msg >> entry.ItemCode;
        msg >> entry.RefObjId;
        msg >> entry.PlusAmount;
        msg >> entry.Monster;
        msg >> entry.Info;
        msg >> entry.Date;
        msg >> entry.Location;
        rows.push_back(entry);
    }

    CIFDropLogsWnd* window = g_pCGInterface->GetGuiFromList<CIFDropLogsWnd>(DROP_LOGS_WINDOW_ID);
    if (window) {
        window->HandleDropLogsResponse(success != 0, totalCount, page, pageSize, rows);
    }

    msg.FlushRemaining();
}

void CNetProcessIn::MonsterPossibleDropsResponse(CMsgStreamBuffer &msg)
{
    byte success;
    int monsterRefObjId;
    int rowCount;
    msg >> success;
    msg >> monsterRefObjId;
    msg >> rowCount;

    if (rowCount < 0) {
        rowCount = 0;
    }
    if (rowCount > 900) {
        rowCount = 900;
    }

    std::vector<int> itemRefObjIds;
    itemRefObjIds.reserve(rowCount);
    for (int i = 0; i < rowCount; ++i) {
        int itemRefObjId;
        msg >> itemRefObjId;
        if (itemRefObjId > 0) {
            itemRefObjIds.push_back(itemRefObjId);
        }
    }

    CIFDropLogWnd* window = g_pCGInterface->GetGuiFromList<CIFDropLogWnd>(DROP_LOG_WINDOW_ID);
    if (window) {
        window->HandlePossibleDropsResponse(success != 0, monsterRefObjId, itemRefObjIds);
    }

    msg.FlushRemaining();
}

void CNetProcessIn::PvpChallengePacket(CMsgStreamBuffer &msg)
{
    byte action;
    msg >> action;

    if (action == 0) {
        __int64 matchId;
        std::n_wstring challengerName;
        __int64 wagerGold;
        int timeoutSeconds;

        msg >> matchId;
        msg >> challengerName;
        msg >> wagerGold;
        msg >> timeoutSeconds;

        CIFPvpChallengeAnswerWnd* window = g_pCGInterface->GetGuiFromList<CIFPvpChallengeAnswerWnd>(PVP_CHALLENGE_ANSWER_WINDOW_ID);
        if (window) {
            window->OpenRequest(matchId, challengerName.c_str(), wagerGold, timeoutSeconds);
        }
    } else if (action == 1) {
        byte success;
        std::n_wstring message;
        msg >> success;
        msg >> message;

        CIFPvpChallengeWnd* window = g_pCGInterface->GetGuiFromList<CIFPvpChallengeWnd>(PVP_CHALLENGE_WINDOW_ID);
        if (window) {
            window->HandleStatus(success != 0, message.c_str());
        }

        if (g_pCGInterface) {
            if (success) {
                g_pCGInterface->ShowMessage_Notice(message);
            } else {
                g_pCGInterface->ShowMessage_Warning(message);
            }
        }
    } else if (action == 2) {
        std::n_wstring message;
        msg >> message;
        if (g_pCGInterface) {
            g_pCGInterface->ShowMessage_Notice(message);
        }
    }

    msg.FlushRemaining();
}


void CNetProcessIn::DeactivateTag(CMsgStreamBuffer &msg) {
    std::n_string CharName167;
    msg >> CharName167;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    if (m_CustomDataManager->_ActiveTags.find(CharName16) != m_CustomDataManager->_ActiveTags.end())
    {
        m_CustomDataManager->_ActiveTags.erase(CharName16);
    }
    CLASSLINK_LOOP_BEGIN(CICUser)
        if (wcscmp(obj->GetName().c_str(), CharName16.c_str()) == 0)
        {
            obj->ChangeName(CharName16);
            obj->SetTitleName(obj->m_hwanlevel);
        }
    CLASSLINK_LOOP_END(CICUser)
    msg.FlushRemaining();
}
void CNetProcessIn::ActivateTag(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;

    std::n_string tagText;
    msg >> CharName167 >> tagText;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    std::n_wstring tag = TO_NWSTRING(tagText);

    if (m_CustomDataManager->_ActiveTags.find(CharName16) == m_CustomDataManager->_ActiveTags.end()) {
        m_CustomDataManager->_ActiveTags.insert(std::make_pair(CharName16, tag));
    } else {
        m_CustomDataManager->_ActiveTags[CharName16] = tag;
    }
    msg.FlushRemaining();
}
void CNetProcessIn::NoticeSystem(CMsgStreamBuffer &msg)
{
    byte operators;
    msg >> operators;
    std::n_wstring Message;
    msg >> Message;
    D3DCOLOR color;
    color = D3DCOLOR_RGBA(0xff, 0xae, 0xc3, 255);
    if (operators == 1) {
        g_pCGInterface->ShowMessage_Quest(Message.c_str());
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());
    }
    else if (operators == 2) {
        g_pCGInterface->ShowMessage_Notice(Message.c_str());
        g_pCGInterface->FUN_00777c30(CHAT_AllGM, Message.c_str(), color, 1);
    }
    else if (operators == 3) {
        g_pCGInterface->ShowMessage_Warning(Message.c_str());
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());
    }
    else if (operators == 4) {
        g_pCGInterface->ShowMessage_Notice(Message.c_str());
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());
    }
    else if (operators == 5) {
        g_pCGInterface->ShowMessage_Quest(Message.c_str());
    }
    else if (operators == 6) {
        g_pCGInterface->ShowMessage_Notice(Message.c_str());
    }
    else if (operators == 7) {
        g_pCGInterface->ShowMessage_Warning(Message.c_str());
    }
    else if (operators == 8) {
        CIFSystemMessage* systemmessage = g_pCGInterface->GetSystemMessageView();
        systemmessage->WriteMessage(255, 0xFFFF00, Message.c_str(), 7, 7);
    }
    else if (operators == 9) {
        g_pCGInterface->ShowLogMessage(color, Message.c_str()); // RED SYSTEM MESSAGE
    }
    else if (operators == 10)
    {
        g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, Message.c_str());// sadece default renk sağ taraf
    }
    else if (operators >= 11 && operators <= 14)
    {
        unsigned char red = 0;
        unsigned char green = 0;
        unsigned char blue = 0;

        switch (operators)
        {
        case 11: // Gold: rewards and important events.
            red = 0xE6;
            green = 0xAD;
            blue = 0x28;
            break;
        case 12: // Purple: VIP and rare achievements.
            red = 0xA9;
            green = 0x70;
            blue = 0xFF;
            break;
        case 13: // Cyan: information and server updates.
            red = 0x35;
            green = 0xCF;
            blue = 0xE8;
            break;
        case 14: // Orange: important non-error warnings.
            red = 0xF0;
            green = 0x78;
            blue = 0x32;
            break;
        }

        D3DCOLOR customColor = D3DCOLOR_ARGB(255, red, green, blue);
        g_pCGInterface->ShowMessage_ColoredNotice(Message, red, green, blue);
        g_pCGInterface->FUN_00777c30(CHAT_AllGM, Message.c_str(), customColor, 1);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::LoadEventSchedule(CMsgStreamBuffer &msg)
{
    byte count;
    msg >> count;

    byte i = 0;
    while (i < count && count > 0) {
        i++;
        int ID;
        std::n_wstring EventName;
        byte Day;
        std::n_wstring Time;
        msg >> ID >> EventName >> Day >> Time;


        CIFEventSchedule::EventSchedule Cate = CIFEventSchedule::EventSchedule();
        Cate.ID = ID;
        Cate.EventName = EventName.c_str();
        Cate.Day = Day;
        Cate.Time = Time.c_str();
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->EventScheduleList.push_back(Cate);
    }
    g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->UpdateList();
    msg.FlushRemaining();
}
void CNetProcessIn::LoadEventRegister(CMsgStreamBuffer &msg)
{
    byte count;
    msg >> count;

    byte i = 0;
    while (i < count && count > 0) {
        i++;
        byte ID;
        std::n_wstring EventName;
        std::n_wstring EventDesc;
        msg >> ID >> EventName >> EventDesc;

        CIFEventRegister::Events Cate = CIFEventRegister::Events();
        Cate.DBID = ID;
        Cate.EventName = EventName.c_str();
        Cate.EventDesc = EventDesc.c_str();
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->EventList.push_back(Cate);
    }
    g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->UpdateEvents();
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateUniqueHistory(CMsgStreamBuffer &msg)
{

    int UniqueID;
    std::n_wstring KillerName;
    byte State;
    __int64 time;
    int RegID, WorldID, MapIndex;
    float PosX, PosZ, PosY;
    byte MapType;
    msg >> UniqueID >> KillerName >> State >> time >> RegID >> PosX >> PosY >> PosZ
        >> WorldID >> MapType >> MapIndex;

    CIFUniqueHistory* historyWnd =
            g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1);
    if (historyWnd == NULL) {
        msg.FlushRemaining();
        return;
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList.find(UniqueID)
       != g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList.end())
    {
        const CCharacterData *uqdata = NULL;
        uqdata = g_CGlobalDataManager->GetCharacter(UniqueID);

        std::wstring UniqueName = KmtGetText(L"UIIT_KMT_UNKNOWN");

        if (uqdata != NULL) {
            const std::n_wstring* translatedName =
                    g_CTextStringManager->GetString2(uqdata->GetData().NameStrID.c_str());
            if (translatedName != NULL)
                UniqueName = translatedName->c_str();
        }
        if(State == 1)
        {
            g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].Killer = KmtGetText(L"UIIT_KMT_NONE");
        }
        else
        {
            g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].Killer = KillerName.c_str();
        }
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].UniqueName = UniqueName;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].Status = State;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].Elapsedtime = time;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].RegionID = RegID;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].KilledX = PosX;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].KilledZ = PosZ;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].KilledY = PosY;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].WorldID = WorldID;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].MapType = MapType;
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList[UniqueID].MapIndex = MapIndex;
        g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->UniqueHistoryList[UniqueID].DpsMeter.clear();
    }
    else
    {
        const CCharacterData *uqdata = NULL;
        uqdata = g_CGlobalDataManager->GetCharacter(UniqueID);

        std::wstring UniqueName = KmtGetText(L"UIIT_KMT_UNKNOWN");

        if (uqdata != NULL) {
            const std::n_wstring* translatedName =
                    g_CTextStringManager->GetString2(uqdata->GetData().NameStrID.c_str());
            if (translatedName != NULL)
                UniqueName = translatedName->c_str();
        }

        CIFUniqueHistory::UniqueHistory New = CIFUniqueHistory::UniqueHistory();
        New.UniqueName = UniqueName;
        New.Status = State;
        New.Elapsedtime = time;
        New.RegionID = RegID;
        New.KilledX = PosX;
        New.KilledY = PosY;
        New.KilledZ = PosZ;
        New.WorldID = WorldID;
        New.MapType = MapType;
        New.MapIndex = MapIndex;
        if(State == 1)
        {
            New.Killer = KmtGetText(L"UIIT_KMT_NONE");
        }
        else
        {
            New.Killer = KillerName.c_str();
        }
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList.insert(std::make_pair(UniqueID, New));
    }

    byte DpsCount;
    msg >> DpsCount;

    byte i = 0;
    while (i < DpsCount && DpsCount > 0) {
        i++;
        std::n_string Names, Value;
        msg >> Names >> Value;

        if(g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->UniqueHistoryList.find(UniqueID)
           != g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->UniqueHistoryList.end())
        {
            g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->
                    UniqueHistoryList[UniqueID].DpsMeter.push_back(std::make_pair(TO_NWSTRING(Names), TO_NWSTRING(Value)));
        }
    }

    if (historyWnd->SelectedUniqueID == UniqueID)
        historyWnd->ClearSelection();
    historyWnd->UpdateRanks(false);
    msg.FlushRemaining();
}
void CNetProcessIn::LoadUniqueHistory(CMsgStreamBuffer &msg)
{
    CIFUniqueHistory* historyWnd =
            g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1);
    if (historyWnd == NULL) {
        msg.FlushRemaining();
        return;
    }
    if (historyWnd->SelectedUniqueID != 0)
        historyWnd->ClearSelection();
    BYTE sCount;
    msg >> sCount;
    BYTE i = 0;
    historyWnd->UniqueHistoryList.clear();

    while (i < sCount && sCount > 0) {
        i++;
        int UniqueID;
        std::n_wstring KillerName;
        byte State;
        __int64 time;
        int RegID, WorldID, MapIndex;
        float PosX, PosZ, PosY;
        byte MapType;
        msg >> UniqueID >> KillerName >> State >> time >> RegID >> PosX >> PosY >> PosZ
            >> WorldID >> MapType >> MapIndex;


        const CCharacterData *uqdata = NULL;
        uqdata = g_CGlobalDataManager->GetCharacter(UniqueID);

        std::wstring UniqueName = KmtGetText(L"UIIT_KMT_UNKNOWN");

        if (uqdata != NULL) {
            const std::n_wstring* translatedName =
                    g_CTextStringManager->GetString2(uqdata->GetData().NameStrID.c_str());
            if (translatedName != NULL)
                UniqueName = translatedName->c_str();
        }

        CIFUniqueHistory::UniqueHistory New = CIFUniqueHistory::UniqueHistory();
        New.UniqueName = UniqueName;
        New.Status = State;
        New.Elapsedtime = time;
        New.RegionID = RegID;
        New.KilledX = PosX;
        New.KilledY = PosY;
        New.KilledZ = PosZ;
        New.WorldID = WorldID;
        New.MapType = MapType;
        New.MapIndex = MapIndex;
        New.DpsMeter = std::vector<std::pair<std::n_wstring, std::n_wstring> >();
        if(State == 1)
        {
            New.Killer = KmtGetText(L"UIIT_KMT_NONE");
        }
        else
        {
            New.Killer = KillerName.c_str();
        }
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->UniqueHistoryList.insert(std::make_pair(UniqueID, New));


        byte DpsCount;
        msg >> DpsCount;

        byte j = 0;
        while (j < DpsCount && DpsCount > 0) {
            j++;
            std::n_string Names, Value;
            msg >> Names >> Value;
            if(g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->UniqueHistoryList.find(UniqueID)
               != g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->UniqueHistoryList.end())
            {
                g_pCGInterface->GetGuiFromList<CIFUniqueHistory>(UniqueHistoryID)->
                        UniqueHistoryList[UniqueID].DpsMeter.push_back(std::make_pair(TO_NWSTRING(Names), TO_NWSTRING(Value))); }
        }

    }

    historyWnd->UpdateRanks(true);
    msg.FlushRemaining();
}
void CNetProcessIn::RankCategories(CMsgStreamBuffer &msg)
{
    byte Count;
    msg >> Count;
    CIFDynamicRanking* ranking =
            g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1);
    if (ranking)
        ranking->ClearCategories();

    int i = 0;
    while (i < Count && Count > 0) {
        i++;


        int ID;
        msg >> ID;

        std::n_string Categorys;
        msg >> Categorys;
        std::n_wstring Category = TO_NWSTRING(Categorys);


        if (ranking)
            ranking->AddCategory(ID, Category);

    }
    if (ranking)
        ranking->FinishCategories();
    msg.FlushRemaining();
}
void CNetProcessIn::LoadRank(CMsgStreamBuffer &msg)
{
    byte count;
    msg >> count;

    CIFDynamicRanking* ranking =
            g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1);
    ranking->RankList.clear();

    for (byte i = 0; i < count; ++i)
    {
        std::n_string charName;
        std::n_string guildName;
        int points;
        msg >> charName >> guildName >> points;

        CIFDynamicRanking::RankStruct entry = CIFDynamicRanking::RankStruct();
        entry.LineNum = i + 1;
        entry.Charname = TO_NWSTRING(charName).c_str();
        entry.Guild = TO_NWSTRING(guildName).c_str();
        entry.Points = Insert(points);
        ranking->RankList.push_back(entry);
    }

    ranking->UpdateRanks();
    msg.FlushRemaining();
}
void CNetProcessIn::LoadOwnRank(CMsgStreamBuffer &msg)
{
    std::n_string charName;
    int rank;
    int points;
    msg >> charName >> rank >> points;

    CIFDynamicRanking* ranking =
            g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1);
    ranking->UpdateSelfRank(TO_NWSTRING(charName), rank, points);
    msg.FlushRemaining();
}
void CNetProcessIn::RemoveCharacterIconRight(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    msg >> CharName167;
    std::n_wstring CharName16 = TO_NWSTRING(CharName167);
    if (m_CustomDataManager->m_RightCharIcons.find(CharName16) != m_CustomDataManager->m_RightCharIcons.end())
    {
        m_CustomDataManager->m_RightCharIcons.erase(CharName16);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateCharacterIconRight(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    int IconID;
    msg >> CharName167 >> IconID;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);
    if (m_CustomDataManager->m_RightCharIcons.find(CharName16) != m_CustomDataManager->m_RightCharIcons.end()) {
        m_CustomDataManager->m_RightCharIcons[CharName16] = IconID;
    } else {
        m_CustomDataManager->m_RightCharIcons.insert((std::make_pair(CharName16, IconID)));
    }
    msg.FlushRemaining();
}
void CNetProcessIn::RemoveCharacterIconLeft(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    msg >> CharName167;
    std::n_wstring CharName16 = TO_NWSTRING(CharName167);
    if (m_CustomDataManager->m_LeftCharIcons.find(CharName16) != m_CustomDataManager->m_LeftCharIcons.end())
    {
        m_CustomDataManager->m_LeftCharIcons.erase(CharName16);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateCharacterIconLeft(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    int IconID;
    msg >> CharName167 >> IconID;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);
    if (m_CustomDataManager->m_LeftCharIcons.find(CharName16) != m_CustomDataManager->m_LeftCharIcons.end()) {
        m_CustomDataManager->m_LeftCharIcons[CharName16] = IconID;
    } else {
        m_CustomDataManager->m_LeftCharIcons.insert((std::make_pair(CharName16, IconID)));
    }
    msg.FlushRemaining();
}

void CNetProcessIn::AddIconToIconManager(CMsgStreamBuffer &msg)
{
    int DBID;
    msg >> DBID;
    std::n_string MediaPath;
    msg >> MediaPath;
    byte Side;
    msg >> Side;
    CIFIconManager::IconStruct IconStruct =  CIFIconManager::IconStruct();
    IconStruct.DBID = DBID;
    IconStruct.side = Side;
    IconStruct.MediaPath = TO_NWSTRING(MediaPath);
    g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IconList.push_back(IconStruct);
    g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->Clear();
    g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ClearDDJ();
    g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->LoadItems();
    msg.FlushRemaining();
}
void CNetProcessIn::CharacterIconManager(CMsgStreamBuffer &msg)
{
    byte Count;
    msg >> Count;

    int i = 0;
    while (i < Count && Count > 0) {
        i++;

        int DBID;
        msg >> DBID;

        std::n_string MediaPath;
        msg >> MediaPath;

        byte Side;
        msg >> Side;
        CIFIconManager::IconStruct IconStruct =  CIFIconManager::IconStruct();
        IconStruct.DBID = DBID;
        IconStruct.side = Side;
        IconStruct.MediaPath = TO_NWSTRING(MediaPath);
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IconList.push_back(IconStruct);
    }
    g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->LoadItems();
    msg.FlushRemaining();
}
void CNetProcessIn::RemoveTitleColor(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    msg >> CharName167;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    if (m_CustomDataManager->_ActiveTitleColors.find(CharName16) != m_CustomDataManager->_ActiveTitleColors.end())
    {
        m_CustomDataManager->_ActiveTitleColors.erase(CharName16);
    }
    CLASSLINK_LOOP_BEGIN(CICUser)
        if (wcscmp(obj->GetName().c_str(), CharName16.c_str()) == 0)
            obj->ChangeTitleColor(0xFFE65B);
    CLASSLINK_LOOP_END(CICUser)
    msg.FlushRemaining();
}
void CNetProcessIn::UpdateTitleColor(CMsgStreamBuffer &msg)
{
    std::n_string CharName167;
    unsigned __int32 NewColor;
    msg >> CharName167 >> NewColor;

    std::n_wstring CharName16 = TO_NWSTRING(CharName167);

    CLASSLINK_LOOP_BEGIN(CICUser)
        if (wcscmp(obj->GetName().c_str(), CharName16.c_str()) == 0)
            obj->ChangeTitleColor(NewColor);
    CLASSLINK_LOOP_END(CICUser)

    if (m_CustomDataManager->_ActiveTitleColors.find(CharName16.c_str()) != m_CustomDataManager->_ActiveTitleColors.end()) {
        m_CustomDataManager->_ActiveTitleColors[CharName16] = NewColor;
    } else {

        m_CustomDataManager->_ActiveTitleColors.insert((std::make_pair(CharName16, NewColor)));
    }
    msg.FlushRemaining();
}
void CNetProcessIn::AddColorToTitleManagerColor(CMsgStreamBuffer &msg)
{
    int DBID;
    std::n_string ColorName;
    unsigned __int32 ColorCode;

    msg >> DBID >> ColorName >> ColorCode;


    CIFTitleManager::ColorStruct ColorStruct =  CIFTitleManager::ColorStruct();
    ColorStruct.DBID = DBID;
    ColorStruct.ColorName = TO_WSTRING(ColorName);
    ColorStruct.ColorCode = ColorCode; /// yellow
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ColorList.push_back(ColorStruct);
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ActivateTabPage(1);
    msg.FlushRemaining();
}
void CNetProcessIn::AddTitleToTitleManager(CMsgStreamBuffer &msg)
{
    byte TitleID;
    std::n_string TitleName;
    msg >> TitleID >> TitleName;

    const std::n_wstring &regionname = g_CTextStringManager->GetString(TO_WSTRING(TitleName).c_str());
    std::wstring TitleNames;

    TitleNames = regionname.c_str();
    if (TitleNames.empty())
        TitleNames = KmtGetText(L"UIIT_KMT_UNKNOWN");
    CIFTitleManager::TitleStruct TitleStruct =  CIFTitleManager::TitleStruct();
    TitleStruct.TitleID = TitleID;
    TitleStruct.TitleName = TitleNames;
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->TitleList.push_back(TitleStruct);
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ActivateTabPage(0);
    msg.FlushRemaining();
}
void CNetProcessIn::CharTitleManagerColors(CMsgStreamBuffer &msg)
{
    byte Count;
    msg >> Count;

    int i = 0;
    while (i < Count && Count > 0) {
        i++;
        int DBID;
        std::n_string ColorName;
        unsigned __int32 ColorCode;
        msg >> DBID;
        msg >> ColorName;
        msg >> ColorCode;
        CIFTitleManager::ColorStruct ColorStruct =  CIFTitleManager::ColorStruct();
        ColorStruct.DBID = DBID;
        ColorStruct.ColorName = TO_WSTRING(ColorName);
        ColorStruct.ColorCode = ColorCode; /// yellow
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ColorList.push_back(ColorStruct);
    }
    msg.FlushRemaining();
}
void CNetProcessIn::CharTitleManager(CMsgStreamBuffer &msg) {
    byte Count;
    msg >> Count;

    int i = 0;
    while (i < Count && Count > 0) {
        i++;
        byte TitleID;
        std::n_string TitleName;
        msg >> TitleID >> TitleName;

        std::stringstream Reg;
        Reg << TitleName;
        const std::n_wstring &regionname = g_CTextStringManager->GetString(TO_WSTRING(TitleName).c_str());
        std::wstring TitleNames;

        TitleNames = regionname.c_str();
        if (TitleNames.empty())
            TitleNames = L"</>";
        CIFTitleManager::TitleStruct TitleStruct =  CIFTitleManager::TitleStruct();
        TitleStruct.TitleID = TitleID;
        TitleStruct.TitleName = TitleNames;
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->TitleList.push_back(TitleStruct);
    }
    g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ActivateTabPage(0);
    msg.FlushRemaining();
}
void CNetProcessIn::OnUpdateGameTime(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()

    CPSMission *pM = (CPSMission *) theApp.GetCurrentProcess();

    msg >> pM->m_wDay >> pM->m_byHour >> pM->m_byMin;

    if (g_pCGInterface->field_0x7cd)
        return;

    CLocalTime_Sub localTimeSub;

    Fun_GetCfgGame()->m_LocalTime.FUN_008b01b0(&localTimeSub);

    Fun_GetCfgGame()->m_LocalTime.InitTimer(pM->m_dwRealTime, pM->m_wDay, pM->m_byHour, pM->m_byMin, 0);
}

void CNetProcessIn::On3809(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6f50)(this, msg);
}

void CNetProcessIn::OnResetClient(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086dab0)(this, msg);
}

void CNetProcessIn::OnLogoutAck(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086db40)(this, msg);
}

void CNetProcessIn::Onb006(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086dbd0)(this, msg);
}

void CNetProcessIn::On300a(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086d550)(this, msg);
}

void CNetProcessIn::On3405(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086e5b0)(this, msg);
}

void CNetProcessIn::On34b5(CMsgStreamBuffer &msg) {

    m_Player->m_FellowSkillData.clear();

    if(g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1)->ShowGWnd(false);
    }

    g_pCGInterface->OnItemMallSectionControl(false);
    if (g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFCounterWnd>(CounterWndNew, 1)->ShowGWnd(false);

    }

    if (g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFFortressWar>(FtwKillCounter, 1)->ShowGWnd(false);
    }

    if (g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->OnCloseWnd();
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFSettings>(1951, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSettings>(1951, 1)->ShowGWnd(false);
    }

    if (m_Settings && m_Settings->EnableNewItemMall)
    {
        CIFVAvatarMallBuyItemList* avatarBuyList =
                g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMallBuyItemList>(AvatarMallBuyListId, 1);
        if (avatarBuyList && avatarBuyList->IsVisible())
            avatarBuyList->ShowGWnd(false);

        CIFVItemMallBuyItem* itemMallBuy =
                g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1);
        if (itemMallBuy && itemMallBuy->IsVisible())
            itemMallBuy->ShowGWnd(false);

        CIFVSelectMall* selectMall =
                g_pCGInterface->m_IRM.GetResObj<CIFVSelectMall>(SelectMallId, 1);
        if (selectMall && selectMall->IsVisible())
            selectMall->ShowGWnd(false);

        CIFVItemMall* itemMall =
                g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1);
        if (itemMall && itemMall->IsVisible())
            itemMall->OnCloseWnd();
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFKillCounter>(1382, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFTeamCounter>(TeamCounterID, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->Clear();
        g_pCGInterface->m_IRM.GetResObj<CIFJobCounter>(JobCounterID, 1)->ShowGWnd(false);

    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFItemLocker>(ItemLocker, 1)->OnCloseWnd();
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFItemUnlocker>(Itemunlocker, 1)->OnCloseWnd();
    }
    else if( g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFNewMsgBox>(1394, 1)->ShowGWnd(false);
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->OnCloseWnd();
    }

    if(g_pCGInterface->m_IRM.GetResObj<CIFSocial>(SocialWndID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSocial>(SocialWndID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1)->OnCloseWnd();
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFGrantName>(GrantNameID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFTitleManager>(TitleManagerID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFIconManager>(IconManagerID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDynamicRanking>(DynamicRankingID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->OnCloseWnd();
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventRegister>(EventRegisterID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFEventSchedule>(EventScheduleID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAchievements>(AchievementsID, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->ShowGWnd(false);
        m_Player->ReverseSlot = 9999;
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFCustomMessageBox>(CustomMessageBox, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFCustomMessageBox>(CustomMessageBox, 1)->ShowGWnd(false);
    }
    if(g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFSavedLocation>(SavedLocation, 1)->ShowGWnd(false);
    }
    if (g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFChangelog>(ChangelogID, 1)->ShowGWnd(false);
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0086e100)(this, msg);
}

void CNetProcessIn::On300c(CMsgStreamBuffer &msg) {
    unsigned short type = 0;
    msg >> type;
    if(type == 0x0c1b)
    {
        unsigned short WorldID;
        msg >> WorldID;

        std::wstring WorldCode;
        g_CGlobalDataManager->GetWorldCodeByID(&WorldCode, WorldID);
        wchar_t buffer[0x100];
        if (WorldCode.compare(L"INS_BOSS_DUNGEON") == 0)
        {
           g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"UIIT_MSG_BOSS_DUNGEON_PARTY_MASTER_JOIN"));
           g_pCGInterface->ShowLogMessage(DEFAULT_SYSTEM_COLOR, TSM_GETTEXTPTR(L"UIIT_MSG_BOSS_DUNGEON_PARTY_MASTER_JOIN"));
            msg.FlushRemaining();
            return;
        }
    }
    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00874ea0)(this, msg);
}

void CNetProcessIn::On34a5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6580)(this, msg);
}

void CNetProcessIn::OnMyCharacterData(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a8c00)(this, msg);
}

void CNetProcessIn::On34a6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008abba0)(this, msg);
}

void CNetProcessIn::On3020(CMsgStreamBuffer &msg) {
    if (m_Settings->EnableServerInfoSkill) {
        CIFMagicStateBoard *test = g_pCGInterface->m_IRM.GetResObj<CIFMagicStateBoard>(22, 1);
        test->FUN_008048c0(548, 2340, 0, 0, 0, 0, 372179968, 116, 124, 136, 0, 15, 0);
    }

    if (theApp.GetCurrentProcess() != NULL) {
        std::n_wstring cn16 = g_pMyPlayerObj->GetCharName().c_str();;
        std::n_wstring charname = L"[ " + cn16 + L" ]";
        std::n_wstring tag = L" KMTGuard";
        std::n_wstring finaltext = charname + tag;
        SetWindowText(theApp.GetHWnd(), TO_STRING(finaltext).c_str());
        SetWindowLong(theApp.GetHWnd(), GWL_STYLE,
                      GetWindowLong(theApp.GetHWnd(), GWL_STYLE) | WS_MINIMIZEBOX);
    }
    if (g_pMyPlayerObj != NULL) {
        m_dc->UpdateState(IN_GAME);
    }
    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6dc0)(this, msg);
}

void CNetProcessIn::OnObjectListBegin(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableUniqueTarget)
    {
        msg >> m_CustomDataManager->g_GroupSpawn_Type;
        msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ac0e0)(this, msg);
}

void CNetProcessIn::OnObjectListData(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableUniqueTarget)
    {
        for (size_t i = 0; i < (msg.m_availableBytesForReading / 4); i++)
        {
            DWORD dead_object;
            msg >> dead_object;
            m_CustomDataManager->g_despawned_objects.push_back(dead_object);
        }
    msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ac170)(this, msg);
}


void CNetProcessIn::OnObjectListEnd(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableUniqueTarget)
    {
        if (m_CustomDataManager->g_GroupSpawn_Type == 2)
        {
            for (std::vector<DWORD>::iterator shit = m_CustomDataManager->g_despawned_objects.begin(); shit != m_CustomDataManager->g_despawned_objects.end(); shit++)
            {
                g_pMyPlayerObj->RemoveExistingUniqueData(*shit);
            }
            m_CustomDataManager->g_despawned_objects.clear();
            std::vector<DWORD>(m_CustomDataManager->g_despawned_objects.begin(), m_CustomDataManager->g_despawned_objects.end()).swap(m_CustomDataManager->g_despawned_objects);
        }
        msg.m_currentReadBytes = 0;
    }

    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ac3d0)(this, msg);
}

void CNetProcessIn::OnChestDataBegin(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a74f0)(this, msg);
}

void CNetProcessIn::OnChestDataData(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a8c10)(this, msg);
}

void CNetProcessIn::OnChestDataEnd(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a9100)(this, msg);
}

void CNetProcessIn::OnObjectEntered(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6ca0)(this, msg);
}

void CNetProcessIn::OnObjectLeft(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableUniqueTarget)
    {
    if (g_pMyPlayerObj)
    {
        DWORD dead_object;
        msg >> dead_object;
        g_pMyPlayerObj->RemoveExistingUniqueData(dead_object);
        /* for(std::map<unsigned __int32, int>::iterator test = my_Maps->EventSuit.begin(); test != my_Maps->EventSuit.end(); test++)
        {
            my_Maps->EventSuit.erase(dead_object);
        }*/

    }
    msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6cc0)(this, msg);
}

void CNetProcessIn::On3076(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a6640)(this, msg);
}

void CNetProcessIn::On30bf(CMsgStreamBuffer &msg) {


    unsigned int uniqueId;
    msg >> uniqueId;
    if(g_pMyPlayerObj != NULL)
    {
        if(g_pMyPlayerObj->GetUniqueId() == uniqueId)
        {
            byte updateType, updateState;
            msg >> updateType >> updateState;
            if(updateType == 4)
            {
                if(updateState == 1) /// ZERK MATE
                {
                    for(std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.begin();
                        it != g_pMyPlayerObj->CCOSDataMgr->CosList.end(); ++it)
                    {
                        CICCos *pUser = (CICCos*)GetCharacterObjectByID_MAYBE(it->first);
                        if (pUser != NULL) {
                            const SCommonData* commonData = pUser->GetCommonData();
                            if (commonData == NULL)
                                continue;

                            static const CCharacterData *data = NULL;
                            data = g_CGlobalDataManager->GetCharacter(commonData->RefObjectId);
                            if(data)
                            {
                                std::n_wstring NameStr = commonData->NameStrID;
                                std::map<std::n_wstring, CustomDataManager::FellowPetStruct>::iterator fellow =
                                    m_CustomDataManager->m_RefFellowPetSystem.find(NameStr);

                                if(fellow != m_CustomDataManager->m_RefFellowPetSystem.end() &&
                                   !fellow->second.NameStrID.empty() &&
                                   fellow->second.SelfSkill_1 > 0 &&
                                   data->GetData().Level >= fellow->second.SelfSkill_Active_Level_1)
                                {
                                    CMsgStreamBuffer buf(0x169A);
                                    buf << byte(18);
                                    buf << it->first;
                                    buf << data->GetData().Level;
                                    //printf("%d \n", it->first);
                                    SendMsg(buf);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a80a0)(this, msg);
}

void CNetProcessIn::Onb0cb(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a7720)(this, msg);
}

void CNetProcessIn::Onb03c(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a8ea0)(this, msg);
}

void CNetProcessIn::On3092(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a7550)(this, msg);
}

void CNetProcessIn::On30df(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008a7630)(this, msg);
}

void CNetProcessIn::On30cd(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881090)(this, msg);
}

void CNetProcessIn::On30ce(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881100)(this, msg);
}

void CNetProcessIn::On30d3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881170)(this, msg);
}

void CNetProcessIn::On30C1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A8FC0)(this, msg);
}

void CNetProcessIn::On30C2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A9000)(this, msg);
}

void CNetProcessIn::On30C3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7870)(this, msg);
}

void CNetProcessIn::On30C8(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A6650)(this, msg);
}

void CNetProcessIn::OnB0C6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7A70)(this, msg);
}

void CNetProcessIn::On30C9(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008AA340)(this, msg);
}

void CNetProcessIn::On30E7(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00883B10)(this, msg);
}

void CNetProcessIn::OnB0C7(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7AC0)(this, msg);
}

void CNetProcessIn::On30CA(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7690)(this, msg);
}

void CNetProcessIn::OnB116(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7B30)(this, msg);
}

void CNetProcessIn::OnB117(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7B80)(this, msg);
}

void CNetProcessIn::OnB420(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7D20)(this, msg);
}

void CNetProcessIn::On30D4(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00889D90)(this, msg);
}

void CNetProcessIn::On3054(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7020)(this, msg);
}

void CNetProcessIn::On3056(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A97C0)(this, msg);
}

void CNetProcessIn::On304E(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A70D0)(this, msg);
}

void CNetProcessIn::On3052(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ADF50)(this, msg);
}

void CNetProcessIn::On3057(CMsgStreamBuffer &msg) {
    bool hasTargetHpUpdate = false;
    unsigned int targetHpObjectId = 0;
    unsigned int targetHpValue = 0;

    if (msg.m_availableBytesForReading >= 11) {
        unsigned int savedReadBytes = msg.m_currentReadBytes;
        unsigned __int32 uniqueId;
        unsigned __int16 updateSource;
        byte updateFlag;

        msg >> uniqueId;
        msg >> updateSource;
        msg >> updateFlag;

        if (updateFlag == 1) {
            int charHp;
            msg >> charHp;

            hasTargetHpUpdate = true;
            targetHpObjectId = uniqueId;
            targetHpValue = charHp > 0 ? static_cast<unsigned int>(charHp) : 0;
        }

        msg.m_currentReadBytes = savedReadBytes;
    }

    CIFMacroMenu* macroMenu = GetReadyMacroMenu();
    if(m_Settings->EnableMacro && macroMenu && g_pMyPlayerObj)
    {
        unsigned __int32 uniqueId;
        msg >> uniqueId;
        unsigned __int16 updateSource;
        msg >> updateSource;
        byte updateFlag;
        msg >> updateFlag;
        if (updateFlag == 1)// HP
        {
            int CharHp;
            msg >> CharHp;
            if (uniqueId == g_pMyPlayerObj->GetUniqueId())
            {
                if(!macroMenu->AutoPotionSlot->CharacterHpTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterHP();
                }
                if(!macroMenu->AutoPotionSlot->CharacterVigorTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterVigorHP();
                }
            }
        }
        if (updateFlag == 2)// MP
        {
            int CharMp;
            msg >> CharMp;
            if (uniqueId == g_pMyPlayerObj->GetUniqueId())
            {
                if(!macroMenu->AutoPotionSlot->CharacterMpTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterMP();
                }
                if(!macroMenu->AutoPotionSlot->CharacterVigorTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterVigorMP();
                }
            }
        }
        if(updateFlag == 4)
        {

            byte effect;
            msg >> effect;
            if (uniqueId == g_pMyPlayerObj->GetUniqueId())
            {
                if(!macroMenu->AutoPotionSlot->CharacterPillTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterPILL();
                }

                if(!macroMenu->AutoPotionSlot->CharacterPillTimerRunningPuri)
                {
                    macroMenu->AutoPotionSlot->CheckCharacterPILLPuri();
                }
            }
            if (g_pMyPlayerObj->CCOSDataMgr->CosList.find(uniqueId) !=  g_pMyPlayerObj->CCOSDataMgr->CosList.end())
            {
                if(!macroMenu->AutoPotionSlot->PetPillTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckPetPILL();
                }
            }
        }
        if(updateFlag == 5)
        {
            if (g_pMyPlayerObj->CCOSDataMgr->CosList.find(uniqueId) !=  g_pMyPlayerObj->CCOSDataMgr->CosList.end())
            {
                if(!macroMenu->AutoPotionSlot->PetHpTimerRunning)
                {
                    macroMenu->AutoPotionSlot->CheckPetHP();
                }
            }
        }
        msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A9E30)(this, msg);
    if (hasTargetHpUpdate)
        RefreshVisibleTargetHpPercentForObjectHp(targetHpObjectId, targetHpValue);
    else
        RefreshVisibleTargetHpPercent();
}

void CNetProcessIn::OnB045(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088B200)(this, msg);
}

void CNetProcessIn::On303D(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880F80)(this, msg);
}

void CNetProcessIn::On30D2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ADD60)(this, msg);
}

void CNetProcessIn::On30CF(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008726E0)(this, msg);
}

void CNetProcessIn::On3011(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872710)(this, msg);
}

void CNetProcessIn::On3058(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872740)(this, msg);
}

void CNetProcessIn::OnB025(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00877340)(this, msg);
}

void CNetProcessIn::OnChatReceive(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
#if 1
    std::n_wstring senderW;
    std::n_wstring messageW;
    std::n_string messageA;
    std::n_string senderA;

    int entityId;
    bool bIsFromMyself = false;

    ChatType chatType;

    msg >> chatType;
    //printf("Chat Type: %d\n", chatType);

    switch (chatType) {
        case CHAT_All:
        case CHAT_AllGM: {
            msg >> entityId >> messageA;
         //   printf("Entity ID: %d, MessageA: %s\n", entityId, messageA.c_str());

            messageW = acp_n_decode(messageA);
           // printf("MessageW: %ls\n", messageW.c_str());

            CIGIDObject *pObject = GetCharacterObjectByID_MAYBE(entityId);

            if (pObject == NULL) {
                senderW = L"???";
             //   printf("Sender is unknown.\n");
            } else {
                if (pObject->IsSame(GFX_RUNTIME_CLASS(CICPlayer))) {
               //     printf("Sender is the same as the current player.\n");
                    return;
                }

                senderW = pObject->GetName();
                //printf("SenderW: %ls\n", senderW.c_str());

                if (pObject->IsKindOf(GFX_RUNTIME_CLASS(CICUser))) {
                  //  printf("Job type is %d\n", reinterpret_cast<CICUser *>(pObject)->GetJobType());

                    if (reinterpret_cast<CICUser *>(pObject)->GetJobType() != TRIJOB_NOJOB) {
                        senderW.insert(0, L"*");
                    }
                }
            }
            break;
        }

        default: {
            msg >> senderA >> messageA;
            //printf("SenderA: %s, MessageA: %s\n", senderA.c_str(), messageA.c_str());

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (senderW == g_pMyPlayerObj->GetCharName()) {
              //  printf("Message is from myself, ignoring.\n");
                return;
            }

//            printf("SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }

        case CHAT_Party : {
            msg >> senderA >> messageA;
  //          printf("Party - SenderA: %s, MessageA: %s\n", senderA.c_str(), messageA.c_str());

            senderW = acp_n_decode(senderA);
            messageW = acp_n_decode(messageA);

            if (g_pMyPlayerObj->GetName() == senderW) {
    //            printf("Message is from myself, ignoring.\n");
                return;
            }

            if(g_pMyPlayerObj->GetJobType() != TRIJOB_NOJOB) {
                senderW.insert(0, L"*");
            }

      //      printf("Party - SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }

        case CHAT_Global: {
            msg >> senderA >> messageA;
        //    printf("Global - SenderA: %s, MessageA: %s\n", senderA.c_str(), messageA.c_str());

            messageW = acp_n_decode(messageA);

            if (senderA.length() == 0) {
                senderW = L"???";
            } else {
                senderW = acp_n_decode(senderA);
            }

            if (g_pMyPlayerObj->GetCharName() == senderW) {
                bIsFromMyself = true;
            }

          //  printf("Global - SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }

        case CHAT_Notice: {
            msg >> messageA;
            //printf("Notice - MessageA: %s\n", messageA.c_str());
            messageW = acp_n_decode(messageA);
            bIsFromMyself = false;
            break;
        }

        case CHAT_Stall: {
            msg >> senderA >> messageA;
            //printf("Stall - SenderA: %s, MessageA: %s\n", senderA.c_str(), messageA.c_str());

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (g_pMyPlayerObj->GetName() == senderW) {
                bIsFromMyself = true;
            }

            //printf("Stall - SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }

        case CHAT_NPC: {
            msg >> entityId >> messageA;
            //printf("NPC - Entity ID: %d, MessageA: %s\n", entityId, messageA.c_str());

            CIGIDObject *pObject = GetCharacterObjectByID_MAYBE(entityId);

            if (pObject == NULL) {
                senderW = L"???";
              //  printf("Sender is unknown.\n");
            } else {
                if (pObject->IsSame(GFX_RUNTIME_CLASS(CICPlayer))) {
                    senderW = reinterpret_cast<CICPlayer *>(pObject)->GetCharName();
                    bIsFromMyself = true;
                } else {
                    senderW = pObject->GetName();
                }
            }

            messageW = g_CTextStringManager->FUN_008c9bb0(messageW);

            if (pObject->IsKindOf(GFX_RUNTIME_CLASS(CICUser))) {
                chatType = CHAT_All;
            }

            //printf("NPC - SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }

        case CHAT_Academy: {
            msg >> senderA >> messageA;
            //printf("Academy - SenderA: %s, MessageA: %s\n", senderA.c_str(), messageA.c_str());

            messageW = acp_n_decode(messageA);
            senderW = acp_n_decode(senderA);

            if (g_pMyPlayerObj->GetCharName() == senderW) {
              //  printf("Message is from myself, ignoring.\n");
                return;
            }

//            printf("Academy - SenderW: %ls, MessageW: %ls\n", senderW.c_str(), messageW.c_str());
            break;
        }
    }

    if (g_pMyPlayerObj == NULL) {
  //      printf("MyPlayerObj is NULL.\n");
        return;
    }

    if (senderW.length() == 0) {
        senderW = acp_n_decode(senderA);
    }

    if (messageW.length() == 0) {
        messageW = acp_n_decode(messageA);
    }

   /* printf("Final - ChatType: %d, SenderW: %ls, EntityID: %d, MessageW: %ls, FromMyself: %d\n",
           chatType, senderW.c_str(), entityId, messageW.c_str(), bIsFromMyself);
*/
    WriteToChatWindow(chatType, senderW, entityId, messageW, bIsFromMyself);

#else
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00877810)(this, msg);
#endif
}


void CNetProcessIn::On3091(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7950)(this, msg);
}

void CNetProcessIn::On302D(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872010)(this, msg);
}

void CNetProcessIn::OnB021(CMsgStreamBuffer &msg) {
    if(g_pCGInterface->m_IRM.GetResObj<CIFMSFPS>(1952, 1) != NULL)
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMSFPS>(1952, 1)->OnMovementResponse(msg);
    }
    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5750)(this, msg);
}

void CNetProcessIn::OnB024(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4EC0)(this, msg);
}

void CNetProcessIn::OnB023(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4F80)(this, msg);
}

void CNetProcessIn::On3028(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A50E0)(this, msg);
}

void CNetProcessIn::OnB04F(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A51E0)(this, msg);
}

void CNetProcessIn::On30D0(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5280)(this, msg);
}

void CNetProcessIn::On3200(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A52F0)(this, msg);
}

void CNetProcessIn::OnB070(CMsgStreamBuffer &msg) {
    /*byte bSuccessFlag;
    msg >> bSuccessFlag;

    if (g_pMyPlayerObj && bSuccessFlag == 1 )//&& MsgBuffer->m_currentReadBytes >= 20) // i'am just checking static size here, so i guess no any problems may occur while parsing.
    {
        byte _type;
        msg >> _type;
        byte _reason;
        msg >> _reason;
        if (_type == 2)//Attack action
        {
            unsigned int SkillID;
            msg >> SkillID;
            unsigned int AttackerID;
            msg >> AttackerID;
            unsigned int SkillInstanceUniqueID;
            msg >> SkillInstanceUniqueID;
            unsigned int TargetID;
            msg >> TargetID;
            if (AttackerID > 0)//Attacker ID will never be 0
            {
                CICharactor * Attacker = GetCharacterObjectByID_MAYBE(AttackerID);

                if(Attacker)
                {
                    printf("%p \n", Attacker);
                    if(Attacker->IsSame(GFX_RUNTIME_CLASS(CICMonster)))
                    {
                        if(Attacker->GetCommonData() != NULL)
                        {
                            if(Attacker->GetCommonData()->Rarity == 3 || Attacker->GetCommonData()->Rarity == 8)
                            {
                                std::map<int, int>::iterator iter = m_CustomDataManager->UniqueTargetHashmap.find(AttackerID);

                                if(iter != m_CustomDataManager->UniqueTargetHashmap.end())
                                {
                                    if(m_CustomDataManager->UniqueTargetHashmap[AttackerID] != TargetID)
                                    {
                                        CICPlayer *pUser = g_pMyPlayerObj->GetCICPlayerByUniqueID(m_CustomDataManager->UniqueTargetHashmap[AttackerID]); // Doğru arama
                                        if (pUser != NULL) { // pUser geçerli mi kontrol et
                                            DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT");
                                            if (effect)
                                            {
                                                pUser->RemoveEffect(effect);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        CICPlayer *pUser = g_pMyPlayerObj->GetCICPlayerByUniqueID(TargetID);
                                        if (pUser != NULL) { // pUser geçerli mi kontrol et
                                            DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT");
                                            if (effect) {
                                               pUser->MakeEffect(effect);

                                            }
                                        }
                                    }
                                    m_CustomDataManager->UniqueTargetHashmap[AttackerID] = TargetID;
                                }
                                else
                                {
                                    m_CustomDataManager->UniqueTargetHashmap.insert(std::make_pair(Attacker->GetUniqueId(), TargetID));
                                    CICPlayer *pUser = g_pMyPlayerObj->GetCICPlayerByUniqueID(TargetID); // Doğru arama
                                    if (pUser != NULL) { // pUser geçerli mi kontrol et
                                        DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT");
                                        if (effect) {
                                                pUser->MakeEffect(effect);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
          //  SkillDamageParsing(msg);

        }
    }
    msg.m_currentReadBytes = 0;*/
    if(m_Settings->EnableUniqueTarget)
    {
        unsigned __int8 bSuccessFlag;
        msg >> bSuccessFlag;
        if (g_pMyPlayerObj && bSuccessFlag == 1 && msg.m_availableBytesForReading >= 20) // i'am just checking static size here, so i guess no any problems may occur while parsing.
        {
            unsigned __int8 _type;
            msg >> _type;
            unsigned __int8 _reason;
            msg >> _reason;
            if (_type == 2)//Attack action
            {
                unsigned __int32 skillID;
                msg >> skillID;
                unsigned __int32 AttackerID;
                msg >> AttackerID;
                unsigned __int32 SkillInstanceUniqueID;
                msg >> SkillInstanceUniqueID;
                unsigned __int32 AttackedID;
                msg >> AttackedID;// MAIN TARGET Unique ID
                if (AttackerID > 0)//Attacker ID will never be 0
                {
                    //well, i'm stressed the one that i've used here,GetCICPlayerByUniqueID this function is nullable
                    //well, no problems but I firstly need to verify that parsing xD kk
                    CICPlayer* CICAttacker = g_pMyPlayerObj->GetCICPlayerByUniqueID(AttackerID);

                    if (CICAttacker == NULL)
                    {
                        msg.m_currentReadBytes = 0;
                    }
                    else
                    {
                        DWORD check_shit = *(DWORD*)(CICAttacker);
                        if (check_shit == 0xDE26C4) // checking if the unique id is MOB exactly a MOnster you know :D ye but the thing is that those client functions are already crashable
                        {
                            const SCommonData* commonData = CICAttacker->GetCommonData();
                            static const CCharacterData *m_ObjectData = NULL;

                            m_ObjectData = commonData != NULL
                                ? g_CGlobalDataManager->GetCharacter(commonData->RefObjectId)
                                : NULL;

                            if (m_ObjectData && (m_ObjectData->GetData().Rarity == 3 || m_ObjectData->GetData().Rarity == 8))
                            {
                                DWORD effect = g_CGlobalDataManager->GetEffectIdByName(L"SYSTEM_AGGRO_EFFECT");
                                if(effect)
                                {
                                    std::map<int, int>::iterator it = m_CustomDataManager->UniqueTargetHashmap.find(AttackerID);
                                    if (it == m_CustomDataManager->UniqueTargetHashmap.end()) // first attack, let the shit begin.
                                    {
                                        m_CustomDataManager->UniqueTargetHashmap[AttackerID] = AttackedID;//i'am using these maps for unique target, lel this packet is a pain in the ass, i hope you are parsing it correctly xDD
                                        m_CustomDataManager->UniqueTargetHashmapPlayer[AttackedID] = AttackerID;

                                        CICPlayer* CICAttacked = g_pMyPlayerObj->GetCICPlayerByUniqueID(AttackedID);
                                        if (CICAttacked)
                                        {
                                            CICAttacked->MakeEffect(effect);
                                        }
                                    }
                                    else
                                    {
                                        if (it->second != AttackedID) // Attacker is changed, let them switch.
                                        {
                                            UINT32 oldTarget = it->second;

                                            CICPlayer* CICOldTarget = g_pMyPlayerObj->GetCICPlayerByUniqueID(oldTarget);
                                            if (CICOldTarget) CICOldTarget->RemoveEffect(effect);

                                            CICPlayer* CICNewTarget = g_pMyPlayerObj->GetCICPlayerByUniqueID(AttackedID);
                                            if (CICNewTarget) CICNewTarget->MakeEffect(effect);


                                            m_CustomDataManager->UniqueTargetHashmap[AttackerID] = AttackedID;
                                            m_CustomDataManager->UniqueTargetHashmap[AttackedID] = AttackedID;

                                            m_CustomDataManager->UniqueTargetHashmapPlayer.erase(oldTarget);
                                        }
                                        else if (it->second == AttackedID) //attacker is same, check if he still have the effect
                                        {
                                            CICPlayer* CICCurrentTarget = g_pMyPlayerObj->GetCICPlayerByUniqueID(AttackedID);

                                            if (CICCurrentTarget)
                                            {
                                                if(CICCurrentTarget->GetObjEffect() != effect)
                                                    CICCurrentTarget->MakeEffect(effect);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                //SkillDamageParsing(msg);
            }
            else if (_type == 6)
            {
                CIFMacroMenu* macroMenu = GetReadyMacroMenu();
                if (macroMenu && macroMenu->AutoSkillSlot)
                {
                    macroMenu->AutoSkillSlot->InvalidObjects.push_back(macroMenu->AutoSkillSlot->SelectObj);
                    macroMenu->AutoSkillSlot->SelectObj = 0;
                }
            }
            else if(_type == 0x10)
            {
                CIFMacroMenu* macroMenu = GetReadyMacroMenu();
                if (macroMenu && macroMenu->AutoSkillSlot)
                {
                    macroMenu->AutoSkillSlot->InvalidObjects.push_back(macroMenu->AutoSkillSlot->SelectObj);
                    macroMenu->AutoSkillSlot->SelectObj = 0;
                }
            }
        }
        msg.m_currentReadBytes = 0;
    }

    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5FD0)(this, msg);
    RefreshVisibleTargetHpPercent();
}

void CNetProcessIn::OnB071(CMsgStreamBuffer &msg) {
    byte bSuccessFlag;
    msg >> bSuccessFlag;
    if (g_pMyPlayerObj && bSuccessFlag == 1 )
    {
        DWORD32 SkillID;
        msg >> SkillID;
        DWORD32 AttackerID;
        msg >> AttackerID;
        //SkillDamageParsing(msg);
    }
    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4960)(this, msg);
    RefreshVisibleTargetHpPercent();
}
void CNetProcessIn::SkillDamageParsing(CMsgStreamBuffer& msg)
{
    byte hasDamage;
    msg >> hasDamage;
    if (hasDamage==1)
    {
        byte hitCount ;
        msg >> hitCount;
        byte targetCount;
        msg >> targetCount;
        for (byte j = 0; j < targetCount; j++)
        {
            DWORD32 targetUniqueID;
            msg >> targetUniqueID;
            byte dmgEffect ;
            msg >>dmgEffect;
            if (( dmgEffect & 0x80 ) != 0 || ( dmgEffect & 0x2 ) != 0 || ( dmgEffect & 0x8 ) != 0)
                continue;

            // Since there it's not enough flags to check, then this way have to be used
            byte dmgState ;
            msg >> dmgState;
            DWORD32 dmgValue;
            msg >> dmgValue;
            if ((dmgState & 32) != 0 )
            {
                    CIObject *SelectdObject = GetCharacterObjectByID_MAYBE(targetUniqueID);
                    if (!strcmp(SelectdObject->GetRuntimeClass()->m_lpszClassName, "CICMonster")) {
                        CICMonster *SelectdUser = (CICMonster *) SelectdObject;
                }
                /*g_pCGInterface->GetCIFSTEffectGUI()->SetIgnoreDamageTime(9);
                */
                wchar_t message[1000];
                swprintf_s(message, KmtGetText(L"UIIT_KMT_IGNORE_VALUE"),dmgValue);
                CIFSystemMessage *systemmessage = reinterpret_cast<CIFSystemMessage *>(g_pCGInterface->m_IRM.GetResObj(68, 1));
                int color = D3DCOLOR_ARGB(255, 255, 0, 0);
                systemmessage->WriteMessage(0xFF, color, message, 0, 1);
            }

            byte unkByte01 ;
            msg >> unkByte01;
            byte unkByte02 ;
            msg >> unkByte02;
            byte unkByte03 ;
            msg >> unkByte03;

        }
    }
}
void CNetProcessIn::OnB0BD(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A59E0)(this, msg);
}

void CNetProcessIn::On330F(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4A30)(this, msg);
}

void CNetProcessIn::OnB0BE(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5DA0)(this, msg);
}

void CNetProcessIn::OnB0BC(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4B00)(this, msg);
}

void CNetProcessIn::OnB072(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4DE0)(this, msg);
}

void CNetProcessIn::On30A6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4980)(this, msg);
}

void CNetProcessIn::On30D1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A4990)(this, msg);
}

void CNetProcessIn::On3206(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5360)(this, msg);
}

void CNetProcessIn::OnB0A1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880BE0)(this, msg);
    g_SkillAutomationController.OnSkillLearnResponseProcessed();
}

void CNetProcessIn::OnB0A2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880CC0)(this, msg);
    g_SkillAutomationController.OnMasteryLearnResponseProcessed();
}

void CNetProcessIn::OnB203(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880E90)(this, msg);
}

void CNetProcessIn::OnB202(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880DB0)(this, msg);
}

void CNetProcessIn::OnB034(CMsgStreamBuffer &msg) {
    if (m_Settings->EnableOldItemMall && msg.m_availableBytesForReading >= 2) {
        const CMsgStreamReadState originalReadState(msg);
        byte bSuccess = 0;
        byte btType = 0;
        msg >> bSuccess >> btType;

        if (bSuccess == 1 && btType == 0x18) {
            if (msg.m_currentReadBytes > msg.m_availableBytesForReading ||
                msg.m_availableBytesForReading - msg.m_currentReadBytes < 6) {
                msg.FlushRemaining();
                return;
            }

            UINT16 usShopGroupId = 0;
            BYTE btGroupIndex = 0;
            BYTE btTabIndex = 0;
            BYTE btSlotIndex = 0;
            BYTE btSlotsCount = 0;
            msg >> usShopGroupId >> btGroupIndex >> btTabIndex >> btSlotIndex >> btSlotsCount;

            if (msg.m_currentReadBytes > msg.m_availableBytesForReading ||
                msg.m_availableBytesForReading - msg.m_currentReadBytes <
                        static_cast<unsigned int>(btSlotsCount) + sizeof(UINT16)) {
                msg.FlushRemaining();
                return;
            }

            std::vector<BYTE> vSlots;
            vSlots.reserve(btSlotsCount);
            for (int i = 0; i < btSlotsCount; ++i) {
                BYTE btSlot = 0;
                msg >> btSlot;
                vSlots.push_back(btSlot);
            }

            UINT16 usQuantity = 0;
            msg >> usQuantity;
            CSOItemPackage *pSOItemPackage = g_CGlobalDataManager->GetSOPackageByShopData(
                    usShopGroupId, btGroupIndex, btTabIndex, btSlotIndex);

            bool bHandled = false;
            if (pSOItemPackage && pSOItemPackage->GetPackageItemData() &&
                pSOItemPackage->GetSOItem() && g_pCGInterface &&
                g_pCGInterface->GetMainPopup() &&
                g_pCGInterface->GetMainPopup()->GetInventory()) {
                int nQuantity = usQuantity > 0 ? usQuantity : 1;
                CRefPackageItemData *pPackageData = pSOItemPackage->GetPackageItemData();
                if (pPackageData && !pPackageData->m_vScrapData.empty() &&
                    pPackageData->m_vScrapData[0] &&
                    pPackageData->m_vScrapData[0]->m_quantity > 0) {
                    nQuantity = pPackageData->m_vScrapData[0]->m_quantity;
                }

                for (size_t i = 0; i < vSlots.size(); ++i) {
                    g_pCGInterface->GetMainPopup()->GetInventory()->FakePick(
                            pSOItemPackage->GetSOItem()->m_refObjItemId,
                            vSlots[i],
                            nQuantity);
                }

                CIFItemMall *pItemMall = g_pCGInterface->GetItemMall();
                if (pItemMall && pItemMall->GetBuyItemCount() > 0) {
                    pItemMall->SetBuyItemCount(pItemMall->GetBuyItemCount() - 1);
                }
                bHandled = true;
            }

            if (bHandled) {
                msg.FlushRemaining();
                return;
            }

            originalReadState.Restore(msg);
        }
        else {
            originalReadState.Restore(msg);
        }
    }

    if (m_Settings->EnablePickSoxEffect && msg.m_availableBytesForReading >= 2) {
        // Pickup effects only inspect the inventory result. Preserve the complete
        // read state so the native handler still receives NPC sales, buyback,
        // and every other 0xB034 operation exactly as the dispatcher supplied it.
        const CMsgStreamReadState originalReadState(msg);
        byte bSuccess;
        byte status;
        msg >> bSuccess >> status;

        if (bSuccess == 1 && status == 6 && msg.m_availableBytesForReading >= 1) {
            byte slot;
            msg >> slot;
            if (slot != 254 && msg.m_availableBytesForReading >= 8) {
                int unk;
                int ItemID;
                msg >> unk >> ItemID;
                const CItemData *pItem = g_CGlobalDataManager->GetItem(ItemID);
                if (pItem && (pItem->GetData().Rarity == 2 || pItem->GetData().Rarity == 3 ||
                              pItem->GetData().Rarity == 6)) {
                    CIFSoxEffect *m_wheel = g_pCGInterface->m_IRM.GetResObj<CIFSoxEffect>(1600, 1);
                    if (m_wheel && m_wheel->m_itempic) {
                        m_wheel->UpdateMenuSize();
                        m_wheel->ShowGWnd(true);
                        m_wheel->m_itempic->TB_Func_13(pItem->GetData().AssocFileIcon, 1, 1);
                        m_wheel->ddjPath = "clientlibrary\\soxeffect\\pickup_rarebox_";
                        m_wheel->ddjCount = 20;
                        m_wheel->ddjMinSpeed = 0.03f;
                        m_wheel->ddjCurrentIndex = 1;
                        m_wheel->operational = true;
                    }
                }
            }
        } else if (m_Settings->EnablePickSoxEffect && bSuccess == 1 && status == 17 &&
                   msg.m_availableBytesForReading >= 13) {
            unsigned int PetUniqueID;
            byte Slot;
            unsigned int unk;
            int ItemID;
            msg >> PetUniqueID >> Slot >> unk >> ItemID;

            const CItemData *pItem = g_CGlobalDataManager->GetItem(ItemID);
            if (pItem && (pItem->GetData().Rarity == 2 || pItem->GetData().Rarity == 3)) {
                CIFSoxEffect *m_wheel = g_pCGInterface->m_IRM.GetResObj<CIFSoxEffect>(1600, 1);
                if (m_wheel && m_wheel->m_itempic) {
                    m_wheel->UpdateMenuSize();
                    m_wheel->ShowGWnd(true);
                    m_wheel->m_itempic->TB_Func_13(pItem->GetData().AssocFileIcon, 1, 1);
                    m_wheel->ddjPath = "clientlibrary\\soxeffect\\pickup_rarebox_";
                    m_wheel->ddjCount = 20;
                    m_wheel->ddjMinSpeed = 0.03f;
                    m_wheel->ddjCurrentIndex = 1;
                    m_wheel->operational = true;
                }
            }
        }

        originalReadState.Restore(msg);
    }

    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087F370)(this, msg);
}

void CNetProcessIn::On3038(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A9260)(this, msg);
}

void CNetProcessIn::On3039(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A9560)(this, msg);
}

void CNetProcessIn::On3036(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A78B0)(this, msg);
}

void CNetProcessIn::OnB03E(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880A20)(this, msg);
}

void CNetProcessIn::OnB03F(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880A70)(this, msg);
}

void CNetProcessIn::On304D(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A75C0)(this, msg);
}

void CNetProcessIn::OnB04C(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087A7B0)(this, msg);
}

void CNetProcessIn::On3068(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00873990)(this, msg);
}

void CNetProcessIn::On305C(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008735D0)(this, msg);
}

void CNetProcessIn::On30E0(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881600)(this, msg);
}

void CNetProcessIn::On3040(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088BF30)(this, msg);
}

void CNetProcessIn::On3201(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00882280)(this, msg);
}

void CNetProcessIn::On3041(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008822C0)(this, msg);
}

void CNetProcessIn::On3042(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008825D0)(this, msg);
}

void CNetProcessIn::On3153(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00882DE0)(this, msg);
}

void CNetProcessIn::On3154(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00882E50)(this, msg);
}

void CNetProcessIn::OnB046(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00888E30)(this, msg);
}

void CNetProcessIn::OnB04B(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00886AC0)(this, msg);
}

void CNetProcessIn::OnB0C0(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FFD0)(this, msg);
}

void CNetProcessIn::OnB050(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880AC0)(this, msg);
}

void CNetProcessIn::OnB051(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880B10)(this, msg);
}

void CNetProcessIn::OnB0C5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872590)(this, msg);
}

void CNetProcessIn::OnB059(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00883520)(this, msg);
}

void CNetProcessIn::OnB05A(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008809D0)(this, msg);
}

void CNetProcessIn::OnB074(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880B60)(this, msg);
}

void CNetProcessIn::On30EB(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881670)(this, msg);
}

void CNetProcessIn::OnB0EA(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881690)(this, msg);
}

void CNetProcessIn::OnB05B(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008727A0)(this, msg);
}

void CNetProcessIn::On34B1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00871D50)(this, msg);
}

void CNetProcessIn::OnB060(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880010)(this, msg);
}

void CNetProcessIn::On3864(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00886B10)(this, msg);
}

void CNetProcessIn::On3065(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00883CC0)(this, msg);
}

void CNetProcessIn::OnB062(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008800B0)(this, msg);
}

void CNetProcessIn::OnB067(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880120)(this, msg);
}

void CNetProcessIn::OnB069(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00883F90)(this, msg);
}

void CNetProcessIn::OnB06A(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008840D0)(this, msg);
}

void CNetProcessIn::OnB06B(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008801E0)(this, msg);
}

void CNetProcessIn::OnB06C(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008841F0)(this, msg);
}

void CNetProcessIn::On706D(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableMacro)
    {
        unsigned int ReqID, UserJID, PTID,PrimaryMast,SecondaryMast;
        msg >> ReqID >> UserJID >> PTID >> PrimaryMast >> SecondaryMast;
        byte JobState, memberInfoFlag;
        msg >> JobState >> memberInfoFlag;

        int memberInfoJID;
        msg >> memberInfoJID;

        std::n_string CharName;
        // unsigned short namelenght;
        //msg >> namelenght;

        msg >> CharName;


        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1) != NULL)
            {
                if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot != NULL)
                {
                    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->Macro_AutoHunt)
                    {
                        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_ACCEPT_JOIN_PT_REQ->GetCheckedState_MAYBE())
                        {
                            if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyOnlyFromList->GetCheckedState_MAYBE())
                            {
                                if(!CharName.empty())
                                {
                                    std::n_wstring chars = TO_WSTRING(CharName).c_str();
                                    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyMemberList.find(chars) !=
                                       g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyMemberList.end())
                                    {
                                        // accept
                                        CMsgStreamBuffer buf(0x306E);
                                        buf << ReqID;
                                        buf << UserJID;
                                        buf << (byte)1;
                                        SendMsg(buf);
                                    }
                                    else
                                    {
                                        // refuse
                                        CMsgStreamBuffer buf(0x306E);
                                        buf << ReqID;
                                        buf << UserJID;
                                        buf << (byte)0;
                                        SendMsg(buf);
                                    }
                                }
                                else
                                {
                                    // refuse
                                    CMsgStreamBuffer buf(0x306E);
                                    buf << ReqID;
                                    buf << UserJID;
                                    buf << (byte)0;
                                    SendMsg(buf);
                                }
                            }
                            else
                            {
                                // accept
                                CMsgStreamBuffer buf(0x306E);
                                buf << ReqID;
                                buf << UserJID;
                                buf << (byte)1;
                                SendMsg(buf);
                            }
                        }
                        else
                        {
                            // refuse
                            CMsgStreamBuffer buf(0x306E);
                            buf << ReqID;
                            buf << UserJID;
                            buf << (byte)0;
                            SendMsg(buf);
                        }
                    }
                }
            }

        msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00884660)(this, msg);
}

void CNetProcessIn::OnB06D(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008847D0)(this, msg);
}

void CNetProcessIn::OnB081(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880330)(this, msg);
}

void CNetProcessIn::On3080(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableMacro)
    {
        byte Type;
        msg >> Type;

        if(g_pMyPlayerObj->GetCurrentHp() == 0)
        {
            if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1) != NULL)
            {
                if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot != NULL)
                {
                    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->Macro_AutoHunt)
                    {
                        if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[RETURN_TOWN_SETTING] == eTownSetting::WAIT_FOR_RES) // wait for ress
                        {
                            if(Type == 4)
                            {
                                CMsgStreamBuffer buf(0x3080);
                                buf << (byte)1;
                                buf << (byte)1;
                                SendMsg(buf);
                            }
                        }
                    }
                }
            }
        }
        if(Type == 2)
            {
                unsigned int SenderUniqueID;
                msg >> SenderUniqueID;

                byte PartyType;
                msg >> PartyType;
                if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1) != NULL)
                {
                    if(g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot != NULL)
                    {
                        CIFMacroMenuAutoHunt * AutoHuntSlot = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
                        if(AutoHuntSlot->AutoPartyAcceptCheckBox->GetCheckedState_MAYBE())
                        {
                            if(AutoHuntSlot->AutoPartyOnlyFromList->GetCheckedState_MAYBE())
                            {
                                CICharactor *pUser = static_cast<CICharactor *>(GetCharacterObjectByID_MAYBE(SenderUniqueID));
                                if(pUser)
                                {
                                    if(pUser->GetJobType() == g_pMyPlayerObj->GetJobType())
                                    {
                                        if(AutoHuntSlot->AutoPartyMemberList.find(pUser->GetName())
                                           != AutoHuntSlot->AutoPartyMemberList.end())
                                        {
                                            // 0x4 GDR_PT_EXP_FREE_ITEM_FREECheckBox
                                            // 0x5 GDR_PT_EXP_AUTO_ITEM_NOCheckBox
                                            // 0x6 GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX
                                            // 0x7 GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox
                                            if(PartyType == 0x4)
                                            {
                                                if(AutoHuntSlot->GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(1);
                                                    buf << byte(1);
                                                    SendMsg(buf);
                                                }
                                                else
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(2);
                                                    buf << (unsigned short)11276;
                                                    SendMsg(buf);
                                                }
                                            }
                                            if(PartyType == 0x5)
                                            {
                                                if(AutoHuntSlot->GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(1);
                                                    buf << byte(1);
                                                    SendMsg(buf);
                                                }
                                                else
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(2);
                                                    buf << (unsigned short)11276;
                                                    SendMsg(buf);
                                                }
                                            }
                                            if(PartyType == 0x6)
                                            {
                                                if(AutoHuntSlot->GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(1);
                                                    buf << byte(1);
                                                    SendMsg(buf);
                                                }
                                                else
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(2);
                                                    buf << (unsigned short)11276;
                                                    SendMsg(buf);
                                                }
                                            }
                                            if(PartyType == 0x7)
                                            {
                                                if(AutoHuntSlot->GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(1);
                                                    buf << byte(1);
                                                    SendMsg(buf);
                                                }
                                                else
                                                {
                                                    CMsgStreamBuffer buf(0x3080); //Accept
                                                    buf << byte(2);
                                                    buf << (unsigned short)11276;
                                                    SendMsg(buf);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            CMsgStreamBuffer buf(0x3080); //refuse
                                            buf << byte(2);
                                            buf << (unsigned short)11276;
                                            SendMsg(buf);
                                        }
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(2);
                                        buf << (unsigned short)11276;
                                        SendMsg(buf);
                                    }
                                }

                            }
                            else
                            {
                                if(PartyType == 0x4)
                                {
                                    if(AutoHuntSlot->GDR_PT_EXP_FREE_ITEM_FREECheckBox->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(1);
                                        buf << byte(1);
                                        SendMsg(buf);
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(2);
                                        buf << (unsigned short)11276;
                                        SendMsg(buf);
                                    }
                                }
                                if(PartyType == 0x5)
                                {
                                    if(AutoHuntSlot->GDR_PT_EXP_AUTO_ITEM_NOCheckBox->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(1);
                                        buf << byte(1);
                                        SendMsg(buf);
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(2);
                                        buf << (unsigned short)11276;
                                        SendMsg(buf);
                                    }
                                }
                                if(PartyType == 0x6)
                                {
                                    if(AutoHuntSlot->GDR_PT_EXP_FREE_ITEM_AUTO_CHECKBOX->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(1);
                                        buf << byte(1);
                                        SendMsg(buf);
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(2);
                                        buf << (unsigned short)11276;
                                        SendMsg(buf);
                                    }
                                }
                                if(PartyType == 0x7)
                                {
                                    if(AutoHuntSlot->GDR_PT_EXP_AUTO_ITEM_AUTOCheckBox->GetCheckedState_MAYBE())
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(1);
                                        buf << byte(1);
                                        SendMsg(buf);
                                    }
                                    else
                                    {
                                        CMsgStreamBuffer buf(0x3080); //Accept
                                        buf << byte(2);
                                        buf << (unsigned short)11276;
                                        SendMsg(buf);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088AA60)(this, msg);
}

void CNetProcessIn::On3085(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008803F0)(this, msg);
}

void CNetProcessIn::OnB082(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880430)(this, msg);
}

void CNetProcessIn::OnB083(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008804A0)(this, msg);
}

void CNetProcessIn::OnB084(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880520)(this, msg);
}

void CNetProcessIn::On3089(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880590)(this, msg);
}

void CNetProcessIn::On3086(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FCB0)(this, msg);
}

void CNetProcessIn::On3087(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088BB50)(this, msg);
}

void CNetProcessIn::On3088(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880840)(this, msg);
}

void CNetProcessIn::On30E8(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00880970)(this, msg);
}

void CNetProcessIn::On308C(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008806B0)(this, msg);
}

void CNetProcessIn::OnB0B1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872040)(this, msg);
}

void CNetProcessIn::On30B8(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00873C10)(this, msg);
}

void CNetProcessIn::OnB0B2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008720F0)(this, msg);
}

void CNetProcessIn::On30B9(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008721B0)(this, msg);
}

void CNetProcessIn::On30B7(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087A260)(this, msg);
}

void CNetProcessIn::OnB0B3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00873FF0)(this, msg);
}

void CNetProcessIn::OnB0B5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872450)(this, msg);
}

void CNetProcessIn::OnB0B4(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872500)(this, msg);
}

void CNetProcessIn::OnB0BA(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00878230)(this, msg);
}

void CNetProcessIn::On30BB(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00873D60)(this, msg);
}

void CNetProcessIn::OnB0A7(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7A20)(this, msg);
}

void CNetProcessIn::OnB010(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008743A0)(this, msg);
}

void CNetProcessIn::On30D6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008811E0)(this, msg);
}

void CNetProcessIn::On30D7(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881280)(this, msg);
}

void CNetProcessIn::On30D5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008812B0)(this, msg);
}

void CNetProcessIn::OnB0D8(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881450)(this, msg);
}

void CNetProcessIn::OnB0D9(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008814B0)(this, msg);
}

void CNetProcessIn::On30EC(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00884A30)(this, msg);
}

void CNetProcessIn::On30DC(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A7EE0)(this, msg);
}

void CNetProcessIn::OnB515(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881570)(this, msg);
}

void CNetProcessIn::On3514(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00884CA0)(this, msg);
}

void CNetProcessIn::On3CA2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008ABCC0)(this, msg);
}

void CNetProcessIn::On30E6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00884DD0)(this, msg);
}

void CNetProcessIn::OnB0E1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008890B0)(this, msg);
}

void CNetProcessIn::OnB0E2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00889210)(this, msg);
}

void CNetProcessIn::OnB0E3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00889370)(this, msg);
}

void CNetProcessIn::OnB0E4(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088A130)(this, msg);
}

void CNetProcessIn::OnB0E5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008830F0)(this, msg);
}

void CNetProcessIn::OnB0E6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881740)(this, msg);
}

void CNetProcessIn::OnB0F0(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008852C0)(this, msg);
}

void CNetProcessIn::On38F5(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00887330)(this, msg);
}

void CNetProcessIn::OnB0F1(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881820)(this, msg);
}

void CNetProcessIn::OnB0F8(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881890)(this, msg);
}

void CNetProcessIn::OnB0F3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881950)(this, msg);
}

void CNetProcessIn::On30FF(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088A420)(this, msg);
}

void CNetProcessIn::On3100(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008819B0)(this, msg);
}

void CNetProcessIn::OnB0F2(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008819E0)(this, msg);
}

void CNetProcessIn::OnB0F4(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881C60)(this, msg);
}

void CNetProcessIn::On34B3(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FCD0)(this, msg);
}

void CNetProcessIn::On3101(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00883060)(this, msg);
}

void CNetProcessIn::On34B4(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00887300)(this, msg);
}

void CNetProcessIn::OnB0F9(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881A70)(this, msg);
}

void CNetProcessIn::OnB0F6(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881AE0)(this, msg);
}

void CNetProcessIn::OnB0FA(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881BD0)(this, msg);
}

void CNetProcessIn::OnB103(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00889680)(this, msg);
}

void CNetProcessIn::OnB104(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881CB0)(this, msg);
}

void CNetProcessIn::OnB105(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008857B0)(this, msg);
}

void CNetProcessIn::OnB106(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881D00)(this, msg);
}

void CNetProcessIn::On3908(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008897C0)(this, msg);
}

void CNetProcessIn::OnB107(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881D80)(this, msg);
}

void CNetProcessIn::OnB0FB(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881DD0)(this, msg);
}

void CNetProcessIn::On3102(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FD00)(this, msg);
}

void CNetProcessIn::OnB0FC(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881E20)(this, msg);
}

void CNetProcessIn::OnB0FD(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881E70)(this, msg);
}

void CNetProcessIn::On3109(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FD90)(this, msg);
}

void CNetProcessIn::On7110(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00885920)(this, msg);
}

void CNetProcessIn::OnB110(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881EC0)(this, msg);
}

void CNetProcessIn::OnB112(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881F30)(this, msg);
}

void CNetProcessIn::OnB114(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00885AE0)(this, msg);
}

void CNetProcessIn::OnB113(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881F80)(this, msg);
}

void CNetProcessIn::OnB501(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00885C20)(this, msg);
}

void CNetProcessIn::On30EF(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088A770)(this, msg);
}
std::wstring intToWString(int number) {
    wchar_t buffer[10];
    swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%d", number);
    return std::wstring(buffer);
}
void tokenizeLine(const wchar_t* line, std::vector<int>& values) {
    const wchar_t* token = line;
    int value;
    while (swscanf(token, L"%d", &value) == 1) {
        values.push_back(value);
        token = std::wcschr(token, L' ');
        if (token == NULL) break;
        token++; // Boşluğu atla
    }
}

void CNetProcessIn::On3305(CMsgStreamBuffer &msg) {
    if(m_Settings->EnableMacro)
    {
        if (!m_Player->FirstSpawn) {
            if (g_pMyPlayerObj != NULL) {

                CIFSettings* setting = g_pCGInterface->m_IRM.GetResObj<CIFSettings>(SettingsWndID, 1);
                char buffersetting[0x200];
                sprintf(buffersetting, "%s\\Setting\\Client_Extra.txt", theApp.GetWorkingDir());

                FILE *filesettings = fopen(buffersetting, "r");
                if (filesettings != NULL) {
                    wchar_t line[512];
                    while (fgetws(line, sizeof(line) / sizeof(wchar_t), filesettings) != NULL) {
                        int value = 0;
                        wchar_t settingName[256] = {0};

                        if (wcsstr(line, L"Fps Window: ") != NULL) {
                            if (swscanf(line, L"Fps Window: %d", &value) == 1) {
                                setting->EnableShowFps->SetCheckBoxState(value);

                            }
                        }
                        else if (wcsstr(line, L"Infinity Zoom: ") != NULL) {
                            if (swscanf(line, L"Infinity Zoom: %d", &value) == 1) {
                                setting->EnableZoom->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Always Active Window: ") != NULL) {
                            if (swscanf(line, L"Always Active Window: %d", &value) == 1) {
                                setting->EnableActiveWnd->SetCheckBoxState(value);
                            }
                        }
                     /*   else if (wcsstr(line, L"Extend Background Limit: ") != NULL) {
                            if (swscanf(line, L"Extend Background Limit: %d", &value) == 1) {
                                setting->BgLimit->SetCheckBoxState(value);
                            }
                        }*/
                    }

                    setting->ClickFPSCheckBox();
                    fclose(filesettings); // Dosyayı kapat
                }




                CIFMacroMenu* CustomMacro = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1);
                CustomMacro->AutoPotionSlot->ActivateTabPage(0);
                CustomMacro->AutoSkillSlot->ActivateTabPage(0);
                CustomMacro->AutoHuntSlot->ActivateTabPage(0);
                CustomMacro->PickupFilterSlot->ActivateTabPage(0);

                char buffer3[0x200];
                sprintf(buffer3, "%s\\Setting\\%ls_Macro.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str());

                int MinLevel = 0;
                int MaxLevel = 0;
                std::n_wstring Title = std::n_wstring();

                FILE *file3 = fopen(buffer3, "r");
                if (file3 != NULL) {
                    wchar_t line[512];
                    while (fgetws(line, sizeof(line) / sizeof(wchar_t), file3) != NULL) {
                        int value = 0;
                        wchar_t settingName[256] = {0};

                        if (wcsstr(line, L"Dont attack the monsters: ") != NULL) {
                            if (swscanf(line, L"Dont attack the monsters: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->DONT_ATTACK_MONSTERCB->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Unique target: ") != NULL) {
                            if (swscanf(line, L"Unique target: %d", &value) == 1) {
                                CIFMacroMenuAutoHunt* hunt =
                                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot;
                                if (hunt->UniqueTargetCheckBox)
                                    hunt->UniqueTargetCheckBox->SetCheckBoxState(value != 0);
                            }
                        }
                        else if (wcsstr(line, L"Radius: ") != NULL) {
                            if (swscanf(line, L"Radius: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[eAutoHuntSetting::RADUIS_SETTING] = value;
                            }
                        }
                        else if (wcsstr(line, L"Hwan setting: ") != NULL) {
                            if (swscanf(line, L"Hwan setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[eAutoHuntSetting::ZERK_SETTING] = value;
                            }
                        }
                        else if (wcsstr(line, L"Return town setting: ") != NULL) {
                            if (swscanf(line, L"Return town setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[eAutoHuntSetting::RETURN_TOWN_SETTING] = value;
                            }
                        }
                        else if (wcsstr(line, L"Back town setting: ") != NULL) {
                            if (swscanf(line, L"Back town setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[eAutoHuntSetting::BACK_HOUR_SETTING] = value;
                            }
                        }
                        else if (wcsstr(line, L"Repair setting: ") != NULL) {
                            if (swscanf(line, L"Repair setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoHuntSetting[eAutoHuntSetting::REPAIR_SETTING] = value;
                            }
                        }
                        else if (wcsstr(line, L"Back town when potion less: ") != NULL) {
                            if (swscanf(line, L"Back town when potion less: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoReturnHPMPLessCheckbox->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Nasrun setting: ") != NULL) {
                            if (swscanf(line, L"Nasrun setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoUseNasrun->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Ignore skill setting: ") != NULL) {
                            if (swscanf(line, L"Ignore skill setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoUseNasrunIgnore->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Invite to Party: ") != NULL) {
                            if (swscanf(line, L"Invite to Party: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyInviteCheckBox->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Accept party invite: ") != NULL) {
                            if (swscanf(line, L"Accept party invite: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyAcceptCheckBox->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Only from list: ") != NULL) {
                            if (swscanf(line, L"Only from list: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyOnlyFromList->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Accept join party requests: ") != NULL) {
                            if (swscanf(line, L"Accept join party requests: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_ACCEPT_JOIN_PT_REQ->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Auto reform party: ") != NULL) {
                            if (swscanf(line, L"Auto reform party: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_AUTO_REFORM_PARTY->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Resurrect party members:") != NULL) {
                            if (swscanf(line, L"Resurrect party members: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoResPtMembersCB->SetCheckBoxState(value);
                            }
                        }
                        else if (wcsstr(line, L"Auto reform min level: ") != NULL) {
                            if (swscanf(line, L"Auto reform min level: %d", &value) == 1) {
                                MinLevel = value;
                                if (MinLevel > 0) {
                                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_AUTO_REFORM_PT_LVL_MIN->SetText(intToWString(MinLevel).c_str());
                                }
                            }
                        }
                        else if (wcsstr(line, L"Auto reform max level: ") != NULL) {
                            if (swscanf(line, L"Auto reform max level: %d", &value) == 1) {

                                MaxLevel = value;
                                if (MaxLevel <= m_Settings->ServerMaxLevel) {
                                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_AUTO_REFORM_PT_LVL_MAX->SetText(intToWString(MaxLevel).c_str());
                                }
                            }
                        }
                        else if (wcsstr(line, L"Auto party setting: ") != NULL) {
                            if (swscanf(line, L"Auto party setting: %d", &value) == 1) {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->LoadPartyExpItemSetting(value);
                            }
                        }
                        else if (wcsstr(line, L"Auto reform party title: ") != NULL) {
                            wchar_t *pos = wcsstr(line, L"Auto reform party title: ");
                            if (pos != NULL) {
                                Title = pos + wcslen(L"Auto reform party title: ");
                                if (Title.length() > 50) {
                                    Title.erase(49);
                                }
                                size_t endPos = Title.find_last_not_of(L"\r\n");
                                if (endPos != std::wstring::npos) {
                                    Title.erase(endPos + 1);
                                }
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->GDR_AUTO_REFORM_TITLE->SetText(Title.c_str());
                            }
                        }
                        else if (wcsstr(line, L"Auto party member ") != NULL) {
                            int memberIndex;
                            wchar_t memberName[256];
                            // "Auto party member " ifadesini ve indeksi bul
                            if (swscanf(line, L"Auto party member %d: %ls", &memberIndex, memberName) == 2) {
                                // Yeni üyeyi haritaya ekle
                                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyMemberList.insert(std::make_pair(memberName, L""));
                            }
                        }

                    }


                    g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->LoadSetting();

                    int i = 0;
                    for(std::map<std::n_wstring , std::n_wstring>::iterator  it =  g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyMemberList.begin();
                        it !=  g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->AutoPartyMemberList.end(); it++)
                    {
                        i++;
                        if(i <= 7)
                        {
                            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoHuntSlot->m_PartySlot[i-1]->LoadItems(it->first);
                        }
                        else
                        {
                            break;
                        }

                    }

                    fclose(file3); // Dosyayı kapat
                }



                char buffer34[0x200];
                sprintf(buffer34, "%s\\Setting\\%ls_MacroAutoBuffSettings.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str());


                FILE *file34 = fopen(buffer34, "r");
                if (file34 != NULL) {
                    wchar_t line[256];
                    while (fgetws(line, sizeof(line) / sizeof(wchar_t), file34) != NULL) {
                        // ":" karakterine kadar olan kısmı anahtar olarak al
                        wchar_t* pos = std::wcschr(line, L':');
                        if (pos != NULL) {
                            *pos = L'\0'; // ":" karakterini null karakteri ile değiştir
                            std::n_wstring key(line);

                            // Değerleri ayırıp vektöre ekle
                            std::vector<int> values;
                            tokenizeLine(pos + 1, values);
                            // Anahtar ve değeri eşleştir
                            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoSkillSlot->PartyBuffList.insert(std::make_pair(key, values));
                        }
                    }

                    fclose(file34); // Dosyayı kapat
                }

                CIFMacroMenuPickFilter *  PickupFilterSlot = g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->PickupFilterSlot;
                char buffer35[256];
                sprintf(buffer35, "%s\\Setting\\%ls_PickupFilter.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str());
                FILE *file35 = fopen(buffer35, "r");
                if (file35 != NULL) {
                    char line[256];
                    while (fgets(line, sizeof(line), file35)) {
                        int value;
                        if (sscanf(line, "Enable Pick Filter: %d", &value) == 1) {
                            PickupFilterSlot->Macro_PetFilter = value;
                            if(value)
                            {
                                g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->PetFilterOnOffBtn();
                            }
                        }
                        else if (sscanf(line, "Pick Via Char: %d", &value) == 1) {
                            PickupFilterSlot->PickViaCharCheckBox->SetCheckBoxState(value);
                            if(value)
                            {
                                PickupFilterSlot->PickViaPetCheckBox->SetCheckBoxState(false);
                            }
                        }
                        else if (sscanf(line, "Pick Via Pet: %d", &value) == 1) {
                            PickupFilterSlot->PickViaPetCheckBox->SetCheckBoxState(value);
                            if(value)
                            {
                                PickupFilterSlot->PickViaCharCheckBox->SetCheckBoxState(false);
                            }
                        } else if (sscanf(line, "Only Rare Equipments: %d", &value) == 1) {
                           PickupFilterSlot->OnlyRareEquiptsCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Gold: %d", &value) == 1) {
                            PickupFilterSlot->DontPickGoldCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Alchemy Tables: %d", &value) == 1) {
                            PickupFilterSlot->DontPickAlchemytablesCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Alchemy Stones: %d", &value) == 1) {
                            PickupFilterSlot->DontPickAlchemyStonesCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Elixirs: %d", &value) == 1) {
                           PickupFilterSlot->DontPickElixirsCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Arrow: %d", &value) == 1) {
                            PickupFilterSlot->DontPickArrowCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Return: %d", &value) == 1) {
                           PickupFilterSlot->DontPickReturnCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Trash: %d", &value) == 1) {
                          PickupFilterSlot->DontPickTrashCB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG1: %d", &value) == 1) {
                            PickupFilterSlot->PickDg1CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG2: %d", &value) == 1) {
                            PickupFilterSlot->PickDg2CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG3: %d", &value) == 1) {
                            PickupFilterSlot->PickDg3CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG4: %d", &value) == 1) {
                            PickupFilterSlot->PickDg4CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG5: %d", &value) == 1) {
                            PickupFilterSlot->PickDg5CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG6: %d", &value) == 1) {
                            PickupFilterSlot->PickDg6CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG7: %d", &value) == 1) {
                            PickupFilterSlot->PickDg7CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG8: %d", &value) == 1) {
                           PickupFilterSlot->PickDg8CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG9: %d", &value) == 1) {
                            PickupFilterSlot->PickDg9CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG10: %d", &value) == 1) {
                            PickupFilterSlot->PickDg10CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG11: %d", &value) == 1) {
                            PickupFilterSlot->PickDg11CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG12: %d", &value) == 1) {
                            PickupFilterSlot->PickDg12CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG13: %d", &value) == 1) {
                            PickupFilterSlot->PickDg13CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG14: %d", &value) == 1) {
                            PickupFilterSlot->PickDg14CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG15: %d", &value) == 1) {
                            PickupFilterSlot->PickDg15CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG16: %d", &value) == 1) {
                            PickupFilterSlot->PickDg16CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG17: %d", &value) == 1) {
                            PickupFilterSlot->PickDg17CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Pick DG18: %d", &value) == 1) {
                            PickupFilterSlot->PickDg18CB->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick HP/MP: %d", &value) == 1) {
                            PickupFilterSlot->DontPickHpMp->SetCheckBoxState(value);
                        } else if (sscanf(line, "Don't Pick Vigor: %d", &value) == 1) {
                            PickupFilterSlot->DontPickVigor->SetCheckBoxState(value);
                        }
                    }
                    // Dosyayı kapat
                    fclose(file35);
                }

/*
                char buffer36[0x200];
                sprintf(buffer36, "%s\\Setting\\%ls_MacroAutoPotion.txt", theApp.GetWorkingDir(), g_pMyPlayerObj->GetCharName().c_str());

                // Dosyayı okuma modunda aç
                FILE *file36 = fopen(buffer36, "r");
                if (file36 != NULL) {
                    wchar_t line[256];
                    while (fgetws(line, sizeof(line) / sizeof(wchar_t), file36) != NULL) {
                        wchar_t lineCopy[256];
                        wcscpy(lineCopy, line);

                        // Yeni satır karakterlerini kaldır
                        wchar_t* newline = std::wcschr(lineCopy, L'\n');
                        if (newline != NULL) *newline = L'\0';
                        wchar_t* carriageReturn = std::wcschr(lineCopy, L'\r');
                        if (carriageReturn != NULL) *carriageReturn = L'\0';

                        // "Setting" ile başlayan satırları işle
                        if (std::wcsncmp(lineCopy, L"Setting", 7) == 0) {
                            wchar_t* pos = std::wcschr(lineCopy, L':');
                            if (pos != NULL) {
                                // ":" karakterinden sonraki kısmı oku
                                pos++; // ":" karakterinden bir sonraki karakter
                                while (*pos == L' ' || *pos == L'\t') pos++; // Boşlukları atla

                                // Değerler
                                std::vector<int> values;
                                tokenizeLine2(pos, values);

                                // Debugging için
                                std::wcout << L"Processed Line: Values: ";
                                for (size_t i = 0; i < values.size(); ++i) {
                                    std::wcout << values[i] << L" ";
                                }
                                std::wcout << std::endl;

                        if (values.size() >= 3) {

                            CIFMacroAutoPotionSlot::Macro_AutoPotionData Datas = CIFMacroAutoPotionSlot::Macro_AutoPotionData();
                            Datas.SlotSeq = values[0];
                            Datas.CheckStatus = values[1];
                            Datas.Value = values[2];

                            g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->MyMacroAutoPotionData.insert(std::make_pair(values[0], Datas));
                        }
                    }
                        }

                }
                    fclose(file36); // Dosyayı kapat
                }
                g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1)->AutoPotionSlot->UpdateSlot();*/
                m_Player->FirstSpawn = true;
            }
        }
        msg.m_currentReadBytes = 0;
    }
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FDD0)(this, msg);
}

void CNetProcessIn::OnB302(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00885E50)(this, msg);
}

void CNetProcessIn::OnB304(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00881FD0)(this, msg);
}

void CNetProcessIn::On7302(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00886000)(this, msg);
}

void CNetProcessIn::OnB30B(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00882030)(this, msg);
}

void CNetProcessIn::OnB308(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00886170)(this, msg);
}

void CNetProcessIn::OnB309(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008820B0)(this, msg);
}

void CNetProcessIn::OnB30A(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00882130)(this, msg);
}

void CNetProcessIn::On3B07(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00886310)(this, msg);
}

void CNetProcessIn::On3204(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FE00)(this, msg);
}

void CNetProcessIn::OnB155(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872810)(this, msg);
}

void CNetProcessIn::On3156(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872810)(this, msg);
}

void CNetProcessIn::OnB157(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872940)(this, msg);
}

void CNetProcessIn::OnB150(CMsgStreamBuffer &msg) {
/*    byte succesflagmaybe;
    byte alchemytypemaybe; // 0x2 plus

    msg >> succesflagmaybe;
    msg >> alchemytypemaybe;

    if(alchemytypemaybe==0x2)
    {
        byte alchemyresult;
        msg >> alchemyresult; // 0x1 succes 0x0 failed
        byte itemslot;
        msg >> itemslot;
        if(alchemyresult == 0x1)
        {
            unsigned int unk;
            msg >> unk;
            int ItemID;
            msg >> ItemID;
            byte CurrentPlus;
            msg >> CurrentPlus;
            if(CurrentPlus > 0)
            {
                CMsgStreamBuffer buf(0x169A);
                buf << byte(16);
                buf << itemslot << CurrentPlus;
                SendMsg(buf);
            }
        }
    }
*/
    msg.m_currentReadBytes = 0;
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008729E0)(this, msg);

    CIFAlchemyMacro* macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
    if (macro != NULL) {
        macro->OnAlchemyResponse();
    }
}

void CNetProcessIn::OnB151(CMsgStreamBuffer &msg) {

    /*
      byte success;
      msg >> success;
      printf("Success: %d\n", success);

      if (success == 1) {
          byte unk2;
          byte unk3;
          msg >> unk2 >> unk3;
          printf("Unk2: %d, Unk3: %d\n", unk2, unk3);

          byte ItemSlot;
          msg >> ItemSlot;
          printf("ItemSlot: %d\n", ItemSlot);

          int unk4;
          msg >> unk4;
          printf("Unk4: %d\n", unk4);

          int ItemID;
          msg >> ItemID;
          printf("ItemID: %d\n", ItemID);

          byte Plus;
          msg >> Plus;
          printf("Plus: %d\n", Plus);

          unsigned long unk5;
          msg >> unk5;
          printf("Unk5: %ld\n", unk5);

          int empty;
          msg >> empty;
          printf("empty: %d\n", empty);

          int DurabilityMaybe;
          msg >> DurabilityMaybe;
          printf("DurabilityMaybe: %d\n", DurabilityMaybe);

          byte TotalBlueMaybe;
          msg >> TotalBlueMaybe;
          printf("TotalBlueMaybe: %d\n", TotalBlueMaybe);

           for (int i = 0; i < TotalBlueMaybe; i++) {
               int Variance, Variance1;
               msg >> Variance;
               msg >> Variance1;
               printf("Variance: %d, Variance1: %d\n", Variance, Variance1);
           }

           byte its1;
           msg >> its1;
           printf("its1: %d\n", its1);

           byte its2;
           msg >> its2;
           printf("its2: %d\n", its2);

           byte its3;
           msg >> its3;
           printf("its3: %d\n", its3);

           byte its4;
           msg >> its4;
           printf("its4: %d\n", its4);

          msg.m_currentReadBytes = 0;
          return;
          }
      else if(success == 2)
      {
          msg.m_currentReadBytes = 0;
      }

      msg.m_currentReadBytes = 0;

      if(g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
      {

          else
          {

          }
          //printf("%d %d %d %d %d %d %d %ld %d %d %ld %d %d %d %d\n", success, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15);
          msg.m_currentReadBytes = 0;
      }*/

    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872CD0)(this, msg);

    CIFAlchemyMacro* macro = g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1);
    if (macro != NULL) {
        macro->OnAlchemyResponse();
    }
}

void CNetProcessIn::OnB16A(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00872FA0)(this, msg);
}

void CNetProcessIn::On3207(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0088A820)(this, msg);
}

void CNetProcessIn::OnB0ED(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008821C0)(this, msg);
}

void CNetProcessIn::On34BA(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008867A0)(this, msg);
}

void CNetProcessIn::OnB402(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008A5230)(this, msg);
}

void CNetProcessIn::OnFFC(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x008832A0)(this, msg);
}

void CNetProcessIn::On2000(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00888DA0)(this, msg);
}

void CNetProcessIn::OnFFD(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x0087FE40)(this, msg);
}

void CNetProcessIn::OnB05E(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00878B40)(this, msg);
}

void CNetProcessIn::On3078(CMsgStreamBuffer &msg) {
    DEBUG_PRINT_CALL()
    reinterpret_cast<void (__thiscall *)(CNetProcessIn *, CMsgStreamBuffer &)>(0x00873140)(this, msg);
}


