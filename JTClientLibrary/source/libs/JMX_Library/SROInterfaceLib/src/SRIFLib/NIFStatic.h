#pragma once

#include "NIFWnd.h"

// Author : Kyuubi09
// TODO : we need to reverse it, if u feel free, do it plis
class CNIFStatic : public CNIFWnd { // 872 bytes
GFX_DECLARE_DYNAMIC_EXISTING(CNIFStatic, 0x0115e090)
public:
    CNIFStatic(void);

    ~CNIFStatic(void);

public:
    bool OnCreate(long ln) override;

    void RenderMyself() override;

    void OnGfxMessage(Event3D *pMsgData) override;

    void UpdateFontCoordinates() override;

    bool SetText(const wchar_t *src) override;

    virtual bool SetTextFormatted(const wchar_t *format, ...);

private:
    /// \brief Set the text and add dots in the end.
    /// \address 00482360
    void OnEllipsisText(const wchar_t *text, int bEllipsis);

public:
	char pad[32];
};
