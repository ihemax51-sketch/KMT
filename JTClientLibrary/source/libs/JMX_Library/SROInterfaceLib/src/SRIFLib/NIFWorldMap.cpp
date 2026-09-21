//
// Created by YUMBUL on 30.06.2024.
//

#include <map>
#include <GFX3DFunction/RStateMgr.h>
#include <GFX3DFunction/RTLoading.h>
#include <CustomData/CustomDataManager.h>
#include <SimpleViewer/VBDynamic.h>
#include <Menu/IFUniqueHistory.h>
#include <Menu/IFMenu.h>
#include <ctime>
#include <GFXMainFrame/Controler.h>
#include <CharacterDependentData.h>
#include <NavMesh/LocationInfo.h>
#include <BSLib/multibyte.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include "NIFWorldMap.h"
#include "ICPlayer.h"
#include "GInterface.h"
#include "Game.h"

#if KMT_UNIQUE_MAP_DIAGNOSTICS
namespace
{
void LogUniqueMapFocus(const char* phase, CNIFWorldMap* map,
                       int regionX, int regionY, int mapPositionX, int mapPositionZ,
                       int baseRegionX, int baseRegionY, int deltaX, int deltaY)
{
    if (g_CGame == NULL)
        return;

    char path[MAX_PATH];
    _snprintf(path, MAX_PATH - 1, "%s\\Setting\\KMTGuardUniqueMap.log", theApp.GetWorkingDir());
    path[MAX_PATH - 1] = '\0';

    FILE* file = fopen(path, "a");
    if (file == NULL)
        return;

    fprintf(file,
            "%s region=(%d,%d) map=(%d,%d) base=(%d,%d) delta=(%d,%d) "
            "mode=%d manual=%u current=%p origin=(%.3f,%.3f) state=(%d,%.3f,%d,%.3f)\n",
            phase, regionX, regionY, mapPositionX, mapPositionZ,
            baseRegionX, baseRegionY, deltaX, deltaY,
            map->MapType1IsSmall0IsBig,
            static_cast<unsigned int>(map->IsAuto1and0IsManual),
            map->m_currentPos, map->m_currentPosX, map->m_currentPosY,
            map->m_stateX, map->m_stateY, map->m_viewWidth, map->m_viewHeight);
    fclose(file);
}
}
#endif

GFX_MSGMAP* CNIFWorldMap::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    /* {GFX_WM_COMMAND, 0, 14, 14, BSSig_u12, 0,
                             (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFSkillBoard::OnBtnClick))},
 */
                    {GFX_WM_COMMAND, 0, 13402, 13402, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CNIFWorldMap::OnClickPingButton))},
                    // Diğer özel mesaj girişleri buraya eklenebilir
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00d99074), skillBoardMessageEntries,
            };
    return &newmap;
}
void CNIFWorldMap::OnClickPingButton()
{
    const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
    if(partyData.bIsPartyMaster && partyData.bInParty)
    {
        CIFMenu *menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        if(menu->CanSendPing)
        {
            menu->CanSendPing = false;
            g_Controler->SetCustomCursor(149);

        }
        else if(!menu->CanSendPing)
        {
            menu->CanSendPing = true;
            g_Controler->SetCustomCursor(166);
        }
    }
    else
    {
        g_pCGInterface->ShowMessage_Warning(L"Only party master can be use ping feature.");
    }
}

