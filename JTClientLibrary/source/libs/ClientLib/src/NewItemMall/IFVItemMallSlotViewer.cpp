#include <GInterface.h>
#include <BSLib/multibyte.h>
#include "IFVItemMallSlotViewer.h"
#include "IFVItemMall.h"
#include <GlobalDataManager.h>
#include <ICPlayer.h>
#include <BSLib/Debug.h>

GFX_IMPLEMENT_DYNCREATE(CIFVItemMallSlotViewer, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFVItemMallSlotViewer, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFVItemMallSlotViewer::CIFVItemMallSlotViewer(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFVItemMallSlotViewer::~CIFVItemMallSlotViewer(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFVItemMallSlotViewer::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvitemmallslotviewer.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    slot1 = m_IRM.GetResObj<CIFVItemMallSlot>(3, 1);

    slot2 = m_IRM.GetResObj<CIFVItemMallSlot>(4, 1);

    slot3 = m_IRM.GetResObj<CIFVItemMallSlot>(5, 1);

    slot1->ShowGWnd(false);
    slot2->ShowGWnd(false);
    slot3->ShowGWnd(false);

    return true;
}


