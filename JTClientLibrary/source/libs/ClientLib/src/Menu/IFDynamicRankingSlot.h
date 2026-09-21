#pragma once

#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFDynamicRankingSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFDynamicRankingSlot)
public:
    CIFDynamicRankingSlot(void);
    ~CIFDynamicRankingSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void SetName(int Num, const wchar_t* Charname, const wchar_t* Guild, const wchar_t* Points);
    void Clear();
    void NewUpdate();
public:

};