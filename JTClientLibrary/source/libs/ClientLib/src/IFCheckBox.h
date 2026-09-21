#pragma once
#include "IFWnd.h"

class CIFCheckBox : public CIFWnd {
    GFX_DECLARE_DYNAMIC_EXISTING(CIFCheckBox, 0x0110FA70)

public:
    /// \brief Probably tells of the checkbox is currently checked or not
    /// \address 00656db0
    bool GetCheckedState_MAYBE() const;

    /// \brief Probably sets the checkbox checked state
    /// \address 00656d50
    void SetCheckBoxState(bool state);

};
