#pragma once
#include "IFDecoratedStatic.h"


class CIFChestGuide : public CIFDecoratedStatic
{
GFX_DECLARE_DYNCREATE(CIFChestGuide)

    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
    void OnUpdate() override;
public:
    CIFStatic * namelabel;
};
