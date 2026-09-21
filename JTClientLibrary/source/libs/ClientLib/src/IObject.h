#pragma once

#include "GFXMainFrame/IEntity.h"

#include <d3d9.h>

#include "GlobalHelpersThatHaveNoHomeYet.h"
#include <CObjCharacter.h>
#include "NavMesh/RTNavCellQuad.h"


class CIObject : public CIEntity {
//GFX_DECLARE_DYNAMIC_EXISTING(CIObject, 0x00eef6b0)
public:

    virtual void Func_14();


    /// \address 004df110
    virtual void Func_15(int param_1, float *param_2);

    virtual void Func_16();

    virtual void Func_17();



    uregion GetRegion() const;

    D3DVECTOR GetLocation() const;

    CRTNavCellQuad* GetNavCell() const;
/*public:
    char pad_0054[32]; //0x0054
    CRTNavCellQuad* nav;
    void* m_object_under_foot; //0x0078
    uregion region; //0x007C
    char pad_007E[2]; //0x007E
    D3DVECTOR location; //0x0080
    float rotation; //0x008C
    char pad_0090[12]; //0x0090
    CCObjCharacter* m_pCCObjAnimation; //0x009C
    char pad_00A0[60]; //0x00A0
*/
public:
    // size 220 - 84 = 136
    char pad_0054[32]; //0x0054
    CRTNavCellQuad* m_navcell; //0x0074
public:
    void* m_object_under_foot; //0x0078
    uregion region; // 0x7C (124)
    char asdsadfas[2];
    D3DVECTOR location; // 0x80
    float rotation; // 0x8C
    D3DVECTOR location_camera_center; // 0x90
    CCObjCharacter* m_pCCObjAnimation; //0x009C
    char pad_0x009C[24]; //0x009C
    __int32 times_drawn; //0x00B8
    char pad_0x00BC[20]; //0x00BC
    float distance; //0x00D0
    char pad_0x00D4[8]; //0x00D4


BEGIN_FIXTURE()
        ENSURE_SIZE(0xdc)
        ENSURE_OFFSET(region, 0x7c)
        ENSURE_OFFSET(location, 0x80)
        ENSURE_OFFSET(rotation, 0x8C)
    END_FIXTURE()

    RUN_FIXTURE(CIObject)
};
