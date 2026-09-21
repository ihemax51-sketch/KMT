#pragma once

#include "NIFWnd.h"

#include "NIFBlockWnd.h"

class CNIFCommunityWnd : public CNIFWnd {
public:
    /// \brief Get a pointer to the chat block window
    /// \address 0058eeb0
    /// \return Pointer to the chat block window
    CNIFBlockWnd *GetBlockWnd();
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0348)
    END_FIXTURE()

    RUN_FIXTURE(CNIFCommunityWnd)
};
