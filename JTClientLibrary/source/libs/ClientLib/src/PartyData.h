#pragma once

struct SPartyData
{
private:
    char pad_0000[13]; //0x0000
public:
    bool bIsPartyMaster;
    bool bInParty; //0x000E
private:
    char pad_000f[1]; //0x000F
public:
    int NumberOfMembers; //0x0010
private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(bInParty, 0x000E)
        ENSURE_OFFSET(NumberOfMembers, 0x0010)
    END_FIXTURE()

    RUN_FIXTURE(SPartyData)
};
