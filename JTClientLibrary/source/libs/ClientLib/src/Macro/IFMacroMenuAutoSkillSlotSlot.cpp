
#include <SRIFLib/IFSlotWithHelpEx.h>
#include <GlobalHelpersThatHaveNoHomeYet.h>
#include "IFMacroMenuAutoSkillSlotSlot.h"


GFX_IMPLEMENT_DYNCREATE(CIFMacroMenuAutoSkillSlotSlot, CIFWnd)
GFX_BEGIN_MESSAGE_MAP(CIFMacroMenuAutoSkillSlotSlot, CIFWnd)
GFX_END_MESSAGE_MAP()
CIFMacroMenuAutoSkillSlotSlot::CIFMacroMenuAutoSkillSlotSlot(void)
{
    slot = 0;
}
CIFMacroMenuAutoSkillSlotSlot::~CIFMacroMenuAutoSkillSlotSlot(void)
{
}
bool CIFMacroMenuAutoSkillSlotSlot::OnCreate(long ln)
{
    // Populate inherited members
    CIFWnd::OnCreate(ln);
    wnd_rect sz;
    sz.size.height = 32;
    sz.size.width = 32;
    sz.pos.x = 0;
    sz.pos.y = 0;
    slot = (CIFSlotWithHelp*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), sz, SkillListSlot, 0);
    return true;
}
void CIFMacroMenuAutoSkillSlotSlot::OnUpdate() {

}

void CIFMacroMenuAutoSkillSlotSlot::Clear()
{

}
void CIFMacroMenuAutoSkillSlotSlot::NewUpdate()
{
//printf("slot is clicked?\n");
}
void CIFMacroMenuAutoSkillSlotSlot::PutSkills(int ID){
    if(ID > 0)
    {
        slot->SetType(73);
        slot->GetIconSlot(73, ID);

        slot->FUN_006809a00();
        slot->ShowGWnd(true);
    }
}

