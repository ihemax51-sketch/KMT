#pragma once

#include "IFMainFrame.h"
#include "IFInventory.h"
#include "IFEquipment.h"
#include "IFSkill.h"
#include "IFAction.h"
#include "IFParty.h"
#include "IFPlayerInfo.h"
#include "IFQuest.h"
#include "IFApprenticeShip.h"
#include "IFButton.h"
#include "IFStatic.h"

class CIFMainPopup : public CIFMainFrame {
    GFX_DECLARE_DYNAMIC_EXISTING(CIFMainPopup, 0x00eea6dc)
    GFX_DECLARE_MESSAGE_MAP(CIFMainPopup)
public:
    GFX_DECLARE_DYNCREATE_FN(CIFMainPopup)

public:
    CIFMainPopup();

public:
    /// \address 006a2560
    bool OnCreate(long ln) override;

    void ActiveAutoMastery();
    /// \address 006a1c50
    void OnUpdate() override;

    /// \address 006a1c60
    void RenderMyself() override;

    /// \address 006a2390
    void ShowGWnd(bool bVisible) override;

public:
    /// \address 006a1d20
    CIFInventory *GetInventory() const;

    /// \address 006a1d80
    CIFSkill *GetSkill() const;

    /// \address 006a1db0
    CIFAction *GetAction() const;

    /// \address 006a1de0
    CIFParty *GetParty() const;

    /// \address 06a1e10
    CIFPlayerInfo *GetPlayerInfo() const;

    /// \address 006a1e40
    CIFQuest *GetQuest() const;

    /// \address 006a1e70
    CIFApprenticeShip *GetApprenticeShip() const;

    /// \address 006a1d50
    CIFEquipment *GetEquipment() const;

    /// \address 006a1cf0
    bool IsSubPageActive(int nPageId);

    /// \address 006a1c70
    void HideAll();

    /// \address 006a1f70
    void ShowSubPage(int nPageId);

private:

    /// \address 006a24f0
    void OnClick_BtnChar();

    /// \address 006a2500
    void OnClick_BtnInv();

    /// \address 006a2510
    void OnClick_BtnSkill();

    /// \address 006a2520
    void OnClick_BtnAct();

    /// \address 006a2530
    void OnClick_BtnParty();

    /// \address 006a2540
    void OnClick_BtnQuest();

    /// \address 006a2550
    void OnClick_BtnApprentice();

private:

    void ApplyInventoryPresentation(bool inventoryPage);
    void ShowInventoryDecorations(bool visible);

    enum eMainPopWndType : int {
        DUMMY = 0,
        MAINPOP_PLAYER_INFO = 0,
        MAINPOP_INVENTORY,
        MAINPOP_SKILL,
        MAINPOP_ACTION,
        MAINPOP_PARTY,
        MAINPOP_QUEST,
        MAINPOP_ACADEMY
    };

private:
    CIFInventory *m_pInventoryWindow; //0x07BC
    CIFEquipment *m_pEquipmentWindow; //0x07C0
    CIFSkill *m_pSkillWindow; //0x07C4
    CIFAction *m_pActionWindow; //0x07C8
    CIFParty *m_pPartyWindow; //0x07CC
    CIFPlayerInfo *m_pPlayerInfoWindow; //0x07D0
    CIFQuest *m_pQuestWindow; //0x07D4
    CIFApprenticeShip *m_pApprenticeShipWindow; //0x07D8
    CIFButton *m_pBtnInventory; //0x07DC
    CIFButton *m_pBtnSkill; //0x07E0
    CIFButton *m_pBtnAction; //0x07E4
    CIFButton *m_pBtnParty; //0x07E8
    CIFButton *m_pBtnCharacter; //0x07EC
    CIFButton *m_pBtnQuest; //0x07F0
    CIFButton *m_pBtnApprentice; //0x07F4
    CIFStatic *m_pBackground; //0x07F8
    eMainPopWndType m_nWindowType; //0x07FC
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x800)
        ENSURE_OFFSET(m_pInventoryWindow, 0x07BC)
        ENSURE_OFFSET(m_pEquipmentWindow, 0x07C0)
        ENSURE_OFFSET(m_pSkillWindow, 0x07C4)
        ENSURE_OFFSET(m_pActionWindow, 0x07C8)
        ENSURE_OFFSET(m_pPartyWindow, 0x07CC)
        ENSURE_OFFSET(m_pPlayerInfoWindow, 0x07D0)
        ENSURE_OFFSET(m_pQuestWindow, 0x07D4)
        ENSURE_OFFSET(m_pApprenticeShipWindow, 0x07D8)
        ENSURE_OFFSET(m_pBtnInventory, 0x07DC)
        ENSURE_OFFSET(m_pBtnSkill, 0x07E0)
        ENSURE_OFFSET(m_pBtnAction, 0x07E4)
        ENSURE_OFFSET(m_pBtnParty, 0x07E8)
        ENSURE_OFFSET(m_pBtnCharacter, 0x07EC)
        ENSURE_OFFSET(m_pBtnQuest, 0x07F0)
        ENSURE_OFFSET(m_pBtnApprentice, 0x07F4)
        ENSURE_OFFSET(m_pBackground, 0x07F8)
        ENSURE_OFFSET(m_nWindowType, 0x07FC)
    END_FIXTURE()

    RUN_FIXTURE(CIFMainPopup)
};
