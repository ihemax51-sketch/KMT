#pragma once

#include <SRIFLib/NIFWnd.h>
class CNIFAlchemySubWndType1  /* CNIFAlchemySubWnd */ : public CNIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CNIFAlchemySubWndType1, 0x00ee8508)
public:
    bool OnCreateIMPL(long ln);
    bool OnReleaseIMPL(long ln);

undefined4 OnCloseWndIMPL();
void OnUpdateIMPL();
};

