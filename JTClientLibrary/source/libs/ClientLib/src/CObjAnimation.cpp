#include "CObjAnimation.h"

void CCObjAnimation::SetResource(const std::n_string &path, undefined4 a2) {
    reinterpret_cast<void(__thiscall *)(CCObjAnimation *, const std::n_string *, undefined4)>(0x00a5b4b0)(this, &path, 2);
}

void CCObjAnimation::SetLocation(const D3DVECTOR &position) {
    reinterpret_cast<void(__thiscall *)(CCObjAnimation *, const D3DVECTOR &)>(0x00a5a880)(this, position);
}

void CCObjAnimation::SetRotation(float rotation) {
    reinterpret_cast<void(__thiscall *)(CCObjAnimation *, float)>(0x00a5a8c0)(this, rotation);
}
void CCObjAnimation::Func_3IMPL(undefined4 p1, undefined4 p2, undefined4 p3 , undefined4 p4 , float p5 , float p6){
    //printf("%d %d %d %d %f %f\n", p1, p2, p3, p4, p5, p6);
    reinterpret_cast<void(__thiscall *)(CCObjAnimation *, undefined4, undefined4, undefined4, undefined4, float, float)>(0x00a5b6d0)(this,p1,p2,p3,p4,p5,p6);
}
