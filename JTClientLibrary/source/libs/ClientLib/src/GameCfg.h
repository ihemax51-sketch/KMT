#pragma once

#include <string>

#include "support/AbiTest.h"

#include "BSLib/BSLib.h"

#include "LocalTime.h"

class GameCfg {
private:
    char pad_0000[0x004C]; //0x0000
public:
    std::string str_gateway_server; //0x004C
    std::string m_strGameType; //0x0068
private:
    char pad_0084[4]; //0x0084
public:
    std::string m_strRegKey; //0x0088
    std::string str_textdata_basepath; //0x00A4
private:
    char pad_00C0[28]; //0x00C0
public:
    short m_RelatedToDrvUpdate; //0x00DC checked to be >= 2004
private:
    char pad_00DE[14]; //0x00DE
public:
    char b_RelatedToDrvUpdate; //0x00EC
private:
    char pad_00ED[1]; //0x00ED
public:
    short gateport; //0x00EE
    int m_version; //0x00F0
private:
    char pad_00F4[4]; //0x00F4
public:
    CLocalTime m_LocalTime; //0x00F8 Missing Members in local time
private:
    char pad_00F9[0x0118 - 0x00F9]; //0x00F9
public:
    int m_contentId; //0x0118
    int m_divisionId; //0x011C
    int m_gatewayId; //0x0120
    CGfxRuntimeClass* m_runtimeClass; //0x0124
    char N0000093A; //0x0128
private:
    char pad_0129[0x012F - 0x0129]; //0x0129
public:
    bool m_bDebugMode; //0x012F
private:
    char pad_0130[0x0160 - 0x0130]; //0x0130
public:
    int langId; //0x0160
private:
    char pad_0164[28]; //0x0164
public:
    std::string str_emblem_server; //0x0180
    std::string str_emblem_folder; //0x019C
    std::string str_intro_scriptname; //0x01B8
    std::string str_intro_soundtrack; //0x01D4
public:
private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(m_LocalTime, 0xf8)
        ENSURE_OFFSET(m_bDebugMode, 0x012F)
    END_FIXTURE()

    RUN_FIXTURE(GameCfg)
};
