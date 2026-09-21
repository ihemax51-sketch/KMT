#pragma once

#include <d3d9.h>

#include "BSLib/BSlib.h"

#include "GFXFM/IFileManager.h"

IDirect3DBaseTexture9 *Fun_CacheTexture_Create(const std::n_string &strPath);

void Fun_CacheTexture_Release(std::n_string *pStrPath);

#define THREADTEXTURE_FMTYPE_NUM 5

// TODO : This class missing alot of important members, u know what u have todo!
class CRTLoading {
public:
    ///\address 00b8bbc0
    CRTLoading();

public:
    void SetFM(int nFMType, IFileManager *pFM);

public:
    char gap0[0x4C];
    IFileManager *fileManager[THREADTEXTURE_FMTYPE_NUM];
};

#define g_pRTTextureLoadingManager *(reinterpret_cast<CRTLoading *>(0x0110f5e8))
