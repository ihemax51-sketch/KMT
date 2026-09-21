#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"

class CIFPartyMemberViewer : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFPartyMemberViewer)
private:
    CIFPartyMemberViewer(void);
    ~CIFPartyMemberViewer(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(BYTE Num, int Race, int CharLevel, int Mastery1, int Mastery2);
    void WriteRace(int ObjID, int SlotID);
    void WriteMastery(int mastery1, int mastery2, int slot);
    void Clear() const;
    void UpdateMenuSize();
    void OnTimer(int) override;
    void OpenDPS(int timeoutSeconds);
private:

public:
    CIFStatic* levelslot[8];
    CIFStatic* raceslot[8];
    CIFStatic* masteryslot1[8];
    CIFStatic* masteryslot2[8];
};
