#pragma once

#include <IFDecoratedStatic.h>
#include <IFStatic.h>

class CIFWebViewerGuide : public CIFDecoratedStatic {
    GFX_DECLARE_DYNCREATE(CIFWebViewerGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;

private:
    CIFStatic* m_label;
};
