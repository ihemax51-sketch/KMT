#pragma once
#include <IFBarWnd.h>
#include <IFTextBox.h>
#include "IFMainFrame.h"

class CIFAchievementsSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFAchievementsSlot)
GFX_DECLARE_MESSAGE_MAP(CIFAchievementsSlot)

private:
    CIFAchievementsSlot(void);
    ~CIFAchievementsSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void WriteLine(int DatabaseID, byte Category, std::wstring Name);
    int OnMouseLeftUp(int a1, int x, int y) override;
    void ClearDDJ();
public:
    int DatabaseID;
    std::n_wstring Description;
void UpdateDefaultIcons(byte CateID, int AchievementID);
    void UpdateDefaultDescs(int AchievementID);
    void UpdateProgress(int AchievementID);
    void UpdateReward(int AchievementID);
};

