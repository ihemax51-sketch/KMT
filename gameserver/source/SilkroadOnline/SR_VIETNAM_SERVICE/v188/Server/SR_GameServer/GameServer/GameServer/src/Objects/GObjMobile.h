//
// Created by kyuubi09 on 3/30/2023.
//
#pragma once

#include "GObj.h"

class CGObjMobile : public CGObj {
DECLARE_DYNAMIC_EXISTING(CGObjMobile, 0x00adeba4)

public:
    void *CGObjMoverByDest; //0x014C
    void *CGObjMoverByDes2; //0x0150
    char pad_0154[24]; //0x0154
    float speed1; //0x016C
    float speed2; //0x0170
    float speed3; //0x0174

    
public:
    void LiveSkill2(int SkillID);
    void OnDeleteObject() override;

    void Func_214() override;

    void Func_212() override;

    void Func_215() override;

    void Func_292() override;

    void Func_291() override;

    void Func_293() override;

    void Func_231() override;

public:
    virtual void Func_300();

    virtual void Func_301();

    virtual void Func_302();

    virtual void Func_303();

    virtual void Func_304();

    virtual void Func_305();

public:
    void Func_211() override;

    void Func_124() override;

    void Func_230() override;

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0178)
    END_FIXTURE()

    RUN_FIXTURE(CGObjMobile)

};