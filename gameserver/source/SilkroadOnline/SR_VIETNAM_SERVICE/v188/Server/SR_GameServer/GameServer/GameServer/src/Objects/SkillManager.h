#pragma once
#include "Skill.h"
#include <list>
#include "stdarg.h"

#define __SKILL_SKILL_ACTOR_LST		std::list<sSkillActor*>
#define __SKILL_SKILL_ACTOR_LST_IT	__SKILL_SKILL_ACTOR_LST::iterator

class CSkillManager
{
public:
    __SKILL_SKILL_ACTOR_LST GetActiveBuffSkills();
};