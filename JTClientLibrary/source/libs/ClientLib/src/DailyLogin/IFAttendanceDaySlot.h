#include <IFWnd.h>


class CIFAttendanceDaySlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFAttendanceDaySlot)
GFX_DECLARE_MESSAGE_MAP(CIFAttendanceDaySlot)
public:
    CIFAttendanceDaySlot(void);
    ~CIFAttendanceDaySlot(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void ShowOK();
    void ClearOK();
public:
    void UpdateCalenderVisual(int day, const char* month, int year, const char* dayOfWeek);
};                                                                                         