#pragma once

class CICPlayer;

namespace KillerAnimationPlayer {
    bool IsValidAnimationId(int animationId);
    bool PlayOnAnimationObject(void* animationObject, int animationId, int blendIn = 0, int duration = 750);
    bool PlayOnPlayer(CICPlayer* player, int animationId);
    bool PlayOnCurrentPlayer(int animationId);
    bool QueueOnCurrentPlayer(int animationId, int blendIn = 0, int duration = 750);
    bool QueueOnPlayerUniqueId(unsigned int uniqueId, int animationId, int blendIn = 0, int duration = 750);
    bool QueueOnObjectUniqueId(unsigned int uniqueId, int animationId, int blendIn = 0, int duration = 750, unsigned int cooldownMs = 0);
    void ProcessPendingAnimations();
}
