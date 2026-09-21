#pragma once

#include "IFWnd.h"
#include <IFStatic.h>

class CIFTimerWnd : public CIFWnd
{
GFX_DECLARE_DYNAMIC_EXISTING(CIFTimerWnd, 0x00ee7b20)
public:
    void StartTimerWnd(int time);

    unsigned int SetTimer(unsigned int time, unsigned int time2);

    void StartTimerWnd2();

private:
    char pad_0x036C[32]; //0x036C
    CIFStatic* m_number0; //0x038C
    CIFStatic* m_number1; //0x0390
    CIFStatic* m_colon; //0x0394
    CIFStatic* m_number2; //0x0398
    CIFStatic* m_number3; //0x039C
    CIFStatic* N00006506; //0x03A0
    CIFStatic* N00006507; //0x03A4
    CIFStatic* N0000650B; //0x03A8


BEGIN_FIXTURE()
        ENSURE_SIZE(0x03AC)
        ENSURE_OFFSET(m_number0, 0x038C)
        ENSURE_OFFSET(m_number1, 0x0390)
        ENSURE_OFFSET(m_colon, 0x0394)
        ENSURE_OFFSET(m_number2, 0x0398)
        ENSURE_OFFSET(m_number3, 0x039C)
        ENSURE_OFFSET(N00006506, 0x03A0)
        ENSURE_OFFSET(N00006507, 0x03A4)
        ENSURE_OFFSET(N0000650B, 0x03A8)

    END_FIXTURE()

    RUN_FIXTURE(CIFTimerWnd)


};//Size=0x03AC