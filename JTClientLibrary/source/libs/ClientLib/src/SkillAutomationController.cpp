#include "SkillAutomationController.h"

#include "GInterface.h"
#include "ICPlayer.h"
#include "IFSkill.h"
#include "GlobalDataManager.h"
#include "GlobalHelpersThatHaveNoHomeYet.h"
#include "CustomData/CustomCICPlayer.h"
#include "CustomData/CustomSettingManager.h"

namespace
{
    const DWORD SKILL_AUTOMATION_INTERVAL_MS = 130;
    const DWORD SKILL_AUTOMATION_REQUEST_TIMEOUT_MS = 3000;

    bool IsValidPlayerState()
    {
        return g_pMyPlayerObj != 0x0 &&
               g_pMyPlayerObj->CHARACTER_STATUS != Dead &&
               g_pMyPlayerObj->CHARACTER_STATUS != Stall &&
               g_pMyPlayerObj->CHARACTER_STATUS != 0 &&
               g_pMyPlayerObj->GetCurrentHp() != 0;
    }
}

CSkillAutomationController g_SkillAutomationController;

CSkillAutomationController::CSkillAutomationController()
    : m_pendingAction(PENDING_NONE),
      m_pendingMasteryId(0),
      m_pendingSkillId(0),
      m_pendingMasteryLevel(0),
      m_pendingSince(0),
      m_lastActionTick(0),
      m_lastActionWasSkill(false),
      m_suspended(false)
{
}

bool CSkillAutomationController::HasActiveAutomation() const
{
    return m_Player != 0x0 &&
           (m_Player->Enabled_AutoMastery || m_Player->Enabled_AutoSkill);
}

bool CSkillAutomationController::IsWorldReady() const
{
    if (!m_Settings || !m_Player ||
        !g_pCGInterface || !g_pCGInterface->GetMainPopup() ||
        !g_pMyPlayerObj || !IsValidPlayerState())
    {
        return false;
    }

    CIFSkill* skillWindow = g_pCGInterface->GetMainPopup()->GetSkill();
    return skillWindow != 0x0;
}

bool CSkillAutomationController::TryGetMasteryLevel(int masteryId, int& level) const
{
    level = 0;

    if (!g_pCGInterface || !g_pCGInterface->GetMainPopup())
        return false;

    CIFSkill* skillWindow = g_pCGInterface->GetMainPopup()->GetSkill();
    if (!skillWindow)
        return false;

    // The live client normally uses the mastery ID as the map key, but the
    // payload also carries the authoritative ID. Keep the keyed lookup fast
    // and retain the legacy-compatible fallback for partially rebuilt maps.
    std::map<int, sMasteryData*>::const_iterator masteryIt =
        skillWindow->sSkillMaps.m_masteryData.find(masteryId);
    if (masteryIt != skillWindow->sSkillMaps.m_masteryData.end() &&
        masteryIt->second && masteryIt->second->nMasteryId == masteryId)
    {
        level = masteryIt->second->btLevel;
        return true;
    }

    for (masteryIt = skillWindow->sSkillMaps.m_masteryData.begin();
         masteryIt != skillWindow->sSkillMaps.m_masteryData.end(); ++masteryIt)
    {
        if (masteryIt->second && masteryIt->second->nMasteryId == masteryId)
        {
            level = masteryIt->second->btLevel;
            return true;
        }
    }

    return false;
}

