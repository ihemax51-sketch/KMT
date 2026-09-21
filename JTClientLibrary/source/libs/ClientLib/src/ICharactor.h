#pragma once

#include "ClassLink.h"
#include "Data/CharacterData.h"
#include "IGIDObject.h"
#include "support/AbiTest.h"
#include "IDecoSkill.h"
#include <ghidra/undefined.h>

enum TRIJOB_TYPE : char { /* Job Type of the Player */
    TRIJOB_HUNTER = 3,
    TRIJOB_NOJOB = 4,
    TRIJOB_THIEF = 2,
    TRIJOB_TRADER = 1
};

enum eAbnormalState : unsigned int {
    ABNORMAL_STATE_NONE = 0,
    ABNORMAL_STATE_FROZEN = 1,
    ABNORMAL_STATE_FROSTBITTEN = 2,
    ABNORMAL_STATE_SHOCKED = 4,
    ABNORMAL_STATE_BURN = 8,
    ABNORMAL_STATE_POISONED = 16,
    ABNORMAL_STATE_ZOMBIE = 32,
    ABNORMAL_STATE_SLEEP = 64,
    ABNORMAL_STATE_BIND = 128,
    ABNORMAL_STATE_DULL = 256,
    ABNORMAL_STATE_FEAR = 512,
    ABNORMAL_STATE_SHORTSIGHTED = 1024,
    ABNORMAL_STATE_BLEED = 2048,
    ABNORMAL_STATE_PETRIFY = 4096,
    ABNORMAL_STATE_DARKNESS = 8192,
    ABNORMAL_STATE_STUNNED = 16384,
    ABNORMAL_STATE_DISEASE = 32768,
    ABNORMAL_STATE_CONFUSION = 65536,
    ABNORMAL_STATE_DECAY = 131072,
    ABNORMAL_STATE_WEAK = 262144,
    ABNORMAL_STATE_IMPOTENT = 524288,
    ABNORMAL_STATE_DIVISION = 1048576,
    ABNORMAL_STATE_PANIC = 2097152,
    ABNORMAL_STATE_COMBUSTION = 4194304,
    ABNORMAL_STATE_BIT23 = 8388608,
    ABNORMAL_STATE_HIDDEN = 16777216,
    ABNORMAL_STATE_BIT25 = 33554432,
    ABNORMAL_STATE_BIT26 = 67108864,
    ABNORMAL_STATE_BIT27 = 134217728,
    ABNORMAL_STATE_BIT28 = 268435456,
    ABNORMAL_STATE_BIT29 = 536870912,
    ABNORMAL_STATE_BIT30 = 1073741824,
    ABNORMAL_STATE_BIT31 = 2147483648,

};
enum CHARACTER_STATUS : char { /* Job Type of the Player */
    Dead = 2,
    Alive = 9,
    Sit = 65,
    NormalAttack = 13,
    SkillCast = 5,
    Stall = 1
};
class CICharactor : public CIGIDObject {
    friend CClassLink<CICharactor>;
public:
    union uCharacterState {
        unsigned short state;
        struct {
            unsigned char stateH;
            unsigned char stateL;
        };
    };

    /// \address 009cf1b0
    void Func_15(int param_1, float *param_2) override;
    void Func_15_impl(int param_1, float *param_2);
    void SetTitleName(char hwanlevel);
    virtual void Func_34();

    virtual void Func_35();

    virtual const SCommonData &Func_36(); // TODO:: could lead to crashes further on

    virtual void Func_37();

    virtual void Func_38();

    virtual void SetMaxHP();

    virtual void Func_40();

    virtual void Func_41();

    virtual void Func_42();

    virtual TRIJOB_TYPE GetJobType();

    virtual void SetJobType(TRIJOB_TYPE type);

    virtual void Func_45();

    virtual void Func_46();

    virtual void Func_47();

    virtual void Func_48();

    virtual void Func_49();

