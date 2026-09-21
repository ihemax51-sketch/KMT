//
// Created by kyuubi09 on 3/30/2023.
//
#pragma once

#include <memory/RemodelUtility.h>

struct  SSkillPreEngagementData
{
public:
    char pad_0000[0x8];
    int m_nSkillId;
    int m_nTimedJobRelated;
};
class CSkill {
public:
    void RemoveSkill();

public:
    char pad_0000[0x14]; //0x0000
    SSkillPreEngagementData* m_pSkillPreEngagementData; //0x0014
};

#pragma pack(push, 1)
struct sSkillPreEngageData
{
#define ADHOC_PADDING 92 - 16

public:
    REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(sSkillPreEngageData);
    int nUnk1; //1
    int nSkillID;
    int nInstanceID;
    //REMODEL_PAD_TO_END(sSkillPreEngageData, pad, 92);
    //at 0x4C -> byte option
    //... th ere are more
    char pad[ADHOC_PADDING];
};
#pragma pack(pop)

//sizeof = 124

#pragma pack(push, 1)
struct sSkillActorData
{
public:
    REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(sSkillActorData);
    DWORD dwUnk_0x04;
    void* pSkillData;
    char pad[124 - 12];
};

#pragma pack(pop)

#pragma pack(push, 1)
struct sSkillActor
{
public:
    REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(sSkillActor);

    DWORD dwUnk_0x04;
    DWORD dwUnk_0x08;
    DWORD dwUnk_0x0C;
    DWORD dwUnk_0x10;
    sSkillPreEngageData* pPreEngageData;
    sSkillActorData* pActorData;
    DWORD dwUnk_0x1C;
    DWORD dwUnk_0x20;
};
#pragma pack(pop)