int CSkillAutomationController::SelectSkillByMasteryId(int masteryId) const
{
    if (!g_pMyPlayerObj || !g_CGlobalDataManager || !g_pCGInterface ||
        !g_pCGInterface->GetMainPopup())
        return 0;

    CIFSkill* skillWindow = g_pCGInterface->GetMainPopup()->GetSkill();
    if (!skillWindow)
        return 0;

    int masteryLevel = 0;
    if (!TryGetMasteryLevel(masteryId, masteryLevel))
        return 0;

    for (std::map<unsigned __int32, CSkillData*>::const_iterator skillIter =
             g_CGlobalDataManager->m_skillDataMap.begin();
         skillIter != g_CGlobalDataManager->m_skillDataMap.end(); ++skillIter)
    {
        CSkillData* skill = skillIter->second;
        if (!skill)
            continue;

        if ((skill->UI_SkillTab == 255 && skill->UI_SkillPage == 255) ||
            skill->ReqCommon_Mastery1 != masteryId ||
            skill->ID == 11828)
        {
            continue;
        }

        if (skillWindow->sSkillMaps.SkillIsLearned_MAYBE(skill->ID) ||
            skillWindow->sSkillMaps.m_SkillData.find(skill->ID) !=
                skillWindow->sSkillMaps.m_SkillData.end())
        {
            continue;
        }

        if (skill->ReqCommon_MasteryLevel1 > masteryLevel ||
            skill->ReqLearn_SP > g_pMyPlayerObj->m_nSkillPoint)
        {
            continue;
        }

        if (skill->ReqLearn_Skill1 > 0 && skill->ReqLearn_Skill2 > 0)
        {
            CSkillData* condition1 =
                g_CGlobalDataManager->GetSkillbyGroupIDandBasicLevel(
                    skill->ReqLearn_Skill1, skill->ReqLearn_SkillLevel1);
            CSkillData* condition2 =
                g_CGlobalDataManager->GetSkillbyGroupIDandBasicLevel(
                    skill->ReqLearn_Skill2, skill->ReqLearn_SkillLevel2);

            if (condition1 && condition2 &&
                skillWindow->sSkillMaps.SkillIsLearned_MAYBE(condition1->ID) &&
                skillWindow->sSkillMaps.SkillIsLearned_MAYBE(condition2->ID))
            {
                return skill->ID;
            }
        }
        else if (skill->ReqLearn_Skill2 > 0)
        {
            CSkillData* condition2 =
                g_CGlobalDataManager->GetSkillbyGroupIDandBasicLevel(
                    skill->ReqLearn_Skill2, skill->ReqLearn_SkillLevel2);

            if (condition2 &&
                skillWindow->sSkillMaps.SkillIsLearned_MAYBE(condition2->ID))
            {
                return skill->ID;
            }
        }
        else if (skill->ReqLearn_Skill1 > 0)
        {
            CSkillData* condition1 =
                g_CGlobalDataManager->GetSkillbyGroupIDandBasicLevel(
                    skill->ReqLearn_Skill1, skill->ReqLearn_SkillLevel1);

            if (condition1 &&
                skillWindow->sSkillMaps.SkillIsLearned_MAYBE(condition1->ID))
            {
                return skill->ID;
            }
        }
        else
        {
            return skill->ID;
        }
    }

    return 0;
}

bool CSkillAutomationController::StartAutoMastery(int masteryId)
{
    if (!m_Player || masteryId <= 0)
        return false;

    m_Player->Enabled_AutoMastery = true;
    m_Player->Selected_AutoMasteryId = masteryId;
    m_suspended = false;
    m_lastActionTick = GetTickCount() - SKILL_AUTOMATION_INTERVAL_MS;

    if (m_pendingAction != PENDING_NONE && m_pendingMasteryId != masteryId)
        ClearPendingAction();

    UpdateTimer();
    return true;
}

bool CSkillAutomationController::StartAutoSkill(int masteryId)
{
    if (!m_Player || masteryId <= 0)
        return false;

    m_Player->Enabled_AutoSkill = true;
    m_Player->Selected_AutoSkillMasteryId = masteryId;
    m_Player->Selected_AutoSkillId = 0;
    m_suspended = false;
    m_lastActionTick = GetTickCount() - SKILL_AUTOMATION_INTERVAL_MS;

    if (m_pendingAction != PENDING_NONE && m_pendingMasteryId != masteryId)
        ClearPendingAction();

    UpdateTimer();
    return true;
}

void CSkillAutomationController::StopAutoMastery()
{
    if (m_Player)
    {
        m_Player->Enabled_AutoMastery = false;
        m_Player->Selected_AutoMasteryId = 0;
    }

    if (m_pendingAction == PENDING_MASTERY)
        ClearPendingAction();

    UpdateTimer();
}