bool CNIFWorldMap::OnWMCreate(long ln) {
    bool x = reinterpret_cast<bool (__thiscall *)(CNIFWorldMap *,long)>(0x0060F340)(this,ln);

    wnd_rect sz;
    sz.size.width = 36;
    sz.size.height = 36;
    CIFButton* button = (CIFButton *) CIFWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFButton), sz, 13402, 0);
    button->TB_Func_13("interface\\worldmap\\wmap_button_location.ddj", 1, 1);
    button->ShowGWnd(true);
    //todo interface\targetwindow\tw_icon_unique.ddj
    return x;
}
void CNIFWorldMap::Fun_0061a620()
{
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*)>(0x0061a620)(this);
}
int CNIFWorldMap::GetRegionTypeMaybe(unsigned short region,float x,float y,float z)
{
    int aa = reinterpret_cast<int(__thiscall*)(CNIFWorldMap*,short , float,float,float)>(0x00610d30)(this,region,x,y,z);
  //  printf("FUN_00610d30 %d %d %f %f %f\n", aa, region, x, y, z);
    return aa;
}
void CNIFWorldMap::FUN_006256e0(int p1)
{
    // printf("FUN_006256e0 %d \n", p1);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int)>(0x0060fc10)(this, p1);
}
void CNIFWorldMap::FUN_00622b60(int p1, int p2)
{
    printf("%d %d \n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int, int)>(0x00622b60)(this, p1, p1);
}
int CNIFWorldMap::FUN_00614170(undefined4 p1, short p2, float p3, float p4, float p5, undefined4 p6)
{
    //printf("FUN_00614170 %d %d %f %f %f %d\n",p1, p2, p3, p4, p5,p6);
    return reinterpret_cast<int(__thiscall*)(CNIFWorldMap*, undefined4, short, float, float, float, undefined4)>(0x00614170)(this, p1, p2, p3, p4, p5, p6);
}

/// 0061a620 0 byte
/// 0060fc10 1 byte


void CNIFWorldMap::Fun_00622f00(undefined4 p1, undefined4 p2,undefined4  p3, undefined4  p4,undefined4  p5,undefined4 p6,undefined4 p7,undefined4 p8,
                                undefined4 p9, undefined4 p10, undefined4 p11){
    //printf("Fun_00622f00 %d %d %d %d %d %d %d %d %d %d %d\n",p1, p2, p3, p4, p5,p6, p7, p8, p9, p10, p11);
    reinterpret_cast<int(__thiscall*)(CNIFWorldMap*,undefined4, undefined4, undefined4,
                                      undefined4, undefined4, undefined4,undefined4, undefined4, undefined4, undefined4, undefined4)>
    (0x00622f00)(this, p1, p2, p3, p4, p5, p6, p7, p8, p9,p10, p11);
}
void CNIFWorldMap::FUN_00621c60(float p1, float p2)
{
    //printf("FUN_00621c60 %f %f\n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, float, float)>(0x00621c60)(this, p1, p1);
}
void CNIFWorldMap::FUN_00620550(int p1, int p2)
{
    //printf("FUN_00620550 %d %d\n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int, int)>(0x00620550)(this, p1, p1);
}
void CNIFWorldMap::SetMapMode()
{
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*)>(0x0061ee80)(this);
}

void CNIFWorldMap::FUN_00624f40(CWorldMapGuideData* p1, undefined4 p2,undefined4 p3)
{
    //printf("FUN_00624f40 %p %d %d \n", p1, p2, p3);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, CWorldMapGuideData*, undefined4,undefined4)>(0x00624f40)(this, p1, p2, p3);
}
void CNIFWorldMap::TEST(int p1)
{
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int)>(0x00610980)(this, p1);
}

void CNIFWorldMap::FUN_0061c5a0(int p1,int  p2)
{
    printf("%d %d \n", p1, p2);
    reinterpret_cast<void(__thiscall *) (CNIFWorldMap*, int, int)>(0x0061c5a0)(this, p1, p2);
}

