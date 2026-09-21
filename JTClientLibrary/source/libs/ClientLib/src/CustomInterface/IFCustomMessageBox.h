#pragma once
#include "IFMainFrame.h"
#include "IFTextBox.h"

class CIFCustomMessageBox : public CIFMainFrame {
GFX_DECLARE_DYNCREATE(CIFCustomMessageBox)
GFX_DECLARE_MESSAGE_MAP(CIFCustomMessageBox)

private:
    CIFCustomMessageBox(void);
    ~CIFCustomMessageBox(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick_1();


public:
    void UpdateMenuSize();
    CIFButton *ok;
    CIFButton *cancel;
    CIFTextBox *write;
    int selectedCheckBoxID;
    int selectedMasteryID;
    void On_BtnClick_2();
};
