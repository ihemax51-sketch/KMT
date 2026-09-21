#pragma once
#include <IFSelectableArea.h>
#include <IFScrollManager.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFIconManagerSlot.h"

class CIFIconManager : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFIconManager)
GFX_DECLARE_MESSAGE_MAP(CIFIconManager)
private:
    CIFIconManager(void);
    ~CIFIconManager(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
    void On_NextBtn();
    void On_PrevBtn();
    void ButtonDelay(int timeoutSeconds);
    void OnTimer(int) override;
public:
    void LoadItems();
    void UpdateMenuSize();
    void Clear();
    void ClearDDJ();
   // int Func_36(int a1, short action, int a3, int a4) override;
    struct IconStruct
    {
        int DBID;
        std::n_wstring MediaPath;
        byte side;
    };
    std::vector<IconStruct> IconList;
//    CIFVerticalScroll* m_scroll;

    int m_SelectedItemId;
    byte m_SelectedSide;

    int m_CurrentIndex;
    int m_MaxIndex;
void UpdateText(int Number);
void LoadPage(int pageNumber);
};
