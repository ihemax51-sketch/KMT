#include "IFMSFPS.h"
#include "IFSettings.h"
#include <GInterface.h>
#include <ctime>
#include <sstream>
#include <TextStringManager.h>
#include <ICPlayer.h>


GFX_IMPLEMENT_DYNCREATE(CIFMSFPS, CIFWnd)

CIFMSFPS::CIFMSFPS(void) {
    UpdateLocation = false;
}


CIFMSFPS::~CIFMSFPS(void) {

}

bool CIFMSFPS::OnCreate(long ln) {
    // Populate inherited members
    CIFWnd::OnCreate(ln);

    TB_Func_13("clientlibrary\\common\\frame.ddj", 0, 0);
   // MoveGWnd(677, 125);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iffpsms.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    //RECT rStatic1 = {0, 0, 29, 22};
    //m_label[0] = (CIFStatic *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), rStatic1, 2, 0);
    //m_label[0]->SetTooltipText(L"FPS");
    //m_label[0]->SetStyleThingy(TOOLTIP);
    //m_label[0]->MoveGWnd(679, 125);

    // RECT rStatic2 = {0, 0, 29, 22};
    // m_label[1] = (CIFStatic *) CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), rStatic2, 3, 0);
    // m_label[1]->SetTooltipText(L"MS");
    // m_label[1]->SetStyleThingy(TOOLTIP);
    // m_label[1]->MoveGWnd(680, 153);
    //

    m_count0 = m_IRM.GetResObj<CIFStatic>(8, 1);
    m_count0->SetText(L"0");


    m_count1 = m_IRM.GetResObj<CIFStatic>(9, 1);
    m_count1->SetText(L"0");

    RECT rDrag = {0, 0, 110, 31};
    CreateInstance(this, GFX_RUNTIME_CLASS(CIFDragableArea), rDrag, 6, 0);

    //MoveGWnd(g_pCGInterface->GetSize().width - 244, 7);
    BringToFront();
    ShowGWnd(true);
    return true;
}

void CIFMSFPS::OnMovementRequest(CMsgStreamBuffer &pMsg) {
    byte HasDestination;
    pMsg >> HasDestination;

    if (HasDestination) {
        pMsg >> m_LastMovementDestinationRequest;
        ftime(&m_LastMovementRequestTime);
    }

    pMsg.m_currentReadBytes = 0;
}

void CIFMSFPS::OnMovementResponse(CMsgStreamBuffer &pMsg) {
    unsigned int EntityUniqueID;
    pMsg >> EntityUniqueID;

    if (g_pMyPlayerObj && g_pMyPlayerObj->GetUniqueId() == EntityUniqueID) {
        byte HasDestination;
        pMsg >> HasDestination;


        if (HasDestination) {
            MovementDestination tmpDest;
            pMsg >> tmpDest;
            if (m_LastMovementDestinationRequest == tmpDest) {
                struct timeb tmb;
                ftime(&tmb);

                int Result = ((tmb.time - m_LastMovementRequestTime.time) * 1000) + (tmb.millitm - m_LastMovementRequestTime.millitm);


                m_count1->SetTextFormatted(KmtGetText(L"UIIT_KMT_VALUE_MS"), Result);

                if (Result <= 200)
                    m_count1->m_FontTexture.SetColor(0xFF00FF00);

                else if (Result >= 201 && Result <= 399)
                    m_count1->m_FontTexture.SetColor(0xFFFFD953);

                else if (Result >= 400)
                    m_count1->m_FontTexture.SetColor(0xFFFF0000);

            }
        }
    }

    pMsg.m_currentReadBytes = 0;
}
void CIFMSFPS::UpdateMenuSize()
{
    CIFWnd * minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
    if(minimap != NULL)
    {
        if(minimap->IsVisible())
        {
           wnd_pos x = minimap->GetPos();
           this->MoveGWnd(x.x - 120, x.y + 10);
        }
    }
}
void CIFMSFPS::OnUpdate() {

    if(!UpdateLocation)
    {
        UpdateLocation = true
;        UpdateMenuSize();
    }
    struct timeb tmb;
    ftime(&tmb);

    if (tmb.time > m_LastFPSRefresh) {
        m_count0->SetTextFormatted(KmtGetText(L"UIIT_KMT_VALUE_FPS"), m_FPS);

        if (m_FPS < 40)
            m_count0->m_FontTexture.SetColor(0xFFFF0000);

        else if (m_FPS >= 40 && m_FPS <= 59)
            m_count0->m_FontTexture.SetColor(0xFFFFD953);

        else if (m_FPS >= 60)
            m_count0->m_FontTexture.SetColor(0xFF00FF00);

        m_FPS = 0;
        m_LastFPSRefresh = tmb.time;
    }

    m_FPS += 1;
}