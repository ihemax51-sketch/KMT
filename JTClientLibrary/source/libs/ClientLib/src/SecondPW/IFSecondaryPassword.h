#pragma once
#include <IFTextBox.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFNormalTile.h"

class CIFSecondaryPassword : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFSecondaryPassword)
GFX_DECLARE_MESSAGE_MAP(CIFSecondaryPassword)
private:
    CIFSecondaryPassword(void);
    ~CIFSecondaryPassword(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
    void Number1();
    void Number2();
    void Number3();
    void Number4();
    void Number5();
    void Number6();
    void Number7();
    void Number8();
    void Number9();
    void Number0();

    void NumberRemove();
    void WriteNumber(std::n_wstring number);
    CIFEdit *GetActivePasswordEdit(int *selectionSource);
    CIFEdit *GetCurrentlyFocusedPasswordEdit();
    bool IsValidPasswordEdit(CIFEdit *edit);
    void RememberFocusedPasswordEdit();
    void InitializeActivePasswordEdit();
    void Confirm();
    void RemovePw();
public:
    void NumberClean();
    void ShowGWnd(bool visible) override;
    void UpdateMenuSize();
    void Clear();
    void SetMode(byte a1);
    undefined1 OnCloseWnd() override;
    byte Mode;
    void OnClickCancelBtn();
    void CreatePassword();
    void ClearAll();
    void SetRememberPC(bool a);
void ShuffleButtons();
    CIFTextBox * DescTextBox;
    CIFEdit* myFirstPasswordBox;
    CIFEdit* mySecondPasswordBox;
    CIFEdit* myThirdPasswordBox;
    CIFButton* myCancelButton;
    CIFButton* myChangeButton;
    std::n_wstring Pass1;
    std::n_wstring Pass2;
    std::n_wstring Pass3;

private:
    CIFEdit *m_pLastActivePasswordEdit;
};
