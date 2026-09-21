#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>
#include "IFSlotWithHelp.h"
class CIFDailyLoginRewardSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFDailyLoginRewardSlot)
GFX_DECLARE_MESSAGE_MAP(CIFDailyLoginRewardSlot)
public:
    CIFDailyLoginRewardSlot(void);
    ~CIFDailyLoginRewardSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;

public:
    CIFSlotWithHelp* m_itemInfo;
    void FillTheSlot(int itemID, int itemCount, int OptLevel, int day, int RefRewardID);
    void ClearSlot();
    void OpenReceiveButton(bool value);
    int RefRewardID;
    int DItemID;
    std::n_wstring ItemCodeName;
    void Receive();
};