void CSkillAutomationController::StopAutoSkill()
{
    if (m_Player)
    {
        m_Player->Enabled_AutoSkill = false;
        m_Player->Selected_AutoSkillMasteryId = 0;
        m_Player->Selected_AutoSkillId = 0;
    }

    if (m_pendingAction == PENDING_SKILL)
        ClearPendingAction();

    UpdateTimer();
}

void CSkillAutomationController::ResetForCharacterChange()
{
    if (m_Player)
    {
        m_Player->Enabled_AutoMastery = false;
        m_Player->Selected_AutoMasteryId = 0;
        m_Player->Enabled_AutoSkill = false;
        m_Player->Selected_AutoSkillMasteryId = 0;
        m_Player->Selected_AutoSkillId = 0;
    }

    m_suspended = false;
    m_lastActionWasSkill = false;
    ClearPendingAction();

    if (g_pCGInterface)
        g_pCGInterface->KillTimer(SKILL_AUTOMATION_TIMER);
}

void CSkillAutomationController::SuspendForWorldTransition()
{
    m_suspended = true;
    ClearPendingAction();
    UpdateTimer();
}

void CSkillAutomationController::ClearPendingAction()
{
    m_pendingAction = PENDING_NONE;
    m_pendingMasteryId = 0;
    m_pendingSkillId = 0;
    m_pendingMasteryLevel = 0;
    m_pendingSince = 0;
}

bool CSkillAutomationController::IsPendingActionComplete() const
{
    if (m_pendingAction == PENDING_MASTERY)
    {
        int currentLevel = 0;
        return TryGetMasteryLevel(m_pendingMasteryId, currentLevel) &&
               currentLevel > m_pendingMasteryLevel;
    }

    if (m_pendingAction == PENDING_SKILL && g_pCGInterface &&
        g_pCGInterface->GetMainPopup())
    {
        CIFSkill* skillWindow = g_pCGInterface->GetMainPopup()->GetSkill();
        if (!skillWindow)
            return false;

        return skillWindow->sSkillMaps.SkillIsLearned_MAYBE(m_pendingSkillId) ||
               skillWindow->sSkillMaps.m_SkillData.find(m_pendingSkillId) !=
                   skillWindow->sSkillMaps.m_SkillData.end();
    }

    return false;
}

bool CSkillAutomationController::TrySendAutoMastery()
{
    if (!m_Player || !m_Player->Enabled_AutoMastery)
        return false;

    int masteryId = m_Player->Selected_AutoMasteryId;
    int masteryLevel = 0;
    if (masteryId <= 0)
    {
        StopAutoMastery();
        return false;
    }

    if (!TryGetMasteryLevel(masteryId, masteryLevel))
        return false;

    const bool chinese = g_pMyPlayerObj->IsChinese();
    const int raceSpecificLimit = chinese
        ? m_Settings->ChineseMasteryLimit
        : m_Settings->EuropeanMasteryLimit;
    const int masteryLimit = raceSpecificLimit > 0
        ? raceSpecificLimit
        : m_Settings->MaxMasteryLevel;

    if (m_Settings->ServerMaxLevel <= 1 ||
        masteryLevel + 1 >= m_Settings->ServerMaxLevel ||
        g_pCGInterface->GetMainPopup()->GetSkill()->sSkillMaps.GetCurrentMasteryCount() >= masteryLimit)
    {
        StopAutoMastery();
        return false;
    }

    if (masteryLevel >= g_pMyPlayerObj->GetCurrentLevel())
        return false;

    unsigned int requiredSp =
        g_CGlobalDataManager->GetLevelData(masteryLevel + 1).m_expM;
    if (g_pMyPlayerObj->m_nSkillPoint < 0 ||
        static_cast<unsigned int>(g_pMyPlayerObj->m_nSkillPoint) < requiredSp)
        return false;

    CMsgStreamBuffer buf(0x70A2);
    buf << masteryId << (byte)1;
    SendMsg(buf);

    m_pendingAction = PENDING_MASTERY;
    m_pendingMasteryId = masteryId;
    m_pendingMasteryLevel = masteryLevel;
    m_pendingSince = GetTickCount();
    m_lastActionWasSkill = false;
    return true;
}

