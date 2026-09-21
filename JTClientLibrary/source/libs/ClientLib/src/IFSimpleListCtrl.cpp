#include <CustomData/CustomSettingManager.h>
#include "IFSimpleListCtrl.h"


bool CIFSimpleListCtrl::OnCreateIMPL(long ln) {
    if(m_Settings->NewIDPWScreen)
    {
        if (this->UniqueID() == 10000) {
            wnd_pos servernamelist = this->GetPos();
            wnd_size servernamelistsize = this->GetSize();
            this->MoveGWnd(servernamelist.x + 22, servernamelist.y);
            this->SetGWndSize(servernamelistsize.width - 22, servernamelistsize.height - 14);
        }
        if (this->UniqueID() == 10001) {
            wnd_pos servernamelist = this->GetPos();
            wnd_size servernamelistsize = this->GetSize();
            this->MoveGWnd(servernamelist.x + 38, servernamelist.y);
        }
        if (this->UniqueID() == 10002) {
            wnd_pos servernamelist = this->GetPos();
            wnd_size servernamelistsize = this->GetSize();
            this->MoveGWnd(servernamelist.x + 58, servernamelist.y);
        }
    }

    return 1;
}