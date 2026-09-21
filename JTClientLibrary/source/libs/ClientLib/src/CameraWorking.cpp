#include "CameraWorking.h"

void CCameraWorking::AddKeyframe(float a1, const D3DVECTOR& translation, const D3DXVECTOR4& rotation)
{
	reinterpret_cast<void(__thiscall*)(CCameraWorking*,float,const D3DVECTOR*,const D3DXVECTOR4*)>(0x004E67B0)(this, a1, &translation, &rotation);
}

void CCameraWorking::sub_4E6630()
{
	reinterpret_cast<void(__thiscall*)(CCameraWorking*)>(0x4E6630)(this);
}
void CCameraWorking::AddKeyframeForSelectIdol(float a1, const D3DVECTOR &translation, const D3DXVECTOR4 &rotation) {

    a1 = 1;
    reinterpret_cast<void(__thiscall *)(CCameraWorking *, float, const D3DVECTOR *, const D3DXVECTOR4 *)>(0x004E67B0)(this, 1, &D3DXVECTOR3(370.00, 20, 206.41), &D3DXVECTOR4(0.80, 0.81, 0, 30));


    //theApp.camera.origin = D3DXVECTOR3(1168.20, 579.10, 602.10);
    //  theApp.camera.rotation_to_world = D3DXVECTOR4(0.62, 2.53, 0, 30);
    //
}