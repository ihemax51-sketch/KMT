#pragma once
#include "IOBillboard.h"

#include "AnimateableSomething.h"

class CIDamageText : public CIOBillboard {
public:
    void Func_9_IMPL();

private:
    char pad_0x0170[0x10]; // 0x0170

    CAnimateableSomething m_positionAnimator;// 0x0180 // size 32
    CAnimateableSomething m_scaleAnimator;// 0x01a0
    CAnimateableSomething m_opacityAnimator;// 0x01c0

    char pad_0x01e0[4 + 1];

    bool m_moveUp;

private:
    BEGIN_FIXTURE()
    ENSURE_OFFSET(m_positionAnimator, 0x0180)
    ENSURE_OFFSET(m_scaleAnimator, 0x01a0)
    ENSURE_OFFSET(m_opacityAnimator, 0x01c0)
    ENSURE_OFFSET(m_moveUp, 0x01e5)
    ENSURE_SIZE(0x1e8)
    END_FIXTURE()

    RUN_FIXTURE(CIDamageText)
};
