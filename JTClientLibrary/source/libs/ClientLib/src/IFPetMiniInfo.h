#pragma once
#include "IFWnd.h"
#include "IFStatic.h"
#include "IFGauge.h"
#include "IFBuffViewer.h"
#include "IFStaticWithPictureClip.h"

class CIFPetMiniInfo  : public CIFStatic {
GFX_DECLARE_DYNAMIC_EXISTING(CIFPetMiniInfo, 0x00EEC228)

public:
    void OnUpdateIMPL();
    bool OnCreateIMPL(long ln);
public:
    int m_petUniqueID; //0x0380
    CIFStaticWithPictureClip *CIFStaticWithPictureClip; //0x0384
    CIFStatic *m_petName; //0x0388
    CIFStatic *m_petLevel; //0x038c
    CIFGauge *m_petHp; //0x0390
    CIFGauge *m_petHGP; //0x0394
    CIFBuffViewer * buffviewer; //0x0398
    int unk0; //0x039C
    CIFStatic* StaticPTR; //0x03A0
BEGIN_FIXTURE()
        ENSURE_SIZE(932)
        ENSURE_OFFSET(m_petUniqueID, 0x0380)
        ENSURE_OFFSET(CIFStaticWithPictureClip, 0x0384)
        ENSURE_OFFSET(m_petName, 0x0388)
        ENSURE_OFFSET(m_petLevel, 0x038c)
        ENSURE_OFFSET(m_petHp, 0x0390)
        ENSURE_OFFSET(m_petHGP, 0x0394)
        ENSURE_OFFSET(buffviewer, 0x0398)
        ENSURE_OFFSET(unk0, 0x039C)
        ENSURE_OFFSET(StaticPTR, 0x03A0)

    END_FIXTURE()

    RUN_FIXTURE(CIFPetMiniInfo)

}; //0x3a4

