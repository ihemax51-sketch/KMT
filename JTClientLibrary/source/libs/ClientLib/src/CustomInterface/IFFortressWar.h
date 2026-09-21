#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFStretchWnd.h"
#include "IFNormalTile.h"
#include "IFSelectableArea.h"
class CIFFortressWar : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFFortressWar)
GFX_DECLARE_MESSAGE_MAP(CIFFortressWar)
private:
    CIFFortressWar(void);
    ~CIFFortressWar(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteGuildKill(BYTE Num, const wchar_t* Charname, const wchar_t* DMG) const;
    void WritePlayerKill(BYTE Num, const wchar_t *Charname, const wchar_t *DMG) const;
    void ClearGuild() const;
    void ClearPlayer() const;
    void UpdateMenuSize();
private:
    void On_BtnClick();
    void On_BtnClickPlayer();
    void ActivateTabPage(BYTE page);
    int Func_4(int a2) override;
    void OnUnknownStuff();
    CIFStatic * m_PlayerNameSlots[5];
    CIFStatic* m_PlayerKillSlots[5];

    CIFStatic * m_GuildNameSlots[5];
    CIFStatic* m_GuildKillSlots[5];

    CIFStatic * m_UnionNameSlots[5];
    CIFStatic* m_UnionKillSlots[5];

    CIFSelectableArea** m_pTabs;
    static const int numberOfTabs = 3;
    static const int tabWidth = 80;
    static const int tabHeight = 24;
    static const int tabFirstId = 100;
public:


void WriteUnionKill(BYTE Num, const wchar_t* Charname, const wchar_t* DMG) const;
void ClearUnion() const;
};
