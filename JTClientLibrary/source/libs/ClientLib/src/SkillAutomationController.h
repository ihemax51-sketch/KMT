#pragma once

#include <windows.h>

class CSkillAutomationController
{
public:
    CSkillAutomationController();

    bool StartAutoMastery(int masteryId);
    bool StartAutoSkill(int masteryId);
    void StopAutoMastery();
    void StopAutoSkill();
    void ResetForCharacterChange();
    void SuspendForWorldTransition();
    void OnMasteryLearnResponseProcessed();
    void OnSkillLearnResponseProcessed();
    void Tick();
    void ScheduleNextTick();
    bool HasActiveAutomation() const;

private:
    enum PendingActionType
    {
        PENDING_NONE,
        PENDING_MASTERY,
        PENDING_SKILL
    };

    bool IsWorldReady() const;
    bool TryGetMasteryLevel(int masteryId, int& level) const;
    int SelectSkillByMasteryId(int masteryId) const;
    bool IsPendingActionComplete() const;
    bool TrySendAutoMastery();
    bool TrySendAutoSkill();
    void ClearPendingAction();
    void HandlePendingTimeout();
    void UpdateTimer();

    PendingActionType m_pendingAction;
    int m_pendingMasteryId;
    int m_pendingSkillId;
    int m_pendingMasteryLevel;
    DWORD m_pendingSince;
    DWORD m_lastActionTick;
    bool m_lastActionWasSkill;
    bool m_suspended;
};

extern CSkillAutomationController g_SkillAutomationController;
