#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFTextBox.h"
#include "IFVerticalScroll.h"
#include "IFEdit.h"


class CIFSavedLocation : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFSavedLocation)
GFX_DECLARE_MESSAGE_MAP(CIFSavedLocation)
public:
    CIFSavedLocation(void);
    ~CIFSavedLocation(void);
    void SaveButton1Click();
    void SaveButton2Click();
    void SaveButton3Click();
    void SaveButton4Click();
    void Teleport1Loc();
    void Send181C(byte telslot, byte itemslot);
    void Teleport2Loc();
    void Teleport3Loc();
    void Teleport4Loc();
    void Remove1Loc();
    void Remove2Loc();
    void Remove3Loc();
    void Remove4Loc();

    void CloseWnd();
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void UpdateMenuSize();
    undefined1 OnCloseWnd() override;
    void SetButtonText(byte LocationID, int RegionID);
    void LoadLocations();

    struct SavedLocation_Data
    {
        byte LocationID;
        int RegionID;
        int WorldID;
    };
    std::map<byte, SavedLocation_Data> m_SavedLocation;

public:
    byte SavedLocation_1;
    byte SavedLocation_2;
    byte SavedLocation_3;
    byte SavedLocation_4;

};



