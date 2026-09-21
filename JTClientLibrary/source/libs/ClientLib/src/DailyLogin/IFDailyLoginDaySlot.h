#include <IFWnd.h>


class CIFDailyLoginDaySlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFDailyLoginDaySlot)
GFX_DECLARE_MESSAGE_MAP(CIFDailyLoginDaySlot)
public:
    CIFDailyLoginDaySlot(void);
    ~CIFDailyLoginDaySlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void ShowOK();
    void ClearOK();
public:
    void UpdateCalenderVisual(int day, const char* month, int year, const char* dayOfWeek);
};                                                                                         