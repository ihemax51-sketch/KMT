#include <CustomData/CustomSettingManager.h>
#include "AlramGuideMgrWnd.h"

#include "IFConfirmReputationGuide.h"
#include "IFEventGuide.h"
#include "IFQuestInfoGuide.h"
#include "IFLetterAlarmGuide.h"
#include "IFServerEventGuide.h"
#include "IFOpenMarketAlramGuide.h"
#include "IFEventGuideSecond.h"
#include "IFMenuGuide.h"
#include "Social/IFAutoEquipGuide.h"

#include "GInterface.h"
#include "Game.h"
#include "ICPlayer.h"
#include "IFMagicStateBoard.h"
#include "IFStateSlot.h"
#include "Guides/IFChestGuide.h"
#include "Social/IFFacebookGuide.h"
#include "Social/IFDiscordGuide.h"
#include "Social/IFWebSGuide.h"
#include "Web/IFWebGuide.h"
#include <DailyLogin/IFDailyLoginGuide.h>
#include <MacroAlchemy/IFAlchemyMacroGuide.h>
#include "Macro/IFMacroGuide.h"
#include "CustomInterface/IFLuckySpinWnd.h"
#include "CustomInterface/IFSpecialOffersWnd.h"
#include "CustomInterface/IFKillerAnimationWnd.h"
#include "CustomInterface/IFDropLogsWnd.h"
#include "CustomInterface/IFPvpChallengeWnd.h"
#include "Menu/IFEventRegister.h"
#include "WebViewer/IFWebViewerGuide.h"
#include "WebViewer/WebViewerConfig.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CAlramGuideMgrWnd, 0x00ee99a8)

#define ALRAM_GUIDE_ICON_SIZE 42
#define ALRAM_GUIDE_MAX_ICONS_LINE 4

static int GetGuideIconColumns(DWORD dwID) {
    return dwID == GDR_EVENT_REGISTER_GUIDE ? 2 : 1;
}

static int GetGuideIconWidth(DWORD dwID) {
    const int columns = GetGuideIconColumns(dwID);
    return (columns * ALRAM_GUIDE_ICON_SIZE) + ((columns - 1) * 2);
}

CAlramGuideMgrWnd::CAlramGuideMgrWnd() {
    m_btGuidesCount = 0;
}

static bool IsGuideIconEnabled(int nWndID) {
    if(CWebViewerConfig::IsWebViewerGuideId(nWndID))
        return m_Settings->ShowGuideWebViewer;

    switch(nWndID) {
        case GDR_MENU_GUIDE:
            return m_Settings->ShowGuideMenu;
        case GDR_LUCKY_SPIN_GUIDE:
            return m_Settings->ShowGuideLuckySpin && m_Settings->EnableLuckySpin;
        case GDR_SPECIAL_OFFERS_GUIDE:
            return m_Settings->ShowGuideSpecialOffers && m_Settings->EnableSpecialOffers;
        case GDR_KILLER_ANIMATION_GUIDE:
            return m_Settings->ShowGuideKillerAnimation;
        case GDR_EVENT_REGISTER_GUIDE:
            return m_Settings->EventRegisterWnd != 0;
        case GDR_CHEST_GUIDE:
            return m_Settings->ShowGuideItemChest;
        case GDR_DROPLOG_OPEN_ICON:
            return m_Settings->ShowGuideDropLogs;
        case GDR_PVP_CHALLENGE_GUIDE:
            return m_Settings->ShowGuidePvpChallenge && m_Settings->EnablePvpChallenge;
        case GDR_MACRO_GUIDE:
            return m_Settings->ShowGuideMacro && m_Settings->EnableMacro;
        case GDR_DAILY_LOGIN:
            return m_Settings->ShowGuideDailyLogin;
        case GDR_DISCORD_GUIDE:
            return m_Settings->ShowGuideDiscord;
        case GDR_WEBS_GUIDE:
            return m_Settings->ShowGuideWebsite;
        case GDR_FACEBOOK_GUIDE:
            return m_Settings->ShowGuideFacebook;
        case GDR_AUTOEQUIP_GUIDE:
            return m_Settings->ShowGuideAutoEquip &&
                   (m_Settings->AutoEquipMaxLevel <= 0 ||
                    g_pMyPlayerObj == NULL ||
                    g_pMyPlayerObj->GetCurrentLevel() <= m_Settings->AutoEquipMaxLevel);
        case GDR_WEB_GUIDE:
            return m_Settings->ShowGuideMapLocation;
        default:
            return true;
    }
}

