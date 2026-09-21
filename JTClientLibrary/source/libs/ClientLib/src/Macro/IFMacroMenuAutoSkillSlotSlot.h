#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>
#include <IFSlotWithHelp.h>

class CIFMacroMenuAutoSkillSlotSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoSkillSlotSlot)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoSkillSlotSlot)
public:
    CIFMacroMenuAutoSkillSlotSlot(void);
    ~CIFMacroMenuAutoSkillSlotSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void PutSkills(int ID);
    void Clear();
    void NewUpdate();
public:
    CIFSlotWithHelp * slot;


};