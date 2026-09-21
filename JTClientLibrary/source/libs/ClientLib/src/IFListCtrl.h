#pragma once

#include "IFWnd.h"
#include "LinkedScroll.h"

class CIFListCtrl : public CIFWnd, public CLinkedScroll {
    GFX_DECLARE_DYNAMIC_EXISTING(CIFListCtrl, 0x00ee9580)
public:
    struct SLineOfText {
        char pad_0000[16]; //0x0000
        int Index; //0x0010
        char pad_0014[4]; //0x0014
        CGFontTexture *m_font;//0x0018
    };

public:
    /// \address 00638B50
    void SetBottomAligned(bool bottomAligned);

    /// \address 00638C70
    void SetRespondToMouseMove(bool respondToMouseMove);

    /// \brief Set the color of the line highlighting (usually background color)
    /// \address 00638CF0
    void SetHighlightColor(D3DCOLOR a2);

    /// \brief Enable or disable highlighting the line under cursor
    /// \address 00638D00
    void SetHightlineLine(bool a2);

    /// \address 00638D40
    void sub_638D40(undefined1 param_1);

    /// \address 00638D50
    void sub_638D50(undefined1 param_1);

    /// \address 00638c20
    void SetLineHeight(int height);

    /// \copybrief CIFWnd::OnCreate
    /// \address 00638B20
    bool OnCreate(long ln) override;

    /// \address 0063A940
    /// \remark This func seems to return the currently selected line of text in the list control.
    CIFListCtrl::SLineOfText *sub_63A940();

    /// \copybrief CLinkedScroll::GetNumberOfItems
    /// \address 00638cd0
    int GetNumberOfItems() const override;

    CIFListCtrl::SLineOfText* GetTextByIndex(int index);

    void FUN_00638d20(unsigned char p1);
public:
    char pad_0384[1];      //0x0384
    undefined1 field_0x385;//0x0385
    char m_bHighlighLine;  //0x0386
    byte BlueWithSelected;      //0x0387
    undefined1 field_0x388;//0x0388
    char pad_0389[19];     //0x0389

public:
    /// Number of pixels per line of text (breaking lines take twice the space, or more)
    int m_LineHeight;//0x039C

    /// Number if lines to keep in history
    int m_LinesOfHistory;//0x03A0

    // Number of lines shown by the control
    int m_HeightInLines;//0x03A4

private:
    bool m_bottomAligned;     //0x03A8
    char pad_03A9[1];         //0x03A9
    bool m_respondToMouseMove;//0x03AA
    char pad_03AB[29 - 8];    //0x03AB
    /// Number of items in the list
    int m_numberOfItems;       // 0x3C0
    D3DCOLOR m_OtherColor;     // 0x3C4
    D3DCOLOR m_BackgroundColor;//0x03C8


    BEGIN_FIXTURE()
        ENSURE_SIZE(972)
    ENSURE_OFFSET(field_0x385, 0x385)
    ENSURE_OFFSET(field_0x388, 0x388)
    ENSURE_OFFSET(m_bottomAligned, 0x3a8)
    ENSURE_OFFSET(m_respondToMouseMove, 0x3aa)
    ENSURE_OFFSET(m_numberOfItems, 0x3c0)
    ENSURE_OFFSET(m_OtherColor, 0x3c4)
    ENSURE_OFFSET(m_BackgroundColor, 0x3c8)
    END_FIXTURE()

    RUN_FIXTURE(CIFListCtrl)

};
