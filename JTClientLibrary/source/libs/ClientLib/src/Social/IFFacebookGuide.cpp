#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include "IFFacebookGuide.h"
#include "IFSocial.h"

GFX_IMPLEMENT_DYNCREATE(CIFFacebookGuide, CIFDecoratedStatic)

bool CIFFacebookGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    // Do not load any textures/icons
    // TB_Func_13("clientlibrary\\guides\\facebook1.ddj", 0, 0);
    // sub_634470("clientlibrary\\guides\\facebook2.ddj");

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    CIFStatic* lbl = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    if (lbl) {
        lbl->ShowGWnd(false);
        std::n_wstring empty(L"");
        lbl->SetTooltip(empty);
    }

    // Hide completely
    this->ShowGWnd(false);
    this->SetGWndSize(0, 0);
    this->MoveGWnd(-10000, -10000);

    return true;
}

int CIFFacebookGuide::OnMouseLeftUp(int /*a1*/, int /*x*/, int /*y*/)
{
    // No-op
    return 0;
}

void CIFFacebookGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    // sub_633990(); // keep disabled to avoid re-showing
}

void CIFFacebookGuide::OnUpdate()
{
    // No-op
}
