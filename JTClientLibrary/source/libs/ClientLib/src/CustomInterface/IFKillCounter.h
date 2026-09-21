#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"

class CIFKillCounter : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFKillCounter)
private:
    CIFKillCounter(void);
    ~CIFKillCounter(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(BYTE Num, const wchar_t* Charname, const wchar_t* DMG);
    void Clear();
    void UpdateMenuSize();
};
