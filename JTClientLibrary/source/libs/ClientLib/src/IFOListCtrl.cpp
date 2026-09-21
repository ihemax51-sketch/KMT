#include "IFOListCtrl.h"
#include "IFSimpleListCtrl.h"


void CIFOListCtrl::GetResObj(){

    if(this->m_IRM.GetResObj<CIFSimpleListCtrl>(10000, 1) != NULL && this->m_IRM.GetResObj<CIFSimpleListCtrl>(10001, 1) != NULL
       && this->m_IRM.GetResObj<CIFSimpleListCtrl>(10002, 1) != NULL)
    {
        wnd_pos servernamelist = this->m_IRM.GetResObj<CIFSimpleListCtrl>(10000, 1)->GetPos();
        wnd_size servernamelistsize = this->m_IRM.GetResObj<CIFSimpleListCtrl>(10000, 1)->GetSize();
        this->m_IRM.GetResObj<CIFSimpleListCtrl>(10000, 1)->MoveGWnd(servernamelist.x + 22, servernamelist.y);
        this->m_IRM.GetResObj<CIFSimpleListCtrl>(10000, 1)->SetGWndSize(servernamelistsize.width - 22, servernamelistsize.height - 14);


        wnd_pos servernamelistx = this->m_IRM.GetResObj<CIFSimpleListCtrl>(10001, 1)->GetPos();
        wnd_size servernamelistsizex = this->m_IRM.GetResObj<CIFSimpleListCtrl>(10001, 1)->GetSize();
        this->m_IRM.GetResObj<CIFSimpleListCtrl>(10001, 1)->MoveGWnd(servernamelistx.x + 38, servernamelistx.y);


        wnd_pos servernamelistxx = this->m_IRM.GetResObj<CIFSimpleListCtrl>(10002, 1)->GetPos();
        wnd_size servernamelistsizexx = this->GetSize();
        this->m_IRM.GetResObj<CIFSimpleListCtrl>(10002, 1)->MoveGWnd(servernamelistxx.x + 58, servernamelistxx.y);

    }

}
