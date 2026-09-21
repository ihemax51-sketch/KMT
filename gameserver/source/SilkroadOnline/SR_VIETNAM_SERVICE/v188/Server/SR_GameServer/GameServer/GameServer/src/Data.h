//
// Created by Kurama on 12/11/2022.
//
#pragma once

#include "Windows.h"

///  !!!!! STOLEN FROM CHERNOBYL GS LIB !!!!!

#pragma pack(push, 4)
struct SAggroMapSecondPairItem {
    DWORD dwPlayerGID;
    DWORD dwDamage;
};
#pragma pack(pop)

#pragma pack(push, 1)

struct SWorldID {
    union {
        struct {
            WORD wLayerID;
            WORD wWorldID;
        };
        DWORD dwWorldID;
    };

    SWorldID(WORD wLayerID, WORD wWorldID) {
        this->wLayerID = wLayerID;
        this->wWorldID = wWorldID;
    }

    SWorldID(DWORD dwWorldID) {
        this->dwWorldID = dwWorldID;
    }

    SWorldID() : wLayerID(0), wWorldID(0) {

    }
};

#pragma pack(pop)

#pragma pack(push, 1)

struct SPosInfo {
public:
    WORD wRegionID;
    float fltX;
    float fltY;
    float fltZ;

    SPosInfo(WORD wRegionID, float fltX, float fltY, float fltZ) {
        this->wRegionID = wRegionID;
        this->fltX = fltX;
        this->fltY = fltY;
        this->fltZ = fltZ;
    }

    SPosInfo() : wRegionID(0), fltX(0.0f), fltY(0.0f), fltZ(0.0f) {

    }
};

#pragma pack(pop)

#pragma pack(push, 1)

struct STeleportPos {
public:
    WORD wRegionID;
    WORD wUnk1;
    float fltX;
    float fltY;
    float fltZ;

    STeleportPos(WORD wRegionID, float fltX, float fltY, float fltZ) {

        this->wRegionID = wRegionID;
        //Not sure about this one, but its 6DDD for instance world (dimension hole for example).
        this->wUnk1 = 0x0000;
        this->fltX = fltX;
        this->fltY = fltY;
        this->fltZ = fltZ;
    }

    STeleportPos(SPosInfo posInfo) {
        this->wRegionID = posInfo.wRegionID;
        this->wUnk1 = 0x0000;
        this->fltX = posInfo.fltX;
        this->fltY = posInfo.fltY;
        this->fltZ = posInfo.fltZ;
    }

    STeleportPos() : wRegionID(0), wUnk1(0), fltX(0.0f), fltY(0.0f), fltZ(0.0f) {

    }
};

#pragma pack(pop)

enum LIFESTATE : UINT8 {
    LIFESTATE_EMBRYO = 0,
    LIFESTATE_ALIVE = 1,
    LIFESTATE_DEAD = 2,
    LIFESTATE_GONE = 3,
};

enum MOTIONSTATE : UINT8 {
    MOTIONSTATE_STAND = 0,
    MOTIONSTATE_SKILL = 1,
    MOTIONSTATE_WALK = 2,
    MOTIONSTATE_RUN = 3,
    MOTIONSTATE_SIT = 4,
    MOTIONSTATE_JUMP = 5,
    MOTIONSTATE_SWIM = 6,
    MOTIONSTATE_RIDE = 7,
    MOTIONSTATE_KNOCKDOWN = 8,
    MOTIONSTATE_STUN = 9,
    MOTIONSTATE_FROZEN = 10,
    MOTIONSTATE_HIT = 11,
    MOTIONSTATE_REQ_HELP = 12,
    MOTIONSTATE_PAO = 13,
    MOTIONSTATE_COUNTERATTACK = 14,
    MOTIONSTATE_SKILL_ACTIONOFF = 15,
    MOTIONSTATE_SKILL_KNOCKBACK = 16,
    MOTIONSTATE_PROTECTIONWALL = 17,
    MOTIONSTATE_CHANGEMOTION = 18,
    MOTIONSTATE_SLEEP = 19,
    MOTIONSTATE_STONE = 20,
};

enum BODYMODE : UINT8 {
    BODYMODE_NORMAL = 0,
    BODYMODE_HWAN = 1,
    BODYMODE_INVICIBLE = 2,
    BODYMODE_INVISIBLE = 3,
    BODYMODE_BERSERKER = 4,
    BODYMODE_GM_INVISIBLE = 5,
    BODYMODE_STEALTH = 6,
    BODYMODE_SKILL_INVISIBLE = 7,
};

enum JOBSTATE : UINT8 {
    JOBSTATE_NONE = 0,
};

enum TELEPORTSTATE : UINT8 {
    TELEPORTSTATE_NONE = 0,
};

enum MONSTERCLASS : UINT8 {
    MONSTERCLASS_NONE = 0,
};

enum GOBJ_PARAM : UINT16 {
    PARAM_LEVEL = 0,
    PARAM_STRENGTH = 1,
    PARAM_INTELLECT = 2,
    PARAM_MAX_HEALTH = 3,
    PARAM_MAX_MANA = 4,
    PARAM_PHY_DEF = 5,
    PARAM_MAG_DEF = 6,
    PARAM_PARRY_RATIO = 9,
    PARAM_HIT_RATE = 11,
    PARAM_PHY_ATTACK_MIN = 13,
    PARAM_PHY_ATTACK_MAX = 14,
    PARAM_MAG_ATTACK_MIN = 15,
    PARAM_MAG_ATTACK_MAX = 16,
    PARAM_PHY_ATTACK_MIN2 = 17,
    PARAM_PHY_ATTACK_MAX2 = 18,
    PARAM_MAG_ATTACK_MIN2 = 19,
    PARAM_MAG_ATTACK_MAX2 = 20,

    //PARAM_ADD_STRENGTH 0x36, 54
    //PARAM_ADD_INTELLECT 0x37, 55

    //used for special equipset boost (garment/protector)
    PARAM_MANA_CONSUMPTION_RELATED = 23,
    PARAM_MOVEMENT_SPEED = 24,
    PARAM_MAX_HP2 = 54,
    PARAM_MAX_MANA2 = 55,

    PARAM_COUNT = 61
};

struct stObjectStates {
    LIFESTATE m_btLifeState; //0x0000
    char pad_0001[1]; //0x0001
    MOTIONSTATE m_btMotionState; //0x0002
    BODYMODE m_btBodyState; //0x0003
    char pad_0004[11]; //0x0004
    JOBSTATE m_btJobState; //0x000F
    TELEPORTSTATE m_btTeleportState; //0x0010
    // ..
};