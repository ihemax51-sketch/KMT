#pragma once
#include <IFBarWnd.h>
#include <IFTextBox.h>
#include "IFMainFrame.h"

class CIFEventRegisterSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFEventRegisterSlot)
GFX_DECLARE_MESSAGE_MAP(CIFEventRegisterSlot)

private:
    CIFEventRegisterSlot(void);
    ~CIFEventRegisterSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(byte No, const wchar_t *EventName, const wchar_t *EventDesc);
    int OnMouseLeftUp(int a1, int x, int y) override;
    void ClearDDJ();
public:
    int number;
    std::n_wstring testbox;
};

