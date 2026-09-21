#pragma once

#include <IFStatic.h>
#include <IFButton.h>
#include "IFTextBox.h"
#include "IFWnd.h"
#include <BSLib/Debug.h>
#include <IFEdit.h>
class CIFPopupList2 : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFPopupList2)
GFX_DECLARE_MESSAGE_MAP(CIFPopupList2)

public:
    CIFPopupList2(void);
    ~CIFPopupList2(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
private:
    void OnSelect(int ,int);
    void OnShowList();
private:


public:
    CIFStatic* m_bg;
    CIFEdit*  m_text;
    CIFTextBox* m_list;
    CIFStatic* m_listbg;
    CIFButton* m_btn;
};

