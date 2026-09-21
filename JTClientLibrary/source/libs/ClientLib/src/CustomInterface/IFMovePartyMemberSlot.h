#include <IFStatic.h>
#include <IFWnd.h>
#include <IFBarWnd.h>

class CIFMovePartyMemberSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMovePartyMemberSlot)
GFX_DECLARE_MESSAGE_MAP(CIFMovePartyMemberSlot)
public:
    CIFMovePartyMemberSlot(void);
    ~CIFMovePartyMemberSlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void SetName(const wchar_t * CharName, const wchar_t * Location, int ptmaster);
    void TakeBtn();
    std::n_wstring CharName16;
public:
    void Clear();
};