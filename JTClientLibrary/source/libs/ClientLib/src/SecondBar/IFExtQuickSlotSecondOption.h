#pragma once
#include "IFMainFrame.h"
#include "IFCheckBox.h"

class CIFExtQuickSlotSecondOption : public CIFMainFrame {
GFX_DECLARE_DYNCREATE(CIFExtQuickSlotSecondOption)
GFX_DECLARE_MESSAGE_MAP(CIFExtQuickSlotSecondOption)
    enum {
        GDR_EXT_QUICKSLOT_OPT_BG1 = 5,                 //CIFNormalTile
        GDR_EXT_QUICKSLOT_OPT_BG2 = 6,                 //CIFNormalTile
        GDR_EXT_QUICKSLOT_OPT_FRAME1 = 7,              //CIFFrame
        GDR_EXT_QUICKSLOT_OPT_FRAME2 = 8,              //CIFFrame
        GDR_EXT_QUICKSLOT_OPT_FRAME2_TEXT = 9,         //CIFStatic
        GDR_EXT_QUICKSLOT_OPT_TRANS = 15,              //CIFStatic
        GDR_EXT_QUICKSLOT_OPT_LOCK = 16,               //CIFStatic
        GDR_EXT_QUICKSLOT_OPT_FIX = 17,                //CIFStatic
        GDR_EXT_QUICKSLOT_OPT_DOUBLELINE = 18,         //CIFStatic
        GDR_EXT_QUICKSLOT_OPT_TRANS_CHECKBOX = 25,     //CIFCheckBox
        GDR_EXT_QUICKSLOT_OPT_LOCK_CHECKBOX = 26,      //CIFCheckBox
        GDR_EXT_QUICKSLOT_OPT_FIX_CHECKBOX = 27,       //CIFCheckBox
        GDR_EXT_QUICKSLOT_OPT_DOUBLELINE_CHECKBOX = 28,//CIFCheckBox
        GDR_EXT_QUICKSLOT_OK_BTN = 35,                 //CIFButton
        GDR_EXT_QUICKSLOT_APPLY_BTN = 36,              //CIFButton
    };

public:
    CIFExtQuickSlotSecondOption(void);

    ~CIFExtQuickSlotSecondOption(void);

    bool OnCreate(long ln) override;

    void ResetPosition();

public:
    void On_BtnClick();
    void LoadCheckBoxState();
    void SetCheckBoxState(int Type);

public:
    bool QUICKSLOT_OPT_TRANS_SETTINGS;
    bool QUICKSLOT_OPT_TRANS_CHECKBOX;
    bool QUICKSLOT_OPT_LOCK_CHECKBOX;
    bool QUICKSLOT_OPT_FIX_CHECKBOX;
    bool QUICKSLOT_OPT_DOUBLELINE_CHECKBOX;
    bool QUICKSLOT_OPT_YATAY;
    int mainx;
    int mainy;
    int barx;
    int bary;
};