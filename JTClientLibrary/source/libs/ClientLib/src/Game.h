#pragma once

#include <string>

#include "GFXMainFrame/GFXMainFrame.h"

#include <GFX3DFunction/GFontTexture.h>
#include <GFX3DFunction/RTLoading.h>

#include <GFXFM/IFileManager.h>

#include <BSLib/StringCheck.h>

#include "unsorted.h"
#include "GEffSoundBody.h"
#include "DxSound.h"
#include "IFConsole.h"

struct ClientResolutonData
{
    char pad_0000[36]; //0x0000
    int width; //0x0024
    int height; //0x0028
}; //Size: 0x002C

struct ClientRes
{
    char index;
    ClientResolutonData res[1]; //0x0004
}; //Size: 0x0030

struct WhatIsThis {
    undefined4 field_0;
    char field_4;
    undefined4 field_8;
    undefined1 field_c;
    undefined1 field_10;
    undefined1 field_11;
    char pad_0012[3];
    undefined1 field_15;
    char pad_0016[1];
    undefined1 field_17;
};

class CGame : public CGFXMainFrame {
public:
    int N0000051E; //0x0490
    CGFontTexture* font; //0x0494
    char pad_0498[40]; //0x0498
    CDxSound* p_CDXSound; //0x04C0
    IFileManager* pFM_Media; //0x04C4
    IFileManager* pFM_Map; //0x04C8
    IFileManager* pFM_Data; //0x04CC
    IFileManager* pFM_AdditionalData; //0x04D0
    IFileManager* pFM_Music; //0x04D4
    IFileManager* pFM_Particles; //0x04D8
    IFileManager* m_RTLoading_FMs[5]; //0x04DC
    CGEffSoundBody* m_CGEffSoundBody; //0x04F0 4F0
    CIFConsole* m_Console; //0x04F4
    char pad_04F8[104]; //0x04F8
    int N00000884; //0x0560
    char pad_0564[8]; //0x0564
    int N00000887; //0x056C
    ClientRes N00000888; //0x0570
    char pad_05A0[44]; //0x05A0
    CRTLoading m_RTLoading; //0x05CC 5CC
    char pad_062C[0x06F8 - 0x062C]; //0x062C
    GameCfg m_GCfg; //0x06F8
    CStringCheck * m_badWordFilter; //0x08E8
    char pad_08EC[0x960 - 0x8EC]; //0x08EC
public:
    /// \address 004F9CD0
    static const ClientResolutonData &GetClientDimensionStuff();

    static CStringCheck *GetBadwordFilter();

    /// \address 004f9d00
    static WhatIsThis& STA_FUN_004f9d00();

    /// \address 00835240
    const ClientRes&GetRes() const;

    /// \address 004f9d60
    /// \return m_Console
    static CIFConsole *GetCheatConsole();

public:
    void SetWindowPos();

    int sub_83580(int arg);

    void LoadAndInitStuffIMPL(int a1);

    void InitGameAssets();

    void LoadTextfiles();

    void ResizeMainWindow();

    void ClientReadConfiguration();

    void LoadPK2s();

    void LoadFileManager();

    IFileManager* OpenPackFile(std::string pakfile, std::string passwd, int mode);

    void SplitCommandline();
    
    void InitializeAfterTitle();

    void sub_8371D0();

    void sub_BA3FC0(int i, int i1);

    void *GetFont(int a2);

private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x0960)
        ENSURE_OFFSET(m_GCfg, 0x06F8)
        ENSURE_OFFSET(m_badWordFilter, 0x08E8)
    END_FIXTURE()

    RUN_FIXTURE(CGame)
};

#define g_CGame ((CGame*)0x00EECE80)

// I am so sorry :cry:
#define theApp (*g_CGame)

GameCfg *Fun_GetCfgGame();
