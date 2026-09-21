#include "DesktopCharacterHud.h"

#include <BSLib/BSLib.h>
#include <GFX3DFunction/RTLoading.h>
#include <d3dx9tex.h>
#include <vector>
#include <cstdio>

// The desktop HUD owns the synchronized pixel buffer.  Keep that proven
// window implementation intact and feed it pixels copied from Silkroad's
// already-loaded profile texture on the game thread.
extern "C" void __cdecl KmtHudLoadAvatar(int refObjectId);
extern "C" void __cdecl KmtHudEnsureDataLock();
extern "C" CRITICAL_SECTION KmtHudDataLock;
extern "C" std::vector<unsigned int> KmtHudAvatarPixels;
extern "C" int KmtHudAvatarWidth;
extern "C" int KmtHudAvatarHeight;
extern "C" int KmtHudAvatarRefObjectId;
extern "C" HWND KmtHudWindow;

#pragma comment(linker, "/alternatename:_KmtHudLoadAvatar=?LoadAvatarForRefObjectId@?A0xbdffea72@@YAXH@Z")
#pragma comment(linker, "/alternatename:_KmtHudEnsureDataLock=?EnsureDataLock@?A0xbdffea72@@YAXXZ")
#pragma comment(linker, "/alternatename:_KmtHudDataLock=?g_dataLock@?A0xbdffea72@@3U_RTL_CRITICAL_SECTION@@A")
#pragma comment(linker, "/alternatename:_KmtHudAvatarPixels=?g_avatarPixels@?A0xbdffea72@@3V?$vector@IV?$allocator@I@std@@@std@@A")
#pragma comment(linker, "/alternatename:_KmtHudAvatarWidth=?g_avatarWidth@?A0xbdffea72@@3HA")
#pragma comment(linker, "/alternatename:_KmtHudAvatarHeight=?g_avatarHeight@?A0xbdffea72@@3HA")
#pragma comment(linker, "/alternatename:_KmtHudAvatarRefObjectId=?g_avatarRefObjectId@?A0xbdffea72@@3HA")
#pragma comment(linker, "/alternatename:_KmtHudWindow=?g_hudWindow@?A0xbdffea72@@3PAUHWND__@@A")

namespace
{
    const UINT HUD_REFRESH_MESSAGE = 0x8452;

    bool CaptureTexturePixels(
        IDirect3DBaseTexture9* baseTexture,
        std::vector<unsigned int>& pixels,
        int* width,
        int* height)
    {
        if (baseTexture == NULL || width == NULL || height == NULL ||
            baseTexture->GetType() != D3DRTYPE_TEXTURE)
            return false;

        IDirect3DTexture9* texture = static_cast<IDirect3DTexture9*>(baseTexture);
        IDirect3DSurface9* sourceSurface = NULL;
        IDirect3DSurface9* convertedSurface = NULL;
        IDirect3DDevice9* device = NULL;
        D3DSURFACE_DESC description;
        D3DLOCKED_RECT lockedRect;
        bool locked = false;
        bool success = false;
        ZeroMemory(&description, sizeof(description));
        ZeroMemory(&lockedRect, sizeof(lockedRect));

        if (FAILED(texture->GetLevelDesc(0, &description)) ||
            description.Width == 0 || description.Height == 0 ||
            description.Width > 1024 || description.Height > 1024)
            goto Cleanup;

        if (FAILED(texture->GetSurfaceLevel(0, &sourceSurface)) ||
            sourceSurface == NULL)
            goto Cleanup;

        if (FAILED(texture->GetDevice(&device)) || device == NULL)
            goto Cleanup;

        if (FAILED(device->CreateOffscreenPlainSurface(
                description.Width,
                description.Height,
                D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM,
                &convertedSurface,
                NULL)) || convertedSurface == NULL)
            goto Cleanup;

        if (FAILED(D3DXLoadSurfaceFromSurface(
                convertedSurface, NULL, NULL,
                sourceSurface, NULL, NULL,
                D3DX_FILTER_NONE, 0)))
            goto Cleanup;

        if (FAILED(convertedSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY)) ||
            lockedRect.pBits == NULL)
            goto Cleanup;
        locked = true;

        try
        {
            pixels.resize(description.Width * description.Height);
        }
        catch (...)
        {
            goto Cleanup;
        }

        for (UINT y = 0; y < description.Height; ++y)
        {
            const unsigned char* source =
                static_cast<const unsigned char*>(lockedRect.pBits) +
                (y * lockedRect.Pitch);
            unsigned int* destination = &pixels[y * description.Width];
            memcpy(destination, source, description.Width * sizeof(unsigned int));
        }

