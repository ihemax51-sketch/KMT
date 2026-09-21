#pragma once

#include "IFWnd.h"

class CIFWhisperList : public CIFWnd {
public:
    /// \brief Add the whisper target to me
    /// \param wstrName
    /// \address 007bc910
    void AddToList(std::n_wstring &wstrName);
private:
    char pad_036c[8]; //0x036C
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0374)
    END_FIXTURE()

    RUN_FIXTURE(CIFWhisperList)
};