bool CNIFWorldMap::CenterFieldMapAt(int regionX, int regionY, int mapPositionX, int mapPositionZ)
{
#if KMT_UNIQUE_MAP_DIAGNOSTICS
    LogUniqueMapFocus("before", this, regionX, regionY, mapPositionX, mapPositionZ,
                      0, 0, 0, 0);
#endif

    // Reproduce the field branch of the client's native map navigator without
    // fabricating its by-value std::n_wstring argument. This is the same reset,
    // page selection, centering calculation, and native pan used by 0x00622F00.
    field_229ad = 1;
    Fun_0061a620();
    FUN_006256e0(0);
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int, int)>(0x00622b60)(this, 0, 0);

    if (m_currentPos != NULL)
        reinterpret_cast<void(__thiscall*)(CNIFWorldMap*)>(0x0061b510)(this);

    if (m_currentPos == NULL)
    {
#if KMT_UNIQUE_MAP_DIAGNOSTICS
        LogUniqueMapFocus("no-current-map", this, regionX, regionY, mapPositionX, mapPositionZ,
                          0, 0, 0, 0);
#endif
        return false;
    }

    const int currentMapInfo =
            reinterpret_cast<int(__thiscall*)(int*)>(0x0096f4e0)(m_currentPos);
    if (currentMapInfo == 0 || IsBadReadPtr(reinterpret_cast<void*>(currentMapInfo), 0x60))
    {
#if KMT_UNIQUE_MAP_DIAGNOSTICS
        LogUniqueMapFocus("bad-current-map", this, regionX, regionY, mapPositionX, mapPositionZ,
                          0, 0, 0, 0);
#endif
        return false;
    }

    const int baseRegionX = *reinterpret_cast<int*>(currentMapInfo + 0x58);
    const int baseRegionY = *reinterpret_cast<int*>(currentMapInfo + 0x5c);
    const int targetX = ((regionX - baseRegionX) << 5) + mapPositionX;
    const int targetY = ((baseRegionY - regionY) << 5) + mapPositionZ;

    int centerX = targetX;
    int centerY = targetY;
    if (MapType1IsSmall0IsBig == 0)
    {
        centerX = 0x130;
        centerY = 0xb0;
    }
    else if (MapType1IsSmall0IsBig == 1)
    {
        centerX = 0x68;
        centerY = 0x68;
    }

    const int deltaX = centerX - targetX;
    const int deltaY = centerY - targetY;
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*, int, int)>(0x0061c5a0)(this, deltaX, deltaY);
    TEST(0);

#if KMT_UNIQUE_MAP_DIAGNOSTICS
    LogUniqueMapFocus("after", this, regionX, regionY, mapPositionX, mapPositionZ,
                      baseRegionX, baseRegionY, deltaX, deltaY);
#endif
    return true;
}

void CNIFWorldMap::FUN_006217e0(int p1)
{
    printf("FUN_006217e0 %d \n", p1);
    reinterpret_cast<void(__thiscall *) (CNIFWorldMap*, int)>(0x006217e0)(this, p1);
}
int currentFrame = 0;
const int frameCount = 432 / 36; // 36x36 bölümlerine ayırdığımız için 12 kare
bool timerrunning = false;