void CAlramGuideMgrWnd::PrepareGuides() {
    wnd_pos posMgr = GetPos();

    listGUIDES::iterator it = m_listGuides.begin();
    int usedColumns = 0;
    for (; it != m_listGuides.end(); ++it) {
        CIFWnd *pGuideIcon = (*it);
        if (!IsGuideIconEnabled(pGuideIcon->UniqueID())) {
            pGuideIcon->ShowGWnd(false);
            continue;
        }

        const int columns = GetGuideIconColumns(pGuideIcon->UniqueID());
        const int iconWidth = GetGuideIconWidth(pGuideIcon->UniqueID());
        // Event Register always starts a fresh row. With the normal first
        // four guide icons enabled this is the first control on row two; if
        // one of those icons is disabled it still keeps its intentional row.
        if (pGuideIcon->UniqueID() == GDR_EVENT_REGISTER_GUIDE && usedColumns != 0) {
            posMgr.y += (ALRAM_GUIDE_ICON_SIZE + 2);
            posMgr.x = GetPos().x;
            usedColumns = 0;
        }
        if (usedColumns + columns > ALRAM_GUIDE_MAX_ICONS_LINE) {
            posMgr.y += (ALRAM_GUIDE_ICON_SIZE + 2);
            posMgr.x = GetPos().x;
            usedColumns = 0;
        }

        posMgr.x -= iconWidth;

        pGuideIcon->MoveGWnd(posMgr.x, posMgr.y);
        pGuideIcon->ShowGWnd(true);

        usedColumns += columns;
        if (usedColumns == ALRAM_GUIDE_MAX_ICONS_LINE) {
            posMgr.y += (ALRAM_GUIDE_ICON_SIZE + 2);
            posMgr.x = GetPos().x;
            usedColumns = 0;
        }
    }

    if (m_Settings->MoveSkillBoard) {
        const ClientResolutonData &res = CGame::GetClientDimensionStuff();
        CIFWnd *playerinfo = g_pCGInterface->m_IRM.GetResObj(11, 1);
        CIFWnd *wind = g_pCGInterface->m_IRM.GetResObj(22, 1);

        /// x 220 y 10
        wind->MoveGWnd(((playerinfo->GetPos().x + playerinfo->GetSize().width) + 27), wind->GetPos().y);
    }

}

void CAlramGuideMgrWnd::RemoveAllGuides() {
    listGUIDES::iterator it = m_listGuides.begin();
    for (; it != m_listGuides.end(); ++it)
        (*it)->EraseWindowObj();
}

bool CAlramGuideMgrWnd::IsAvailableGuide(DWORD dwID) {
    listGUIDES::iterator it = m_listGuides.begin();
    for (; it != m_listGuides.end(); ++it)
        if ((*it)->UniqueID() == dwID)
            return true;
    return false;
}

CIFWnd *CAlramGuideMgrWnd::GetGuide(DWORD dwID) {
    listGUIDES::iterator it = m_listGuides.begin();
    for (; it != m_listGuides.end(); ++it)
        if ((*it)->UniqueID() == dwID)
            return (*it);
    return NULL;
}

CAlramGuideMgrWnd::~CAlramGuideMgrWnd() {
    RemoveAllGuides();
}

