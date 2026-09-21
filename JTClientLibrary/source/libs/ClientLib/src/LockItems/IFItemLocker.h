#pragma once
#include <ctime>
#include "IFItemLockSlot.h"
#include "IFMainFrame.h"
#include "IFNormalTile.h"
#include "IFEdit.h"

class CIFItemLocker : public CIFMainFrame {
GFX_DECLARE_DYNCREATE(CIFItemLocker)
GFX_DECLARE_MESSAGE_MAP(CIFItemLocker)

private:
    CIFItemLocker(void);
    ~CIFItemLocker(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick_1();
    void On_BtnClick_2();
public:
    void SetItemInfo(int ItemSlotIDs, unsigned short ItemTypeIDs);
    void UpdateMenuSize();
    void Clear();
    int ItemSlotID;
    unsigned short ItemTypeID;
    CIFEdit* Pass1;
    CIFEdit* Pass2;
    CIFButton* ok;
    CIFButton* cancel;
    CIFStatic* write;
    CIFItemLockSlot* ItemSlot;
    undefined1 OnCloseWnd() override;
    void OnBtnCert();
void StartRemainingTime();
    void OnTimer(int timerId) override;
    std::time_t endTime;
void EnableStateLockButton();
};
