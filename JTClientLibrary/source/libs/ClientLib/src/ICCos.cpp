#include <CustomData/CustomDataManager.h>
#include "ICCos.h"
#include "unsorted.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CICCos, 0x00EEF6F0)

CLASSLINK_STATIC_IMPL(CICCos)
void CICCos::RemoveEffect(unsigned __int32 effectID)
{
    reinterpret_cast<void* (__thiscall*)(CICCos*, unsigned __int32)>(0x009CD2B0)(this, effectID);
}
#include <string>
void CICCos::MakeEffect(unsigned __int32 effectID)
{
    reinterpret_cast<void* (__thiscall*)(CICCos*, unsigned __int32)>(0x009CF960)(this, effectID);
}
void CICCos::Func_15(int param_1, float* param_2) {
    //printf("%s\n", __FUNCTION__);
    reinterpret_cast<void (__thiscall *)(const CICCos *, int param_1, float *param_2)>(0x009c6820)(this, param_1, param_2);
}

void CICCos::Func_15_impl(int param_1, float* param_2) {
    CICCos::Func_15(param_1, param_2);
}
