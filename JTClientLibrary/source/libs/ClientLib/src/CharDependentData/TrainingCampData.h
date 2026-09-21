#pragma once

#include "support/AbiTest.h"

struct STrainingCampData {
private:
    char pad_0000[32]; //0x0004
public:
    bool bIsAcademyRegistered; //0x0020
private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(bIsAcademyRegistered, 0x0020)
    END_FIXTURE()
    
    RUN_FIXTURE(STrainingCampData)
};