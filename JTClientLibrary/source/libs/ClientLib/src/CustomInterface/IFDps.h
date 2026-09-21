#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"

class CIFDps : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFDps)
private:
    CIFDps(void);
    ~CIFDps(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(BYTE Num, std::n_wstring Charname, const wchar_t* DMG);
    void Clear();
    void UpdateMenuSize();
    void OnTimer(int) override;
    void OpenDPS(int timeoutSeconds);
    void SetUniqueName(std::n_wstring text);
private:


};
