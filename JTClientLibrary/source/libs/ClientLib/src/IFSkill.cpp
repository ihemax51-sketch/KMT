#include <CustomData/CustomCICPlayer.h>
#include "IFSkill.h"
#include "IFCheckBox.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFSkill, 0x00eea73c)



bool CIFSkill::OnCreateIMPL(long ln) {
    //printf("%s\n", __FUNCTION__);
    bool a = reinterpret_cast<bool (__thiscall *)(const CIFSkill *, long)>(0x006a6e00)(this, ln);
    /*   wnd_rect sz;
       sz.size.width = 16;
       sz.size.height = 16;

       AutoMasteryBicheon = (CIFCheckBox*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFCheckBox), sz, 13456, 1);
       //   AutoMasteryBicheon->MoveGWnd(hp.x - 24, hp.y +2);
       AutoMasteryBicheon->ShowGWnd(true);
       AutoMasteryBicheon->BringToFront();
       AutoMasteryBicheon->FUN_00656d50(false);*/

    return a;
}
void CIFSkill::ShowGWndIMPL(bool p1) {

    reinterpret_cast<void(__thiscall *)(CIFSkill *, bool)>(0x006aa6f0)(this, p1);
}

void CIFSkill::OnUpdateIMPL() {
    reinterpret_cast<void (__thiscall*)(CIFSkill*)>(0x006a5250)(this);
}

void CIFSkill::FUN_006a6e60(undefined4 a1) {
    reinterpret_cast<void (__thiscall*)(CIFSkill*, undefined4)>(0x006a6e60)(this, a1);
}
bool SSkillMaps::SkillIsLearned_MAYBE(int a1)
{
    bool aa = reinterpret_cast<bool(__thiscall*)(SSkillMaps*, int)>(0x009c0b90)(this, a1);
    return aa;
}
undefined4 SSkillMaps::FUN_00a95dc00(int masteryID)
{
    undefined4 aa = reinterpret_cast<undefined4(__thiscall*)(SSkillMaps*, int)>(0x009c0d30)(this, masteryID);
    //printf("%d %p\n", aa, this);
    return aa;
}
byte SSkillMaps::NextSkillLevel_MAYBE(int a1)
{
    byte aa = reinterpret_cast<byte(__thiscall*)(SSkillMaps*, int)>(0x009c1940)(this, a1);
    return aa;
}
byte SSkillMaps::test(int a1)
{
    byte aa = reinterpret_cast<byte(__thiscall*)(SSkillMaps*, int)>(0x009c1180)(this, a1);
    return aa;
}

CSkillData* SSkillMaps::GetSkillDataIFSkillActive(int a1)
{
    CSkillData* aa = reinterpret_cast<CSkillData*(__thiscall*)(SSkillMaps*, int)>(0x009c0c80)(this, a1);
    //printf("%p \n", aa);
    return aa;
}
int SSkillMaps::GetCurrentMasteryCount()
{
    int aa = reinterpret_cast<int(__thiscall*)(SSkillMaps*)>(0x009c1370)(this);
    return aa;
}
void SSkillMaps::FUN_009c12b0(int a1){
    reinterpret_cast<void*(__thiscall*)(SSkillMaps*, int)>(0x009c12b0)(this, a1);
}
/// what 0x00a95dc0 ?
/// last 0x009c0c80
bool SSkillMaps::CanLearnSkill(int skillId) {
    bool aa = reinterpret_cast<bool(__thiscall*)(SSkillMaps*, int)>(0x00a93cc0)(this, skillId);
    //printf("%p \n", aa);
    return aa;
}
bool SSkillMaps::IsLearnedMastery(int skillId) {
    bool aa = reinterpret_cast<bool(__thiscall*)(SSkillMaps*, int)>(0x009c0b50)(this, skillId);
    //printf("%d %p\n", aa, this);
    return aa;
}