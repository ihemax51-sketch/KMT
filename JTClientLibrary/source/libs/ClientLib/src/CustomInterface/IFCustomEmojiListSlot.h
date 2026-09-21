#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFCustomEmojiListSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFCustomEmojiListSlot)
GFX_DECLARE_MESSAGE_MAP(CIFCustomEmojiListSlot)
public:
    CIFCustomEmojiListSlot(void);
    ~CIFCustomEmojiListSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void SetName(std::string Path, std::wstring FullText);
    std::wstring FullTextx;
public:
    void Clear();
    int OnMouseLeftUp(int a1, int x, int y) override;
};