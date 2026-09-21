#pragma once

#include "IFDecoratedStatic.h"

class CIFMenuGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFMenuGuide)

public:
    bool OnCreate(long ln) override;

    int OnMouseLeftUp(int a1, int x, int y) override;

    void OnCIFReady() override;

    void OnUpdate() override;
public:
    CIFStatic* namelabel;
    void OnTimer(int) override;
    void GiveStr(int timeoutSeconds);
    void GiveInt(int timeoutSeconds);
};
