#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"

class CIFTeamCounter : public CIFFrame
{
GFX_DECLARE_DYNCREATE(CIFTeamCounter)
private:
    CIFTeamCounter(void);
    ~CIFTeamCounter(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(BYTE Num, byte team, const wchar_t *Charname, int Kills, int BlueTeam, int RedTeam) const;
    void Clear() const;
    void UpdateMenuSize();
public:

    CIFStatic* bluekill;
    CIFStatic* redkill;

    CIFStatic* NameSlot1;
    CIFStatic* NameSlot2;
    CIFStatic* NameSlot3;
    CIFStatic* NameSlot4;
    CIFStatic* NameSlot5;

    CIFStatic* KillSlot1;
    CIFStatic* KillSlot2;
    CIFStatic* KillSlot3;
    CIFStatic* KillSlot4;
    CIFStatic* KillSlot5;

    CIFStatic* IconSlot1;
    CIFStatic* IconSlot2;
    CIFStatic* IconSlot3;
    CIFStatic* IconSlot4;
    CIFStatic* IconSlot5;


};