        *width = static_cast<int>(description.Width);
        *height = static_cast<int>(description.Height);
        success = true;

    Cleanup:
        if (locked && convertedSurface != NULL)
            convertedSurface->UnlockRect();
        if (convertedSurface != NULL)
            convertedSurface->Release();
        if (sourceSurface != NULL)
            sourceSurface->Release();
        if (device != NULL)
            device->Release();
        return success;
    }

    bool CommitTexturePixels(
        int refObjectId,
        std::vector<unsigned int>& pixels,
        int width,
        int height)
    {
        if (refObjectId <= 0 || pixels.empty() || width <= 0 || height <= 0)
            return false;

        KmtHudEnsureDataLock();
        EnterCriticalSection(&KmtHudDataLock);
        KmtHudAvatarPixels.swap(pixels);
        KmtHudAvatarWidth = width;
        KmtHudAvatarHeight = height;
        KmtHudAvatarRefObjectId = refObjectId;
        HWND hudWindow = KmtHudWindow;
        LeaveCriticalSection(&KmtHudDataLock);

        if (hudWindow != NULL)
            PostMessageA(hudWindow, HUD_REFRESH_MESSAGE, 0, 0);
        return true;
    }

    bool RefreshPortraitFromTexturePathImpl(int refObjectId, const char* texturePath)
    {
        if (refObjectId <= 0 || texturePath == NULL || texturePath[0] == 0)
            return false;

        std::n_string path(texturePath);
        IDirect3DBaseTexture9* texture = Fun_CacheTexture_Create(path);
        if (texture == NULL)
            return false;

        std::vector<unsigned int> pixels;
        int width = 0;
        int height = 0;
        const bool captured = CaptureTexturePixels(texture, pixels, &width, &height);
        Fun_CacheTexture_Release(&path);
        return captured && CommitTexturePixels(
            refObjectId, pixels, width, height);
    }

    bool RefreshPortraitFromTexturePathSafely(int refObjectId, const char* texturePath)
    {
        __try
        {
            return RefreshPortraitFromTexturePathImpl(refObjectId, texturePath);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool BuildPortraitPath(int refObjectId, char* output, unsigned int outputSize)
    {
        const char* portraitType = NULL;
        int portraitIndex = 0;
        if (refObjectId >= 1907 && refObjectId <= 1919)
        {
            portraitType = "char_ch_man";
            portraitIndex = 1920 - refObjectId;
        }
        else if (refObjectId >= 1920 && refObjectId <= 1932)
        {
            portraitType = "char_ch_woman";
            portraitIndex = 1933 - refObjectId;
        }
        else if (refObjectId >= 14875 && refObjectId <= 14887)
        {
            portraitType = "char_eu_man";
            portraitIndex = refObjectId - 14874;
        }
        else if (refObjectId >= 14888 && refObjectId <= 14900)
        {
            portraitType = "char_eu_woman";
            portraitIndex = refObjectId - 14887;
        }

        if (portraitType == NULL || portraitIndex <= 0 || output == NULL || outputSize == 0)
            return false;

        _snprintf(output, outputSize - 1,
            "juicer\\character\\%s%d.ddj", portraitType, portraitIndex);
        output[outputSize - 1] = 0;
        return true;
    }
}

namespace DesktopCharacterHud
{
    bool HasPortraitForRefObjectId(int refObjectId)
    {
        if (refObjectId <= 0)
            return false;

        __try
        {
            KmtHudEnsureDataLock();
            EnterCriticalSection(&KmtHudDataLock);
            const bool available =
                KmtHudAvatarRefObjectId == refObjectId &&
                KmtHudAvatarWidth > 0 &&
                KmtHudAvatarHeight > 0 &&
                !KmtHudAvatarPixels.empty();
            LeaveCriticalSection(&KmtHudDataLock);
            return available;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool RefreshPortraitFromTexturePath(int refObjectId, const char* texturePath)
    {
        return RefreshPortraitFromTexturePathSafely(refObjectId, texturePath);
    }

    bool RefreshPortraitForRefObjectId(int refObjectId)
    {
        if (refObjectId <= 0)
            return false;

        char portraitPath[128] = {0};
        if (BuildPortraitPath(refObjectId, portraitPath, sizeof(portraitPath)) &&
            RefreshPortraitFromTexturePathSafely(refObjectId, portraitPath))
            return true;

        __try
        {
            KmtHudLoadAvatar(refObjectId);
            return KmtHudAvatarWidth > 0 && KmtHudAvatarHeight > 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}
