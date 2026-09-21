#pragma once
#include <IFScrollManager.h>
#include "IFDecoratedStatic.h"
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFEventRegisterSlot.h"


class CIFEventRegister : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFEventRegister)
GFX_DECLARE_MESSAGE_MAP(CIFEventRegister)
private:
    CIFEventRegister(void);
    ~CIFEventRegister(void);
    int Func_4(int a2) override;
    int Func_36(int a1, short action, int a3, int a4) override;

    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnTimer(int) override;
    void OpenRegButton(int timeoutSeconds);
    void OpenCancelButton(int timeoutSeconds);
    void On_BtnClickReg();
    void On_BtnClickCancel();
public:
    void UpdateMenuSize();
    void Clear();
    void UpdateEvents();
    void ClearDDJ();
    void SetDescBoxText(std::n_wstring string);
    CIFEventRegisterSlot* m_slots[50];
    CIFScrollManager * m_scroll;
    CIFTextBox* descbox;
    CIFVerticalScroll* vscroll;
    struct Events
    {
        byte DBID;
        std::wstring EventName;
        std::wstring EventDesc;

    };
    std::vector<Events> EventList;
    int SelectedEventID;

};

class CIFEventRegisterGuide : public CIFDecoratedStatic
{
    GFX_DECLARE_DYNCREATE(CIFEventRegisterGuide)

public:
    bool OnCreate(long ln) override;
    int OnMouseLeftUp(int a1, int x, int y) override;
    void OnCIFReady() override;
};
