#pragma once
#include "IFMainFrame.h"
#include "IFEdit.h"

class CIFGrantName : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFGrantName)
GFX_DECLARE_MESSAGE_MAP(CIFGrantName)
private:
    CIFGrantName(void);
    ~CIFGrantName(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
public:
    void UpdateMenuSize();
    void Clear();
void OnTimer(int timerId) override;

    void SetBattleRoyaleRegionStartTime();
    void SetBattleRoyaleRegionKillTime();
    void SetBattleRoyaleStage_1_Time(int timeoutSeconds);
    void SetBattleRoyaleStage_2_Time(int timeoutSeconds);
    void SetBattleRoyaleStage_3_Time(int timeoutSeconds);
    void SetBattleRoyaleStage_4_Time(int timeoutSeconds);
    void SetBattleRoyaleStage_5_Time(int timeoutSeconds);

    int m_battleroyale_stage1_SecondsLeft;
    int m_battleroyale_stage2_SecondsLeft;
    int m_battleroyale_stage3_SecondsLeft;
    int m_battleroyale_stage4_SecondsLeft;
    int m_battleroyale_stage5_SecondsLeft;
};
