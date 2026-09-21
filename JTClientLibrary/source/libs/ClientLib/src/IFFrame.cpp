#include "IFFrame.h"
#include <BSLib/Debug.h>

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFFrame, 0x00EECB14)

bool CIFFrame::OnCreate(long ln) {
    CIFTileWnd::OnCreate(ln);

    Set_field_036C(false);
    return true;
}

void CIFFrame::RenderMyself() {
    /// \todo implement me.
    reinterpret_cast<void (__thiscall *)(CIFFrame *)>(0x00815490)(this);
}

void CIFFrame::SetFrameTexture(std::n_string strTexture) {
    std::n_string strCorners[4] = {strTexture + "left_up.ddj", strTexture + "right_up.ddj",
                                   strTexture + "right_down.ddj", strTexture + "left_down.ddj"};
    SetCornersTexture(strCorners);

    std::n_string strEdges[4] = {strTexture + "left_side.ddj", strTexture + "mid_up.ddj",
                                 strTexture + "right_side.ddj", strTexture + "mid_down.ddj"};
    SetEdgesTexture(strEdges);
}