CIFWnd *CAlramGuideMgrWnd::CreateGuideIcon(int nWndID) {
    if(!IsGuideIconEnabled(nWndID))
        return NULL;

    // Try to find the element in the list
    listGUIDES::iterator it = m_listGuides.begin();
    for (; it != m_listGuides.end(); ++it) {
        if ((*it)->UniqueID() == nWndID) {
            return *it;
        }
    }

    // List did not contain the element, try to create it
    RECT rect = {0,
                 0,
                 GetGuideIconWidth(nWndID),
                 ALRAM_GUIDE_ICON_SIZE};

    CIFWnd* pObj = 0;

    if(CWebViewerConfig::IsWebViewerGuideId(nWndID)) {
        pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFWebViewerGuide), rect, nWndID, 0);
    } else switch(nWndID) {
        case GDR_CONFIRMREPUTATION_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFConfirmReputationGuide), rect, GDR_CONFIRMREPUTATION_GUIDE, 0);
            break;

        case GDR_EVENTGUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFEventGuide), rect, GDR_EVENTGUIDE, 0);
            break;

        case GDR_QUESTINFO_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFQuestInfoGuide), rect, GDR_QUESTINFO_GUIDE, 0);
            break;

        case GDR_LETTER_ALARM_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFLetterAlarmGuide), rect, GDR_LETTER_ALARM_GUIDE, 0);
            break;

        case GDR_SERVEREVENT_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFServerEventGuide), rect, GDR_SERVEREVENT_GUIDE, 0);
            break;

        case GDR_OPENMARKETALRAM_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFOpenMarketAlramGuide), rect, GDR_OPENMARKETALRAM_GUIDE, 0);
            break;

        case GDR_EVENTGUIDE_SECOND:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFEventGuideSecond), rect, GDR_EVENTGUIDE_SECOND, 0);
            break;
        case GDR_CHEST_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFChestGuide), rect, GDR_CHEST_GUIDE, 0);
            break;
        case GDR_MENU_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFMenuGuide), rect, GDR_MENU_GUIDE, 0);
            break;

        case GDR_LUCKY_SPIN_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFLuckySpinGuide), rect, GDR_LUCKY_SPIN_GUIDE, 0);
            break;

        case GDR_SPECIAL_OFFERS_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFSpecialOffersGuide), rect, GDR_SPECIAL_OFFERS_GUIDE, 0);
            break;

        case GDR_KILLER_ANIMATION_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFKillerAnimationGuide), rect, GDR_KILLER_ANIMATION_GUIDE, 0);
            break;

        case GDR_EVENT_REGISTER_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFEventRegisterGuide), rect, GDR_EVENT_REGISTER_GUIDE, 0);
            break;

        case GDR_DROPLOG_OPEN_ICON:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFDropLogsGuide), rect, GDR_DROPLOG_OPEN_ICON, 0);
            break;

        case GDR_PVP_CHALLENGE_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFPvpChallengeGuide), rect, GDR_PVP_CHALLENGE_GUIDE, 0);
            break;

	case GDR_AUTOEQUIP_GUIDE:
            pObj = (CIFWnd*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFAutoEquipGuide), rect, GDR_AUTOEQUIP_GUIDE, 0);
            break;

        case GDR_FACEBOOK_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFFacebookGuide), rect, GDR_FACEBOOK_GUIDE, 0);
            break;

        case GDR_DISCORD_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFDiscordGuide), rect, GDR_DISCORD_GUIDE, 0);
            break;

        case GDR_WEBS_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFWebSGuide), rect, GDR_WEBS_GUIDE, 0);
            break;

        case GDR_WEB_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFWebGuide), rect, GDR_WEB_GUIDE, 0);
            break;

        case GDR_DAILY_LOGIN:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFDailyLoginGuide), rect, GDR_DAILY_LOGIN, 0);
            break;
        case GDR_MACRO_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroGuide), rect, GDR_MACRO_GUIDE, 0);
            break;
        case GDR_ALCHEM_MACRO_GUIDE:
            pObj = (CIFWnd *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFAlchemyMacroGuide), rect, GDR_ALCHEM_MACRO_GUIDE, 0);
            break;

        default:
            return 0;
    }

    if(pObj == 0)
        return 0;

    m_listGuides.push_back(pObj);
    m_btGuidesCount++;

    PrepareGuides();

    return pObj;
}

void CAlramGuideMgrWnd::RemoveGuide(DWORD dwID) {
    listGUIDES::iterator it = m_listGuides.begin();
    for (; it != m_listGuides.end(); ++it) {
        if ((*it)->UniqueID() != dwID)
            continue;
        (*it)->ShowGWnd(false);
        m_listGuides.erase(it);
        // Hmmm why this is not m_btGuidesCount minus too?
        break;
    }
}
