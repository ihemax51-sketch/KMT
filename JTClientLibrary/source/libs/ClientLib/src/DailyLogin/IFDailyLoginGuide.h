#pragma once
#include "IFDecoratedStatic.h"
#define GUIDE_DAILY_LOGIN 133810

class CIFDailyLoginGuide : public CIFDecoratedStatic
{
GFX_DECLARE_DYNCREATE(CIFDailyLoginGuide)

    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
    void OnUpdate() override;
public:
    CIFStatic * namelabel;
};
