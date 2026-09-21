#pragma once
#include <IFSelectableArea.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFAchievementsSlot.h"
#include <IFBarWnd.h>
#include <IFScrollManager.h>
#include <IFTextBox.h>
#include <vector>
class CIFAchievements : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFAchievements)
GFX_DECLARE_MESSAGE_MAP(CIFAchievements)
private:
    CIFAchievements(void);
    ~CIFAchievements(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
    void OnUnknownStuff();
    int Func_4(int a2) override;
    int Func_36(int a1, short action, int a3, int a4) override;

    CIFSelectableArea *m_pTabs[2];
    static const int numberOfTabs = 2;
    static const int tabWidth = 110;
    static const int tabHeight = 24;
    static const int tabMarginLeft = 18;
    static const int tabFirstId = 100;
    std::vector<CIFAchievementsSlot*> m_slots;
    CIFScrollManager* m_scroll;
    CIFVerticalScroll* vscroll;
    CIFTextBox* descbox;
    void On_BtnClickAll();
    void On_BtnClickGeneral();
    void On_BtnClickQuest();
    void On_BtnClickUnique();
    void On_BtnClickMatch();
    void On_BtnClickEvent();
    void On_BtnClickRemove();
    void OnTimer(int) override;
    void RemoveButtonDelay(int timeoutSeconds);
    void UseButtonDelay(int timeoutSeconds);
    bool EnsureSlotCount(size_t required);
    void PopulateList(int category);
    void HighlightCategory(int category);
public:
    void UpdateMenuSize();
    void Clear();
    void ClearDDJ();
    void ActivateTabPage(BYTE page);
    void SetDescBoxText(std::n_wstring string);
    void SetUseButtonState(bool s);

    int SelectedItemID;
    byte ActiveTabNumber;
    int LastActiveCategory;


void OnListUpdated();
};
