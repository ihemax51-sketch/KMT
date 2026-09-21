#pragma once
#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFEventScheduleSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFEventScheduleSlot)
public:
    CIFEventScheduleSlot(void);
    ~CIFEventScheduleSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void SetName(int Num, const wchar_t* name, long time);
public:
    long times;
void Clear();
};