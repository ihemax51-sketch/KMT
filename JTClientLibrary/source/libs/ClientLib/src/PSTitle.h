#pragma once
#include "PSOuterInterface.h"
#include "IFStatic.h"
#include "IFListCtrl.h"

class CIFLoginRegisterButton;

// Verify and apply the configured race-specific totals before the gameplay
// interface reads the native mastery selection blocks.
void KmtApplyConfiguredMasteryLimits();

class CPSTitle : public CPSOuterInterface
{
GFX_DECLARE_DYNAMIC_EXISTING(CPSTitle, 0x00eedbc4)

    GFX_DECLARE_DYNCREATE_FN(CPSTitle)

GFX_DECLARE_MESSAGE_MAP(CPSTitle)

public:
    CPSTitle();
    ~CPSTitle();
    enum {
        GDR_LOGO = 8,
        GDR_IDPWFRAME = 9,
        GDR_LIST = 10,
        GDR_CONNECTBTN = 11,
        GDR_EXIT = 12,
        GDR_LISTBTN = 44,
        GDR_ID = 41,
        GDR_PW = 42,
        GDR_SWNAME = 43,
        GDR_101ID = 101,
        GDR_102PW = 102,
        GDR_103SW = 103,
    };
    bool OnServerPacketRecv(class CMsgStreamBuffer* MsgBuffer);
    bool OnCreateIMPL(long ln);
    void OnUpdateIMPL();
    void OnClick_56Select(undefined4 param_1, undefined4 param_2);
    void Tsss();
    void OnClickScreen(undefined4 param_1, undefined4 param_2);
    void OnClickListButton(undefined4 param_1, undefined4 param_2);
    void OnClickCancel(undefined4 param_1, undefined4 param_2);
    void OnDisappear(undefined4 p1);
    void OnClickList(undefined4 param_1, undefined4 param_2);

    void PatchAlchemyPerm();

    void PressConnectButton();
    void PressButtonServerList();
    void ShowLoginRegisterWindow();
    CIFLoginRegisterButton* EnsureLoginRegisterButton();
    CIFLoginRegisterButton* GetLoginRegisterButton();
    void SyncLoginRegisterButton();
    void SetLoginRegisterButtonVisible(bool visible);
    void ReplayCachedShardList();
    void ApplyCachedShardStatusTexture();
    std::n_string GetLoginEditText(int id);
    bool GetLoginFrameRect(int& x, int& y, int& width, int& height);
};