    virtual void Func_50();

    virtual void Func_51();

    virtual void Func_52();

    virtual void Func_53();

    virtual void Func_54();

public:

    /// \brief Display the given text message above the entity for a certain amount of time.
    /// \address 009c77c0
    /// \param message The message to be displayed
    /// \param color Color of the letters being displayed
    void ShowMessageAboveEntity(const std::n_wstring &message, D3DCOLOR color);


    unsigned int GetCurrentHp() const;

    unsigned int GetMaxHp() const;

    unsigned int GetCurrentMp() const;

    unsigned int GetMaxMp() const;

    /// \address 009c7880
    unsigned int FUN_009c7880() const;

public:
    char pad_0254[32]; //0x0254
    class CClassLink<CICharactor> classLink; //0x0274
    std::list<CIDecoSkill*> decolist; //0x0284
    char pad_0290[0x02C4 - 0x0290]; //0x0290
public:
    eAbnormalState m_abnormalState; // 0x02C4
    std::list<int> poisonlist;
    char pad_02C8[380]; //0x02C8
    int unkhp; //0x0450
    int m_mpCurrent; //0x0454
    int m_hpMax; //0x0458
    int m_mpMax; //0x045C
    int m_hpCurrent; //0x0460
    char pad_0464[4]; //0x0464
    unsigned int N00000A38; //0x0468
    char pad_046C[8]; //0x046C
protected:
    class CGFontTexture m_textureMessageAbove; //0x0474
    D3DCOLOR m_colorMessageAbove; //0x04DC
public:
    char pad_04e0[16]; //0x04E0
    float m_renderY; //0x04F0
    float m_renderX; //0x04F4
    float m_renderDepth; //0x04F8
    char pad_04FC[17]; //0x04FC
    TRIJOB_TYPE b_JobType; //0x050D
    char pad_050E[332]; //0x050E
    short N00000AFC; //0x065A:
    byte CHARACTER_STATUS; //0x065C
    byte Dead0Stay1Walking2Sit0SkillCast0emotion33Stall12817isridingpet;
    byte unk123;
    byte unk1234;
    char pad_065E[8]; //0x0660
    float m_walkSpeed; //0x066C
    float m_runSpeed; //0x0670
    char gap[4];
    int PETRiding; //0x0674
    char pad_0678[16]; //0x0678
    float TargetX; //0x0688
    float TargetY; //0x068C
    float TargetZ; //0x0690
    char pad_0694[44]; //0x0694
    float CurrentX; //0x06C0
    float CurrentY; //0x06C4
    float CurrentZ; //0x06C8
    char pad_06CC[32]; //0x06CC
    int TEST1; //0x06EC
    char pad_06F0[36]; //0x06F0
    int TEST2; //0x0714
    char pad_0718[60]; //0x0718
    int m_Effects; //0x0754f
    char pad_0758[12]; //0x0758
    char m_hwanlevel; //0x0764
    char pad_0765[39]; //0x0765
private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x78c)
        ENSURE_OFFSET(classLink, 0x274)
        ENSURE_OFFSET(decolist, 0x284)
        ENSURE_OFFSET(m_abnormalState, 0x02C4)
        ENSURE_OFFSET(m_mpCurrent, 0x0454)
        ENSURE_OFFSET(m_hpMax, 0x0458)
        ENSURE_OFFSET(m_mpMax, 0x045C)
        ENSURE_OFFSET(m_hpCurrent, 0x0460)
        ENSURE_OFFSET(m_renderY, 0x4F0)
        ENSURE_OFFSET(m_renderX, 0x4F4)
        ENSURE_OFFSET(m_renderDepth, 0x4F8)
        ENSURE_OFFSET(b_JobType, 0x50d)
      //  ENSURE_OFFSET(m_characterState, 0x65c)
    END_FIXTURE()

    RUN_FIXTURE(CICharactor)
};
