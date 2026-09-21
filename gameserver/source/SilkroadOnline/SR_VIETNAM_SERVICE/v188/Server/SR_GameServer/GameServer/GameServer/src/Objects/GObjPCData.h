//
// Created by kyuubi09 on 3/30/2023.
//
#pragma once

#include <Objects/Skill.h>
#include <map>
#include <list>
#include <Test/Test.h>
class CGObjPC;

class CGObjPCData
{
public:
    void RemoveSkillById(int skillId);
    void RemoveActiveSkillById(int skillId);
    void AddActiveSkillById(int skillId);
public:
    CGObjPC* pCGObjPC; //0x0000
    char pad_0004[548]; //0x0004
    std::map<int, CSkill*>active; //0x0228
    char pad_022C[52]; //0x022C
    std::list<CSkill*>UsedSkillList; //0x0268
private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x0274)
        ENSURE_OFFSET(active, 0x228)
        ENSURE_OFFSET(UsedSkillList, 0x268)
        END_FIXTURE()

        RUN_FIXTURE(CGObjPCData)
}; //Size: 0x0270
