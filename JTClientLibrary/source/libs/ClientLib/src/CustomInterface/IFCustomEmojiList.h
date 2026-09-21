#pragma once
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFCustomEmojiListSlot.h"

class CIFCustomEmojiList : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFCustomEmojiList)
GFX_DECLARE_MESSAGE_MAP(CIFCustomEmojiList)
private:
    CIFCustomEmojiList(void);
    ~CIFCustomEmojiList(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
public:
    void UpdateMenuSize();
    void Clear();

    CIFCustomEmojiListSlot * emojiSlots[100];
};