bool CSkillAutomationController::TrySendAutoSkill()
{
    if (!m_Player || !m_Player->Enabled_AutoSkill)
        return false;

    int masteryId = m_Player->Selected_AutoSkillMasteryId;
    int masteryLevel = 0;
    if (masteryId <= 0)
    {
        StopAutoSkill();
        return false;
    }

    if (!TryGetMasteryLevel(masteryId, masteryLevel))
        return false;

    int skillId = SelectSkillByMasteryId(masteryId);
    if (skillId <= 0 || skillId > 100000)
        return false;

    CMsgStreamBuffer buf(0x70A1);
    buf << skillId;
    SendMsg(buf);

    m_Player->Selected_AutoSkillId = skillId;
    m_pendingAction = PENDING_SKILL;
    m_pendingMasteryId = masteryId;
    m_pendingSkillId = skillId;
    m_pendingSince = GetTickCount();
    m_lastActionWasSkill = true;
    return true;
}

void CSkillAutomationController::HandlePendingTimeout()
{
    // A timeout only means the native skill/mastery containers did not expose
    // the expected state change before our next check. Keep the requested
    // automation active and let the next timer tick re-evaluate live state.
    ClearPendingAction();
    m_lastActionTick = GetTickCount();
}

void CSkillAutomationController::OnMasteryLearnResponseProcessed()
{
    if (m_pendingAction != PENDING_MASTERY)
        return;

    if (IsPendingActionComplete())
    {
        ClearPendingAction();
        m_lastActionTick = GetTickCount();
    }
}

void CSkillAutomationController::OnSkillLearnResponseProcessed()
{
    if (m_pendingAction != PENDING_SKILL)
        return;

    if (IsPendingActionComplete())
    {
        ClearPendingAction();
        m_lastActionTick = GetTickCount();
    }
}

void CSkillAutomationController::Tick()
{
    if (!HasActiveAutomation())
    {
        ClearPendingAction();
        return;
    }

    if (!IsWorldReady())
    {
        ClearPendingAction();
        return;
    }

    if (m_suspended)
    {
        m_suspended = false;
    }

    const DWORD now = GetTickCount();
    if (m_pendingAction != PENDING_NONE)
    {
        if (IsPendingActionComplete())
        {
            ClearPendingAction();
            m_lastActionTick = now;
        }
        else if (now - m_pendingSince >= SKILL_AUTOMATION_REQUEST_TIMEOUT_MS)
        {
            HandlePendingTimeout();
        }
        return;
    }

    if (now - m_lastActionTick < SKILL_AUTOMATION_INTERVAL_MS)
        return;

    const bool skillEnabled = m_Player->Enabled_AutoSkill;
    const bool masteryEnabled = m_Player->Enabled_AutoMastery;

    if (skillEnabled && masteryEnabled)
    {
        if (m_lastActionWasSkill)
        {
            if (TrySendAutoMastery() || TrySendAutoSkill())
                return;
        }
        else if (TrySendAutoSkill() || TrySendAutoMastery())
        {
            return;
        }
    }
    else if (skillEnabled)
    {
        TrySendAutoSkill();
    }
    else if (masteryEnabled)
    {
        TrySendAutoMastery();
    }
}

void CSkillAutomationController::UpdateTimer()
{
    if (!g_pCGInterface)
        return;

    if (HasActiveAutomation())
    {
        g_pCGInterface->KillTimer(SKILL_AUTOMATION_TIMER);
        g_pCGInterface->StartTimer(SKILL_AUTOMATION_TIMER,
                                    SKILL_AUTOMATION_INTERVAL_MS);
    }
    else
    {
        g_pCGInterface->KillTimer(SKILL_AUTOMATION_TIMER);
    }
}

void CSkillAutomationController::ScheduleNextTick()
{
    UpdateTimer();
}
