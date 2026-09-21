#include "SkillManager.h"

#define MEMUTIL_READ_BY_PTR_OFFSET(ptr, offset, type) \
	*(type*)(((uintptr_t)ptr) + offset)


__SKILL_SKILL_ACTOR_LST CSkillManager::GetActiveBuffSkills()
{
    __SKILL_SKILL_ACTOR_LST lst = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x268, __SKILL_SKILL_ACTOR_LST);
    return lst;
}