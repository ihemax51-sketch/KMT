#pragma once
#include "IFDecoratedStatic.h"


class CIFMacroGuide : public CIFDecoratedStatic
{
GFX_DECLARE_DYNCREATE(CIFMacroGuide)

    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
    void OnUpdate() override;
public:
    CIFStatic * namelabel;
};
