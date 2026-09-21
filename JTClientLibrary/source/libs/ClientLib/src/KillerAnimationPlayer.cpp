#include "KillerAnimationPlayer.h"

#include "CObjAnimation.h"
#include "CObjCharacter.h"
#include "GInterface.h"
#include "ICPlayer.h"
#include "IObject.h"
#include "unsorted.h"
#include <Windows.h>
#include <map>
#include <vector>

namespace KillerAnimationPlayer {
namespace {
    enum AnimationTargetKind {
        TARGET_CURRENT_PLAYER = 0,
        TARGET_PLAYER_UNIQUE_ID = 1,
        TARGET_OBJECT_UNIQUE_ID = 2
    };

    struct PendingAnimation {
        AnimationTargetKind kind;
        unsigned int uniqueId;
        int animationId;
        int blendIn;
        int duration;
        unsigned int cooldownMs;
        DWORD nextAttemptTick;
        int attempts;
    };

    typedef std::vector<PendingAnimation> PendingAnimationList;
    PendingAnimationList g_pendingAnimations;
    std::map<unsigned int, DWORD> g_lastPlayedByKey;

    const int kMaxQueuedAnimations = 64;
    const int kMaxResolveAttempts = 5;
    const DWORD kInitialQueueDelayMs = 50;
    const DWORD kRetryDelayMs = 120;
    const DWORD kAnimationQueueTimerMs = 30;

    unsigned int BuildCooldownKey(AnimationTargetKind kind, unsigned int uniqueId)
    {
        return (static_cast<unsigned int>(kind) << 28) ^ uniqueId;
    }

    void TrimPendingQueueIfNeeded()
    {
        while (g_pendingAnimations.size() > kMaxQueuedAnimations)
            g_pendingAnimations.erase(g_pendingAnimations.begin());
    }

    void ScheduleQueueTimer()
    {
        if (!g_pCGInterface || g_pendingAnimations.empty())
            return;

        g_pCGInterface->KillTimer(ANIMATION_QUEUE_TIMER);
        g_pCGInterface->StartTimer(ANIMATION_QUEUE_TIMER, kAnimationQueueTimerMs);
    }

    bool Queue(AnimationTargetKind kind, unsigned int uniqueId, int animationId, int blendIn, int duration, unsigned int cooldownMs)
    {
        if (!IsValidAnimationId(animationId))
            return false;

        PendingAnimation pending;
        pending.kind = kind;
        pending.uniqueId = uniqueId;
        pending.animationId = animationId;
        pending.blendIn = blendIn;
        pending.duration = duration;
        pending.cooldownMs = cooldownMs;
        pending.nextAttemptTick = GetTickCount() + kInitialQueueDelayMs;
        pending.attempts = 0;

        g_pendingAnimations.push_back(pending);
        TrimPendingQueueIfNeeded();
        ScheduleQueueTimer();
        return true;
    }

    CIObject* ResolveTargetObject(const PendingAnimation& pending)
    {
        if (pending.kind == TARGET_CURRENT_PLAYER)
            return g_pMyPlayerObj;

        if (pending.kind == TARGET_PLAYER_UNIQUE_ID)
        {
            if (!g_pMyPlayerObj)
                return 0;

            return g_pMyPlayerObj->GetCICPlayerByUniqueID(pending.uniqueId);
        }

        return GetCharacterObjectByID_MAYBE(pending.uniqueId);
    }
}

bool IsValidAnimationId(int animationId)
{
    return animationId > 0 && animationId <= 500;
}

bool PlayOnAnimationObject(void* animationObject, int animationId, int blendIn, int duration)
{
    if (!animationObject || !IsValidAnimationId(animationId)) {
        return false;
    }

    bool played = false;
    __try {
        reinterpret_cast<CCObjAnimation*>(animationObject)->Func_3(animationId, blendIn, duration, 0, 1.0f, 0.1f);
        played = true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        played = false;
    }

    return played;
}

bool PlayOnPlayer(CICPlayer* player, int animationId)
{
    if (!player || !player->m_pCCObjAnimation) {
        return false;
    }

    return PlayOnAnimationObject(player->m_pCCObjAnimation, animationId, 0, 750);
}

bool PlayOnCurrentPlayer(int animationId)
{
    return PlayOnPlayer(g_pMyPlayerObj, animationId);
}

bool QueueOnCurrentPlayer(int animationId, int blendIn, int duration)
{
    return Queue(TARGET_CURRENT_PLAYER, 0, animationId, blendIn, duration, 0);
}

bool QueueOnPlayerUniqueId(unsigned int uniqueId, int animationId, int blendIn, int duration)
{
    if (uniqueId == 0)
        return false;

    return Queue(TARGET_PLAYER_UNIQUE_ID, uniqueId, animationId, blendIn, duration, 0);
}

bool QueueOnObjectUniqueId(unsigned int uniqueId, int animationId, int blendIn, int duration, unsigned int cooldownMs)
{
    if (uniqueId == 0)
        return false;

    return Queue(TARGET_OBJECT_UNIQUE_ID, uniqueId, animationId, blendIn, duration, cooldownMs);
}

void ProcessPendingAnimations()
{
    if (g_pendingAnimations.empty())
        return;

    const DWORD now = GetTickCount();

    for (PendingAnimationList::iterator it = g_pendingAnimations.begin(); it != g_pendingAnimations.end();)
    {
        if ((LONG)(now - it->nextAttemptTick) < 0)
        {
            ++it;
            continue;
        }

        const unsigned int cooldownKey = BuildCooldownKey(it->kind, it->uniqueId);
        if (it->cooldownMs > 0)
        {
            std::map<unsigned int, DWORD>::const_iterator cooldownIt = g_lastPlayedByKey.find(cooldownKey);
            if (cooldownIt != g_lastPlayedByKey.end() &&
                now - cooldownIt->second < it->cooldownMs)
            {
                it = g_pendingAnimations.erase(it);
                continue;
            }
        }

        CIObject* object = ResolveTargetObject(*it);
        if (!object || !object->m_pCCObjAnimation)
        {
            ++it->attempts;
            if (it->attempts >= kMaxResolveAttempts)
            {
                it = g_pendingAnimations.erase(it);
            }
            else
            {
                it->nextAttemptTick = now + kRetryDelayMs;
                ++it;
            }
            continue;
        }

        if (PlayOnAnimationObject(object->m_pCCObjAnimation, it->animationId, it->blendIn, it->duration))
        {
            if (it->cooldownMs > 0)
                g_lastPlayedByKey[cooldownKey] = now;

            it = g_pendingAnimations.erase(it);
            continue;
        }

        ++it->attempts;
        if (it->attempts >= kMaxResolveAttempts)
        {
            it = g_pendingAnimations.erase(it);
        }
        else
        {
            it->nextAttemptTick = now + kRetryDelayMs;
            ++it;
        }
    }

    ScheduleQueueTimer();
}

}
