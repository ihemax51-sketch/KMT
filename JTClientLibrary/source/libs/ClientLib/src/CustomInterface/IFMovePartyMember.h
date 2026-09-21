#pragma once
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFMovePartyMemberSlot.h"

class CIFMovePartyMember : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFMovePartyMember)
GFX_DECLARE_MESSAGE_MAP(CIFMovePartyMember)
private:
    CIFMovePartyMember(void);
    ~CIFMovePartyMember(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();

public:
    undefined1 OnCloseWnd() override;
    void UpdateMenuSize();
    void Clear();
    CIFMovePartyMemberSlot* pSlot[7];
    int replace;
};