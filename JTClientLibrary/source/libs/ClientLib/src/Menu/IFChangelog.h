#pragma once
#include <IFVerticalScroll.h>
#include "IFMainFrame.h"


class CIFChangelog : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFChangelog)
GFX_DECLARE_MESSAGE_MAP(CIFChangelog)
private:
    int Func_4(int a2) override;
    int Func_36(int a1, short action, int a3, int a4) override;
    CIFChangelog(void);
    ~CIFChangelog(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void UpdateMenuSize();
    CIFVerticalScroll* m_Scroll;
};
