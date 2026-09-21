#pragma once
#include "IFMainFrame.h"


class CIFSocial : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFSocial)
GFX_DECLARE_MESSAGE_MAP(CIFSocial)
private:
    CIFSocial(void);
    ~CIFSocial(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick_1();

    void On_BtnClick_2();
public:

    void UpdateMenuSize();
    int SocialID;
};
