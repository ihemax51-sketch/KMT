//
// Created by kyuubi09 on 3/30/2023.
//

#include <memory/hook.h>
#include "GObjPCData.h"


void CGObjPCData::RemoveSkillById(int skillId) {
    std::list<CSkill*>::iterator it = UsedSkillList.begin();

    for (; it != UsedSkillList.end(); ++it) {
        SSkillPreEngagementData* preEngagementData = (*it)->m_pSkillPreEngagementData;
        if (preEngagementData && preEngagementData->m_nSkillId == skillId) {
            (*it)->RemoveSkill();
            break;
        }
    }
}

void CGObjPCData::RemoveActiveSkillById(int skillId) {

    for (std::map<int, CSkill*>::iterator it = active.begin(); it != active.end(); ++it) {
        if (it->first == skillId) {
            active.erase(skillId);
            break;
        }
    }
}
void CGObjPCData::AddActiveSkillById(int skillId) {

    /*for (std::map<int, CSkill*>::iterator it = active.begin(); it != active.end(); ++it) {
        if (it->first == skillId) {
            active.erase(skillId);
            break;
        }
    }*/
}