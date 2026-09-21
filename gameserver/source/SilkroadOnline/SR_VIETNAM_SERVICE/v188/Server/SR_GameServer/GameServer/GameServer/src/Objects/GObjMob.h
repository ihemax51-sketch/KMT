//
// Created by YUMBUL on 19.03.2023.
//
#pragma once

#include <memory/hook.h>
#include <Test/Test.h>
#include "GObjNPC.h"
#include "GObjPC.h"

class CGObjMob : public CGObjNPC
{
public:
    CGObjPC* MobDamageMeter(unsigned int a2);
    void LivedpsMobAttackRecorder(void* attacker, int p2, int p3, int p4, int p5);
    static void FlushLiveDpsBatch();
    void Mobkillquest(CGObjPC* param_1, int param_2);//MyCGObjNPC_HandleAggroMap
    static CGObjMob *SpawnMob(uint32_t RefObjId, uint32_t GameWorldId, uint16_t RegionId, float PosX, float PosY, float PosZ, float GenerateRadius);
    unsigned int HandleMobKilled(IGObj* pKiller);

    CGObjMob* MonsterAdd(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned int p5);

    void MonsterRemove(CGObjMob*p1);

    static CGObjMob* CreateMob(uint32_t RefObjId, uint32_t GameWorldId, uint16_t RegionId, float PosX, float PosY, float PosZ, float GenerateRadius);
    static void DeleteMonster(int WorldID);

    static void DeleteMonsterByWorldID(int WorldID);
    static void DespawnGObj(IGObj* pObj);
    // Returns base data from object
    static void* GetRefObjBaseData(int RefObjId);
    // Gets the creep type from object
    static __int32 GetCreepType(void* RefObjBaseDataPtr);
    // Gets the behavior from object
    static void *GetRefTactic(void *RefObjBaseDataPtr);

public:
    int Monsterclass; //0x1CD8
    char pad_1CDC[76]; //0x1CDC
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x1D28)
        ENSURE_OFFSET(Monsterclass, 0x1CD8)


    END_FIXTURE()

    RUN_FIXTURE(CGObjMob)


}; //Size: 0x1D28
