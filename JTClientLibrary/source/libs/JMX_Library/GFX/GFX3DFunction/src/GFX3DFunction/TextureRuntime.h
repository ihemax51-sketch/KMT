#pragma once

#include <d3d9.h>

#include <GFXFM/IFileManager.h>

class CTextureRuntime {
public:
    virtual ~CTextureRuntime();

    /// \address 00b87eb0
    virtual void SetTextureData(IDirect3DBaseTexture9 *pTexture3D, DWORD dwTextureLOD);

    /// \address 00b875c0
    virtual const IDirect3DBaseTexture9 *GetRuntimeTexture();

private:
    IDirect3DBaseTexture9 *m_pMainTexture3D; //0x0004
    IDirect3DBaseTexture9 *m_pRuntimeTexture; //0x0008 // Current
    IFileManager *m_pTextureFileContainer; //0x000C // Who knows, maybe its pk2 or disk.
    char pad_0010[4]; //0x0010
    std::n_string m_strTexturePath; //0x0014
    char pad_0030[4]; //0x0030
    DWORD m_dwTextureLOD; //0x0034
    CRITICAL_SECTION m_csTexture; //0x0038
    DWORD m_dwRuntimeType; //0x0050 // 2, 1 and they are unknown
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0054)
        ENSURE_OFFSET(m_pMainTexture3D, 0x0004)
        ENSURE_OFFSET(m_pRuntimeTexture, 0x0008)
        ENSURE_OFFSET(m_pTextureFileContainer, 0x000C)
        ENSURE_OFFSET(m_strTexturePath, 0x0014)
        ENSURE_OFFSET(m_dwTextureLOD, 0x0034)
        ENSURE_OFFSET(m_csTexture, 0x0038)
        ENSURE_OFFSET(m_dwRuntimeType, 0x0050)
    END_FIXTURE()

    RUN_FIXTURE(CTextureRuntime)
};
