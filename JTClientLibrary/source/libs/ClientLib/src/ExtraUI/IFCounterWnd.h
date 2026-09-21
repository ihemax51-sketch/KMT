#pragma once
#include "IFMainFrame.h"
#include "IFEdit.h"

class CIFCounterWnd : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFCounterWnd)
GFX_DECLARE_MESSAGE_MAP(CIFCounterWnd)
private:
    CIFCounterWnd(void);
    ~CIFCounterWnd(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
public:
    void UpdateMenuSize();
    void Clear();
    CIFStatic* m_CountLeft;
    CIFStatic* m_CountRight;
    void UpdateCounter(int Left, int Right);
};
