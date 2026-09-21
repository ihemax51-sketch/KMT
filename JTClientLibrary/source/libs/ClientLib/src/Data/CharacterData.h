#pragma once

#include <BSLib/BSLib.h>
#include <ghidra/undefined.h>

#include "CommonData.h"

/**
 * Character data from characterdata*.txt
 * @note Field names match column names of file
 */
struct SCharacterData : public SCommonData {
    int Level; //0x01C0
    int CharGender; //0x01C4
    int MaxHP; //0x01C8
    int MaxMP; //0x01CC
    undefined pad_01D0[16]; //0x01D0
    int PD; //0x01E0
    int MD; //0x01E4
    int PAR; //0x01E8
    int MAR; //0x01EC 1f4
    int ER; //0x01F0
    int BR; //0x01F4
    int HR; //0x01F8
    int CHR; //0x01FC
    int ExpToGive; //0x0200
    int CreepType; //0x0204
    int Knockdown; //0x0208
    int KO_RecoverTime; //0x020C
    int DefaultSkill_1; //0x0210
    undefined pad_0214[24]; //0x0214

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(Level, 0x01C0)
        ENSURE_OFFSET(CharGender, 0x01C4)
        ENSURE_OFFSET(MaxHP, 0x01C8)
        ENSURE_OFFSET(MaxMP, 0x01CC)
        ENSURE_OFFSET(PD, 0x01E0)
        ENSURE_OFFSET(MD, 0x01E4)
        ENSURE_OFFSET(PAR, 0x01E8)
        ENSURE_OFFSET(MAR, 0x01EC)
        ENSURE_OFFSET(ER, 0x01F0)
        ENSURE_OFFSET(BR, 0x01F4)
        ENSURE_OFFSET(HR, 0x01F8)
        ENSURE_OFFSET(CHR, 0x01FC)
        ENSURE_OFFSET(ExpToGive, 0x0200)
        ENSURE_OFFSET(CreepType, 0x0204)
        ENSURE_OFFSET(Knockdown, 0x0208)
        ENSURE_OFFSET(KO_RecoverTime, 0x020C)
        ENSURE_OFFSET(DefaultSkill_1, 0x0210)
    END_FIXTURE()

    RUN_FIXTURE(SCharacterData)
};

class CCharacterData {

public:
    virtual ~CCharacterData();

    /// \address 00971210
    const SCharacterData &GetData() const;
    bool IsGrapPet() const;
    bool IsAttackPet() const;
private:
    //undefined pad[4];
    SCharacterData m_data;

    BEGIN_FIXTURE()
        ENSURE_OFFSET(m_data, 0x8);
    END_FIXTURE()
    RUN_FIXTURE(CCharacterData)
};
