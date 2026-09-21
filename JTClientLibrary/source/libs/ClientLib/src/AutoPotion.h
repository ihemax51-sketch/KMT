#pragma once

#include "support/AbiTest.h"

class CAutoPotion {
public:
    enum ePotionType {
        HP_POTION = 0,
        MP_POTION,
        CURE_PILL,
        AUTO_POTION_COUNT
    };

private:
    enum eInitState : char {
        NotInit = 0,
        InitComplete = 2
    };

    struct Potion {
        bool bIsEnabled;
        bool bTimerRunning;
        unsigned char ThresholdPercent;// threshold in percent
        unsigned int ThresholdValue;   // threshold value from HP or MP
        eInitState InitState;

    private:
        unsigned char pad1[3];

    public:
        unsigned char BeltAndSlotCombined;

        BEGIN_FIXTURE()
        ENSURE_SIZE(0x10)
        ENSURE_OFFSET(bIsEnabled, 0x0000)
        ENSURE_OFFSET(bTimerRunning, 0x0001)
        ENSURE_OFFSET(ThresholdPercent, 0x0002)
        ENSURE_OFFSET(ThresholdValue, 0x0004)
        ENSURE_OFFSET(InitState, 0x0008)
        ENSURE_OFFSET(BeltAndSlotCombined, 0x000c)
        END_FIXTURE()
        RUN_FIXTURE(Potion)
    };

public:
    virtual ~CAutoPotion();

private:
    /// \address 0082ce80
    void UsePotion(unsigned char byType);

    /// \address 0082cd90
    int GetPotionDelay() const;


private:
    Potion m_data_in_array[AUTO_POTION_COUNT];

    bool m_bUsePotionDelay;
    int m_nUseDelay;// delay in ms
};
