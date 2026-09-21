#pragma once
#include <IFCheckBox.h>
#include <IFTextBox.h>
#include "IFMainFrame.h"

class CIFVSelectMall : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFVSelectMall)
GFX_DECLARE_MESSAGE_MAP(CIFVSelectMall)
private:
    CIFVSelectMall(void);
    ~CIFVSelectMall(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void AvatarMallBtn();
    void ItemMallBtn();
    void OpenMall();
    void Cancel();
public:
    void UpdateMenuSize();
    void ItemMallCheckBox();
    void AvatarMallCheckBox();
};
