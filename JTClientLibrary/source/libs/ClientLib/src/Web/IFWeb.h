#pragma once
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "SilkroadWeb.h"

class CIFWeb : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFWeb)
GFX_DECLARE_MESSAGE_MAP(CIFWeb)
private:
    CIFWeb(void);
    ~CIFWeb(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    SilkroadWeb * m_Web;
    void UpdateMenuSize();
    undefined OnCloseWnd() override;
void Charge();
};