void UpdateAnimation() {
    currentFrame = (currentFrame + 1) % frameCount;
}
void CNIFWorldMap::UpdateButtonPosition()
{
    const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
    if(partyData.bIsPartyMaster && partyData.bInParty)
    {
        if(!this->GetGuiFromList<CNIFButton>(13402)->IsVisible())
        {
            this->GetGuiFromList<CIFButton>(13402)->ShowGWnd(true);
        }
    }
    else
    {
        if(this->GetGuiFromList<CNIFButton>(13402)->IsVisible())
        {
            this->GetGuiFromList<CIFButton>(13402)->ShowGWnd(false);
        }
    }
    wnd_pos x = this->GetGuiFromList<CNIFButton>(20)->GetPos();
    if(this->GetGuiFromList<CNIFButton>(6)->IsVisible())
    {
        if(this->GetGuiFromList<CIFButton>(13402) != NULL)
        {
            this->GetGuiFromList<CIFButton>(13402)->MoveGWnd(x.x - 76, x.y);
        }
    }
    else
    {
        if(this->GetGuiFromList<CIFButton>(13402) != NULL)
        {
            this->GetGuiFromList<CIFButton>(13402)->MoveGWnd(x.x - 38, x.y);
        }
    }

}
void CNIFWorldMap::OnWMRenderMySelf() {
    reinterpret_cast<void(__thiscall*)(CNIFWorldMap*)>(0x0061BAC0)(this);
    UpdateButtonPosition();

    CIFUniqueHistory* uniqueHistory =
            g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1);

    if (uniqueHistory != NULL)
    {
        if (uniqueHistory->SelectedRegionID != 0)
        {
            int REGIONID = uniqueHistory->SelectedRegionID;
            float X = uniqueHistory->SelectedX;

            float Z = uniqueHistory->SelectedZ;
            byte mapType = uniqueHistory->SelectedMapType;
            //            int Y = g_pCGInterface->m_IRM.GetResObj<CIFUniqueHistory>(UniqueHistoryID, 1)->SelectedY;
            if (m_currentPos != NULL && m_currentPos != 0x0)
            {
                int currentTile_MAYBE = reinterpret_cast<int(__thiscall*) (int*)>(0x0096f4e0)(m_currentPos);
                if (currentTile_MAYBE == 0 || IsBadReadPtr((void*)currentTile_MAYBE, sizeof(int)))
                    return;

                float renderPosX;
                float renderPosY;
                if (mapType == 2)
                {
                    renderPosX = m_currentPosX -
                                 ((float)*(int*)(currentTile_MAYBE + 0x58)) * 0x20 + X;
                    renderPosY = m_currentPosY +
                                 ((float)*(int*)(currentTile_MAYBE + 0x5c)) * 0x20 + Z;
                }
                else
                {
                    renderPosX = m_currentPosX +
                                 ((X / 10.0f + (float)(((REGIONID & 0xff) - 0x87) * 0xc0)) - (float)m_stateX) /
                                 m_stateY;
                    renderPosY = m_currentPosY +
                                 ((float)*(int*)(currentTile_MAYBE + 0x54) -
                                  ((Z / 10.0f + (float)(((REGIONID >> 8) - 0x5c) * 0xc0)) - (float)m_viewWidth) /
                                  m_viewHeight);
                }

                IDirect3DBaseTexture9* markerTexture =
                        static_cast<IDirect3DBaseTexture9*>(m_CustomDataManager->PingIcon);
                float markerU2 = 1.0f / 12.0f;
                if (markerTexture == NULL)
                {
                    markerTexture = static_cast<IDirect3DBaseTexture9*>(m_CustomDataManager->MapIcon);
                    markerU2 = 1.0f;
                }
                if (markerTexture == NULL)
                    return;

                struct MarkerVertex
                {
                    float x, y, z, rhw;
                    float u, v;
                };

                const float markerLeft = renderPosX - 15.0f;
                const float markerTop = renderPosY - 30.0f;
                const float markerRight = markerLeft + 30.0f;
                const float markerBottom = markerTop + 30.0f;
                MarkerVertex vertices[4] =
                {
                    { markerLeft,  markerTop,    0.0f, 1.0f, 0.0f,     0.0f },
                    { markerRight, markerTop,    0.0f, 1.0f, markerU2, 0.0f },
                    { markerRight, markerBottom, 0.0f, 1.0f, markerU2, 1.0f },
                    { markerLeft,  markerBottom, 0.0f, 1.0f, 0.0f,     1.0f }
                };

                g_RStateMgr.m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
                g_RStateMgr.m_pDevice->SetTexture(0, markerTexture);
                g_RStateMgr.FUN_004700a0();
                g_RStateMgr.m_pDevice->DrawPrimitiveUP(
                        D3DPT_TRIANGLEFAN, 2, vertices, sizeof(MarkerVertex));
            }

        }
    }

    if (g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1) != NULL)
    {
        CIFMenu* menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        if (menu->PingedRegionID)
        {
            if (!timerrunning) {
                timerrunning = true;
                this->StartTimer(1, 100); // Timer'ı başlat
            }

            // Harita verileri
            int REGIONID = menu->PingedRegionID;
            int X = menu->PingedPosX;

            int Z = menu->PingedPosZ;

            if(menu->MapType == 2)
            {
                if (m_currentPos != 0x0)
                {
                    int currentTile_MAYBE = reinterpret_cast<int(__thiscall*) (int*)>(0x0096f4e0)(m_currentPos);
                    if (currentTile_MAYBE == 0 || IsBadReadPtr((void*)currentTile_MAYBE, sizeof(int))) {
                        return;
                    }

                    float renderPosX = m_currentPosX - ((float)*(int*)(currentTile_MAYBE + 0x58)) * 0x20 + X;
                    float renderPosY = m_currentPosY + ((float)*(int*)(currentTile_MAYBE + 0x5c)) *  0x20 + Z;

                    D3DVECTOR dataOut[9];

                    dataOut[0].x = renderPosX - 15;
                    dataOut[0].y = renderPosY - 15;
                    dataOut[0].z = 1.0;

                    dataOut[2].x = dataOut[0].x + 36.0f;
                    dataOut[2].y = dataOut[0].y;
                    dataOut[2].z = 1.0;

                    dataOut[4].x = dataOut[0].x + 36.0f;
                    dataOut[4].y = renderPosY - 15 + 36.0f;
                    dataOut[4].z = 1.0;

                    dataOut[6].x = dataOut[0].x;
                    dataOut[6].y = dataOut[4].y;
                    dataOut[6].z = 1.0;

                    dataOut[8].z = 0.0;

                    dataOut[1].z = 0.0;
                    dataOut[3].z = 0.0;
                    dataOut[5].z = 1.0;
                    dataOut[7].z = 1.0;

                    dataOut[1].x = 0.1;
                    dataOut[3].x = 0.1;
                    dataOut[5].x = 0.1;
                    dataOut[7].x = 0.1;

                    dataOut[1].y = 0.0;
                    dataOut[3].y = 1.0;
                    dataOut[5].y = 1.0;
                    dataOut[7].y = 0.0;

                    // 432x36 boyutundaki dokunun 36x36 parçasını animasyonlu olarak render etme
                    const int textureWidth = 432;
                    const int textureHeight = 36;
                    const int subTextureWidth = 36;
                    const int subTextureHeight = 36;
                    const int numSubTextures = textureWidth / subTextureWidth; // 12 kare

                    // Kaynak dikdörtgeni (source rectangle) ayarla
                    RECT srcRect;
                    srcRect.left = currentFrame * subTextureWidth;
                    srcRect.right = srcRect.left + subTextureWidth;
                    srcRect.top = 0;
                    srcRect.bottom = subTextureHeight;

                    // Kaynak koordinatlarını ayarla
                    float u1 = static_cast<float>(srcRect.left) / textureWidth;
                    float v1 = static_cast<float>(srcRect.top) / textureHeight;
                    float u2 = static_cast<float>(srcRect.right) / textureWidth;
                    float v2 = static_cast<float>(srcRect.bottom) / textureHeight;

                    // Vertex yapısı tanımı
                    struct Vertex
                    {
                        float x, y, z, rhw; // Pozisyon verileri
                        float u, v;         // Doku koordinatları (texture coordinates)
                    };

                    Vertex vertices[4] =
                            {
                                    { dataOut[0].x, dataOut[0].y, 0, 1, u1, v1 },
                                    { dataOut[2].x, dataOut[2].y, 0, 1, u2, v1 },
                                    { dataOut[4].x, dataOut[4].y, 0, 1, u2, v2 },
                                    { dataOut[6].x, dataOut[6].y, 0, 1, u1, v2 }
                            };

                    // FVF ve doku ayarlarını yap
                    g_RStateMgr.m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
                    g_RStateMgr.m_pDevice->SetTexture(0, static_cast<IDirect3DBaseTexture9*>(m_CustomDataManager->PingIcon));

                    // Özel işlev çağrısı (sizin kodunuzda: FUN_004700a0)
                    g_RStateMgr.FUN_004700a0();

                    // Primitif çizim komutu
                    g_RStateMgr.m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(Vertex));
                }
            }
            else
            {
                if (m_currentPos != 0x0)
                {
                    int currentTile_MAYBE = reinterpret_cast<int(__thiscall*) (int*)>(0x0096f4e0)(m_currentPos);
                    if (currentTile_MAYBE == 0 || IsBadReadPtr((void*)currentTile_MAYBE, sizeof(int))) {
                        return;
                    }

                    float renderPosX = m_currentPosX + ((X / 10.0 + (float)(((REGIONID & 0xff) - 0x87) * 0xc0)) - (float)m_stateX) / m_stateY;

                    float renderPosY = m_currentPosY + ((float)*(int*)(currentTile_MAYBE + 0x54) - ((Z / 10.0 + (float)(((REGIONID >> 8) - 0x5c) * 0xc0))
                                                                                                    - (float)m_viewWidth) / m_viewHeight);

                    D3DVECTOR dataOut[9];

                    dataOut[0].x = renderPosX - 15;
                    dataOut[0].y = renderPosY - 15;
                    dataOut[0].z = 1.0;

                    dataOut[2].x = dataOut[0].x + 36.0f;
                    dataOut[2].y = dataOut[0].y;
                    dataOut[2].z = 1.0;

                    dataOut[4].x = dataOut[0].x + 36.0f;
                    dataOut[4].y = renderPosY - 15 + 36.0f;
                    dataOut[4].z = 1.0;

                    dataOut[6].x = dataOut[0].x;
                    dataOut[6].y = dataOut[4].y;
                    dataOut[6].z = 1.0;

                    dataOut[8].z = 0.0;

                    dataOut[1].z = 0.0;
                    dataOut[3].z = 0.0;
                    dataOut[5].z = 1.0;
                    dataOut[7].z = 1.0;

                    dataOut[1].x = 0.1;
                    dataOut[3].x = 0.1;
                    dataOut[5].x = 0.1;
                    dataOut[7].x = 0.1;

                    dataOut[1].y = 0.0;
                    dataOut[3].y = 1.0;
                    dataOut[5].y = 1.0;
                    dataOut[7].y = 0.0;

                    // 432x36 boyutundaki dokunun 36x36 parçasını animasyonlu olarak render etme
                    const int textureWidth = 432;
                    const int textureHeight = 36;
                    const int subTextureWidth = 36;
                    const int subTextureHeight = 36;
                    const int numSubTextures = textureWidth / subTextureWidth; // 12 kare

                    // Kaynak dikdörtgeni (source rectangle) ayarla
                    RECT srcRect;
                    srcRect.left = currentFrame * subTextureWidth;
                    srcRect.right = srcRect.left + subTextureWidth;
                    srcRect.top = 0;
                    srcRect.bottom = subTextureHeight;

                    // Kaynak koordinatlarını ayarla
                    float u1 = static_cast<float>(srcRect.left) / textureWidth;
                    float v1 = static_cast<float>(srcRect.top) / textureHeight;
                    float u2 = static_cast<float>(srcRect.right) / textureWidth;
                    float v2 = static_cast<float>(srcRect.bottom) / textureHeight;

                    // Vertex yapısı tanımı
                    struct Vertex
                    {
                        float x, y, z, rhw; // Pozisyon verileri
                        float u, v;         // Doku koordinatları (texture coordinates)
                    };

                    Vertex vertices[4] =
                            {
                                    { dataOut[0].x, dataOut[0].y, 0, 1, u1, v1 },
                                    { dataOut[2].x, dataOut[2].y, 0, 1, u2, v1 },
                                    { dataOut[4].x, dataOut[4].y, 0, 1, u2, v2 },
                                    { dataOut[6].x, dataOut[6].y, 0, 1, u1, v2 }
                            };

                    // FVF ve doku ayarlarını yap
                    g_RStateMgr.m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
                    g_RStateMgr.m_pDevice->SetTexture(0, static_cast<IDirect3DBaseTexture9*>(m_CustomDataManager->PingIcon));

                    // Özel işlev çağrısı (sizin kodunuzda: FUN_004700a0)
                    g_RStateMgr.FUN_004700a0();

                    // Primitif çizim komutu
                    g_RStateMgr.m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(Vertex));
                }
            }

        }
    }


    if (m_CustomDataManager->DimenSionalRegion.find(g_pMyPlayerObj->GetRegion().r) != m_CustomDataManager->DimenSionalRegion.end())
        {

            //Rebot::Config.cercle = -60;
            float x1 = m_CustomDataManager->x1 + m_CustomDataManager->cercle + *((float *)((int)this + 0x95cc)); // kéo dãn bên trái
            float x2 = m_CustomDataManager->x2 - m_CustomDataManager->cercle + *((float *)((int)this + 0x95e4)); // kéo dãn bên phải
            float y1 = m_CustomDataManager->y1 + m_CustomDataManager->cercle + *((float *)((int)this + 0x95d0)); // kéo dãn bên trên
            float y2 = m_CustomDataManager->y2 - m_CustomDataManager->cercle + *((float *)((int)this + 0x9600)); //kéo dãn bên dưới

            D3DVECTOR local_6c[9];
            local_6c[0].z = 1.0;
            local_6c[2].z = 1.0;
            local_6c[4].z = 1.0;
            local_6c[6].z = 1.0;
            local_6c[8].z = 0.0;

            local_6c[1].z = 0.0;
            local_6c[3].z = 0.0;
            local_6c[5].z = 1.0;
            local_6c[7].z = 1.0;

            local_6c[1].x = 0.1;
            local_6c[3].x = 0.1;
            local_6c[5].x = 0.1;
            local_6c[7].x = 0.1;

            local_6c[1].y = 0.0;
            local_6c[3].y = 1.0;
            local_6c[5].y = 1.0;
            local_6c[7].y = 0.0;


            local_6c[0].x = x1;
            local_6c[0].y = y1;
            local_6c[2].x = x2;
            local_6c[2].y = y1;
            local_6c[4].x = x2;
            local_6c[4].y = y2;
            local_6c[6].x = x1;
            local_6c[6].y = y2;



            SYSTEMTIME time;
            GetSystemTime(&time);
            int ms = time.wMilliseconds;
            int loop = ms/200;

            std::ostringstream temp;
            int img = loop + 1;
            temp << img;
            std::string imgpath = "interface\\royale\\circle_" + temp.str() + ".ddj";
            std::wstring imgpaths = TO_WSTRING(imgpath).c_str();
            const IDirect3DBaseTexture9* puVarxx = Fun_CacheTexture_Create(TO_NSTRING(imgpaths));
            g_RStateMgr.SetTextureForStage(0, puVarxx);
            g_RStateMgr.SetDeviceFVFState(0x104);
            int local_159 = 0;
            //if (g_pDynamicVertexBuffer->IVBDynamic_Func_6((float *)((int)this + 0x95cc), 0x60, &local_159) != 0) {
            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(local_6c, 0x60, &local_159) != 0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_159, 2);
            }

        /*    CNIFStatic *notshow1 = GetResObj<CNIFStatic>(26);
            CNIFStatic *notshow3 = GetResObj<CNIFStatic>(13);
            CNIFButton *notshow4 = GetResObj<CNIFButton>(7);
            CNIFButton *notshow5 = GetResObj<CNIFButton>(6);
            CNIFButton *notshow6 = GetResObj<CNIFButton>(15);
            CNIFStatic *notshow7 = GetResObj<CNIFStatic>(30);
            CNIFWnd *notshow8 = GetResObj<CNIFWnd>(31);

            notshow1->ShowGWnd(false);
            notshow3->ShowGWnd(false);
            notshow4->ShowGWnd(false);
            notshow5->ShowGWnd(false);
            notshow6->ShowGWnd(false);
            notshow7->ShowGWnd(false);
            notshow8->ShowGWnd(false);*/
        }


}
void CNIFWorldMap::OnTimerIMPL(int timerId) {
    if (timerrunning) {
        this->KillTimer(timerId);
        timerrunning = false;
        UpdateAnimation(); // Animasyonu güncelle
    }
    reinterpret_cast<void *(__thiscall *)(CNIFWorldMap *, int)>(0x006191d0)(this, timerId);
}

