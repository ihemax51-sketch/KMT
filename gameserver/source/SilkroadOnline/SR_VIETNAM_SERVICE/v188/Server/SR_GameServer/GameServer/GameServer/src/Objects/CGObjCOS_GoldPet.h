#pragma once
#include <memory/hook.h>
#include "CGObjCOS_Pet.h"

class CGObjCOS_GoldPet : public CGObjCOS_Pet
{
public:
    void setspeed(float p1, float p2);
    void GoldPetFunc314();
    void FUNC2155();
    void Func308(unsigned int param1);
    void FUNC362();
    void FUNCMovement(unsigned int param);
    unsigned int FunNew(unsigned int param);
    void FUN_004d2ad0();
    void FUN343(unsigned int param1, unsigned int param2);
    void FUN348(float param);
    void FUN230(unsigned int param);

public:
    char pad_1E08[8]; //0x1E08
    //BEGIN_FIXTURE()
    //ENSURE_SIZE(7392)
    //END_FIXTURE()

    //RUN_FIXTURE(CGObjCOS)
};
