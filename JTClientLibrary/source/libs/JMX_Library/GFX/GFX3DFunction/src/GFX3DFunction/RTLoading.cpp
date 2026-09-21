#include "RTLoading.h"

IDirect3DBaseTexture9 *Fun_CacheTexture_Create(const std::n_string &strPath) {
    return reinterpret_cast<IDirect3DBaseTexture9 *(__cdecl *)(const std::n_string *)>(0x005008F0)(&strPath);
}

void Fun_CacheTexture_Release(std::n_string *pStrPath) {
    reinterpret_cast<void *(__cdecl *)(const std::n_string *)>(0x00500460)(pStrPath);
}

void CRTLoading::SetFM(int nFMType, IFileManager *pFM) {
    reinterpret_cast<void (__thiscall *)(CRTLoading *, int, IFileManager *)>(0x00B876B0)(this, nFMType, pFM);
}
