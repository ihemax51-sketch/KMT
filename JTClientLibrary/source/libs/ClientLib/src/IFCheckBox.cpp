#include "IFCheckBox.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFCheckBox, 0x0110FA70)

bool CIFCheckBox::GetCheckedState_MAYBE() const {
    return reinterpret_cast<bool (__thiscall *)(const CIFCheckBox *)>(0x00656db0)(this);
}

void CIFCheckBox::SetCheckBoxState(bool state) {
    reinterpret_cast<void (__thiscall *)(CIFCheckBox *, bool)>(0x00656d50)(this, state);
}
