#pragma once
#include "IFMainFrame.h"

class CIFStatic;

class CIFMenu : public CIFMainFrame
{
	GFX_DECLARE_DYNCREATE(CIFMenu)
	GFX_DECLARE_MESSAGE_MAP(CIFMenu)

public:
    CIFMenu(void);
	~CIFMenu(void);

	bool OnCreate(long ln) override;
	void OnUpdate() override;
	void RenderMyself() override;
	void UpdateMenuSize();
    void SetCharFace();
private:
    void CreateProfileControls();

	int OnCreatedInstance(UINT a1, UINT a2);
	int On4002(int a1, int a2);
	int On4003(int a1, int a2);
	int OnVisibleStateChange(int a1, int a2);
	int On4005(int a1, int a2);
	int OnWindowPosChanged(UINT a1, UINT a2);

private:
    void On_BtnClickGrantName();
    void On_BtnClickTitleManager();
    void On_BtnClickIconManager();

void On_BtnClickRank();
    void On_BtnClickUniqueLog();
void On_BtnClickEventRegister();
void On_BtnClickEventTimer();
void On_BtnClickAchievements();
void On_BtnClickChangelog();
void On_BtnClickAlchemyMacro();

    int GetDis(std::n_wstring str, int index);

    int GetTextWidth(std::n_wstring str);

    void HandelEmoji(int x, int y, byte type);
public:
    bool CanSendPing;
    CIFWnd *GetMenuResource(int id);

/*
    int PingRegionID;
    int PingPosX;
    int PingPosY;
    int PingPosZ;
    int Seconds;
*/

    int PingedRegionID;
    int PingedPosX;
    int PingedPosY;
    int PingedPosZ;
    int MapIndex;
    int MapType;
void On_BtnSettings();

    CIFStatic* m_profileFace;
    CIFStatic* m_profileRace;
    CIFStatic* m_profileName;
    CIFStatic* m_profileGuild;
    bool m_originalMenuBuilt;
    bool m_maxiSettingsApplied;
    bool m_casyMenuBuilt;
};
