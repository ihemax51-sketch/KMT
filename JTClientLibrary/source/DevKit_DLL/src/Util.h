#pragma once

#include "BSLib/BSLib.h"
#include <Game.h>

bool Setup();
bool SetupWithDiagnostics();

bool DoesFileExists(const std::string &name);

void InstallRuntimeClasses(CGame *);
bool EnsureRuntimeClassesInstalled();

void RegisterObject(const CGfxRuntimeClass *);

typedef void(*overrideFnPtr)();
void RegisterObject(const CGfxRuntimeClass *);
void GetSilkPos(uregion dis, D3DVECTOR& location);
bool ObjIntersect(CIObject* Src, CIObject* Dis);


bool intersect(D3DXVECTOR2 aa, D3DXVECTOR2 bb, D3DXVECTOR2 cc, D3DXVECTOR2 dd);
double determinant(double v1, double v2, double v3, double v4);
D3DVECTOR GetSilkPosD3D(uregion dis, D3DVECTOR location);

extern std::vector<overrideFnPtr> override_objects;

template<typename T, int address>
void OverrideRtClassAt() {
    CGfxRuntimeClass *rt = (CGfxRuntimeClass *) address;

    rt->m_pfnCreateObject = T::CreateObject;
    rt->m_pfnDeleteObject = T::DeleteObject;
}

template<typename T, int address>
void OverrideObject() {
    override_objects.push_back(&OverrideRtClassAt<T, address>);
}

void PatchWatermark();