#include <cmath> // cmath kütüphanesini ekle

bool CNIFWorldMap::OnMouseActions(Event3D* mouseData) {
    bool result = reinterpret_cast<bool(__thiscall *)(CNIFWorldMap *, Event3D*)>(0x00623220)(this, mouseData);
    if (mouseData->Msg == WM_LBUTTONUP) {
        g_Controler->SetCustomCursor(149);
        CIFMenu *menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
        if(menu->CanSendPing)
        {
            menu->CanSendPing = false;
            if (m_currentPos != 0x0 && m_currentPos != NULL) {
                int currentTile_MAYBE = reinterpret_cast<int (__thiscall *)(int *)>(0x0096f4e0)(m_currentPos);
                if (currentTile_MAYBE == 0 || IsBadReadPtr((void *)currentTile_MAYBE, sizeof(int))) {
                    return false;
                }

                // Mouse tıklama pozisyonlarını al
                float clickPosX = mouseData->lParam;
                float clickPosY = mouseData->wParam;
                //std::cout << "Mouse Position: " << clickPosX << ", " << clickPosY << std::endl;

                // Harita pozisyonlarını hesapla
                float mapPosX = ((clickPosX - m_currentPosX) * m_stateY) + static_cast<float>(m_stateX);
                float mapPosY = ((-clickPosY + m_currentPosY + static_cast<float>(*(int *)(currentTile_MAYBE + 0x54))) * m_viewHeight) + static_cast<float>(m_viewWidth);
                //std::cout << "Calculated mapPosX: " << mapPosX << ", mapPosY: " << mapPosY << std::endl;

                // X ve Y sektörlerini hesapla
                int xSector = static_cast<int>(std::floor((mapPosX / 192.0) + 135.0));
                int ySector = static_cast<int>(std::floor((mapPosY / 192.0) + 92.0));

                // X ve Y ofsetlerini hesapla
                int xOffset = static_cast<int>(std::floor((((mapPosX / 192.0) - (xSector - 135.0)) * 192.0) * 10.0));
                int yOffset = static_cast<int>(std::floor((((mapPosY / 192.0) - (ySector - 92.0)) * 192.0) * 10.0));

                int pingedRegionID = (ySector << 8) | xSector;

                bool isDungeon = (pingedRegionID > SHRT_MAX);

             /*   menu->PingedRegionID = pingedRegionID;
                menu->PingedPosX = xOffset;
                menu->PingedPosY = yOffset;
                menu->PingedPosZ = yOffset;
*/
                const SPartyData& partyData = g_CCharacterDependentData.GetPartyData();
                CMsgStreamBuffer buf(0xB299);
                buf << pingedRegionID;
                buf << xOffset;
                buf << yOffset;
                buf << yOffset;
                buf << Is0OutSide2IsInside;
                buf << Normal0Town1Dungeon2;

                buf << (BYTE)(partyData.NumberOfMembers);
                for (int i = 0; i < partyData.NumberOfMembers; ++i)
                {
                    const SPartyMemberData& memberData = g_CCharacterDependentData.GetPartyMemberData(i);
                    buf << std::n_string(TO_NSTRING(memberData.m_charactername));
                }
                SendMsg(buf);

            }
        }
    }
    return result;
}





undefined1 CNIFWorldMap::OnCloseWndIMPL(){
    CIFMenu *menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
    if(menu->CanSendPing)
    {
        menu->CanSendPing = false;
        g_Controler->SetCustomCursor(149);
    }
    return reinterpret_cast<undefined1 (__thiscall *)(CNIFWorldMap *)>(0x0046d7c0)(this);
}
