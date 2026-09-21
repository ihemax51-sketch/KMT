#pragma once


#include "IFWnd.h"
#include "IFCOSStatus.h"
class CIFCOSManager : public CIFWnd
{
public:
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
    char pad_036C[4]; //0x036C
    std::list<CIFCOSStatus*> N00000A57; //0x0370
    char pad_037C[8]; //0x037C
    int PetSizemaybe; //0x0384
    int UnknownintEverySpawnAdded2; //0x0388
BEGIN_FIXTURE()
        ENSURE_SIZE( 0x38c)
    END_FIXTURE()

    RUN_FIXTURE(CIFCOSManager)


};
