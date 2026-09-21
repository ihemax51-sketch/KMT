#include <GInterface.h>
#include <BSLib/multibyte.h>
#include "IFMacroMenuAutoSkillSlotViewer.h"
#include <GlobalDataManager.h>
#include <ICPlayer.h>
#include <BSLib/Debug.h>

GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoSkillSlotViewer, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoSkillSlotViewer, CIFWnd)
GFX_END_MESSAGE_MAP()

CIFMacroMenuAutoSkillSlotViewer::CIFMacroMenuAutoSkillSlotViewer(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

CIFMacroMenuAutoSkillSlotViewer::~CIFMacroMenuAutoSkillSlotViewer(void)
{
    BS_DEBUG_LOW(">" __FUNCTION__);
}

bool CIFMacroMenuAutoSkillSlotViewer::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    wnd_rect sz;
    sz.pos.x = 8;
    sz.pos.y = 2;
    sz.size.width = 32;
    sz.size.height = 32;
    for(int i =0; i< 8;i++) {
        slot1[i] = (CIFMacroMenuAutoSkillSlotSlot*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFMacroMenuAutoSkillSlotSlot), sz, 301 + i, 0);
        sz.pos.x += 36;
    }
    return true;
}


