#include "IFSliderCtrl.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFSliderCtrl, 0x00EE9948)

/*void CIFSliderCtrl::TB_Func_13(const std::string str, int a3, int a4) {
    reinterpret_cast<void(__thiscall*)(CTextBoard*, const std::string, int, int)>(0x0065a0f0)(this, str, a3, a4);
}*/

void CIFSliderCtrl::SetSliderTexture(const std::n_string& leftButtonTexturePath, const std::n_string& rightButtonTexturePath,
                                     const std::n_string& midButtonTexturePath) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*,
    const std::n_string,
    const std::n_string,
    const std::n_string)>(0x00659da0)(this, leftButtonTexturePath,
                                      rightButtonTexturePath,
                                      midButtonTexturePath);
}

void CIFSliderCtrl::FUN_006596f0(int param_1, int param_2, int param_3, undefined4 param_4, int param_5,
                                 undefined4 param_6) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, int, int, int, undefined4, int, undefined4)>(0x006596f0)(this,
                                                                                                                param_1,
                                                                                                                param_2,
                                                                                                                param_3,
                                                                                                                param_4,
                                                                                                                param_5,
                                                                                                                param_6);
}

void CIFSliderCtrl::SetMaxButtonSlide(undefined4 param_1) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, undefined4)>(0x00659840)(this, param_1);
}

void CIFSliderCtrl::FUN_006599e0(int param_2, int param_3, int param_4, int param_5) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, int, int, int, int)>(0x006599e0)(this, param_2, param_3,
                                                                                        param_4, param_5);
}

void CIFSliderCtrl::FUN_0065a390(int param_2, int param_3, int param_4) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, int, int,int)>(0x0065a390)(this, param_2, param_3,
                                                                                  param_4);
}



void CIFSliderCtrl::SetEnabledState(bool bState) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, bool)>(0x006597d0)(this, bState);
}

void CIFSliderCtrl::FUN_006598a0(int param_1, int param_2) {
    reinterpret_cast<void(__thiscall*)(CIFSliderCtrl*, int, int)>(0x006598a0)(this, param_1, param_2);
}