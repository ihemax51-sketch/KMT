#pragma once

#include <memory/MemoryUtility.h>
#include <SettingMgr/NewSettings.h>

class CInstanceGuild {
public:
    int GetID() const;
    int GetAllianceID() const;
public:
    char pad_0000[8]; //0x0000
    int unk; //0x0008
    char pad_000C[12]; //0x000C
    int GuildID; //0x0018
    char pad_001C[4]; //0x001C
    char MyGuildName[4]; //0x0020
    char pad_0024[20]; //0x0024
    uint8_t GuildLevel; //0x0038
    uint8_t N0000122E; //0x0039
    uint8_t N00001232; //0x003A
    uint8_t N0000122F; //0x003B
    //int GP; //0x003C
    //char pad_0040[96]; //0x0040
};
