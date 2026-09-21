//
// Created by Kurama on 2/24/2023.
//

#include "NIFTileWnd.h"

#include "GFX3DFunction/RTLoading.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CNIFTileWnd, 0x0115df48)

CNIFTileWnd::~CNIFTileWnd() {
    ReleaseTextures();
}

bool CNIFTileWnd::OnCreate(long ln) {
    InitializeFrameTextures();
    field_0348 = false;

    return true;
}

void CNIFTileWnd::InitializeFrameTextures() {
    // .....
#if 0
    for (int i = 4; i != 0; i--) {
        m_pCornersTexture[i] = NULL;
        m_pEdgesTexture[i] = NULL;

        m_strCornersTexture[i] = "";
        m_strEdgesTexture[i] = "";

        m_sCornersSize[i].height = 0;
        m_sCornersSize[i].width = 0;
        m_sEdgesSize[i].height = 0;
        m_sEdgesSize[i].width = 0;

        for (int j = 4, uv = 1, p = 0; j != 0; j--, uv += 2, p += 2) {
            m_vecCornersRenderBuff[j][uv].y = 0.1f;
            m_vecEdgesRenderBuff[j][uv].y = 0.1f;
            m_vecCornersRenderBuff[j][uv].x = 1.0f;
            m_vecEdgesRenderBuff[j][uv].x = 1.0f;

            m_vecCornersRenderBuff[j][p].y = 0.0f;
            m_vecCornersRenderBuff[j][p].z = 0.0f;
            m_vecEdgesRenderBuff[j][p].y = 0.0f;
            m_vecEdgesRenderBuff[j][p].z = 0.0f;
        }

        // ....
    }
#endif // if 0
}

void CNIFTileWnd::ReleaseTextures() {
    for (int i = 4; i != 0; i--) {
        m_pCornersTexture[i] = NULL;
        m_pEdgesTexture[i] = NULL;

        if (!m_strCornersTexture[i].empty()) {
            Fun_CacheTexture_Release(&m_strCornersTexture[i]);
            m_strCornersTexture[i] = "";
        }

        if (!m_strEdgesTexture[i].empty()) {
            Fun_CacheTexture_Release(&m_strEdgesTexture[i]);
            m_strEdgesTexture[i] = "";
        }
    }
}

void CNIFTileWnd::OptimizeEdgesVerticesPos() {
    // Y,Z is X,Y for the Vertices
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][1].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][1].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][3].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][3].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][5].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][5].z = 1.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][7].z = 1.0f;
    m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][7].y = 0.0f;

    m_vecEdgesRenderBuff[EDGE_MID_UP][1].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][1].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][3].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][3].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][5].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][5].z = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][7].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_UP][7].z = 1.0f;

    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][1].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][1].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][3].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][3].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].z = 1.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].z = 1.0f;

    m_vecEdgesRenderBuff[EDGE_MID_DOWN][1].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][1].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].z = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].y = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].z = 1.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][7].y = 0.0f;
    m_vecEdgesRenderBuff[EDGE_MID_DOWN][7].z = 1.0f;
}

void CNIFTileWnd::OptimizeCornersVerticesPos() {
    // Y,Z is X,Y for the Vertices
    m_vecCornersRenderBuff[CORNER_LEFT_UP][1].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][1].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][3].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][3].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][5].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][5].z = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][7].z = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][7].y = 0.0f;

    m_vecCornersRenderBuff[CORNER_RIGHT_UP][1].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][1].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][3].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][3].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][5].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][5].z = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][7].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][7].z = 1.0f;

    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][1].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][1].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][3].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][3].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][5].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][5].z = 1.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][7].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][7].z = 1.0f;

    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][1].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][1].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][3].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][3].z = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][5].y = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][5].z = 1.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][7].y = 0.0f;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][7].z = 1.0f;
}

void CNIFTileWnd::UpdateTexturesCoordinates() {
    CNIFWnd::UpdateTexturesCoordinates();

    wnd_rect rcBounds = GetBounds();
    float fWndPosX = (float) rcBounds.pos.x - 0.5f;
    float fWndWidth = (float) (rcBounds.size.width + rcBounds.pos.x) - 0.5f;
    float fWndPosY = (float) rcBounds.pos.y - 0.5f;
    float fWndHeight = (float) (rcBounds.size.height + rcBounds.pos.y) - 0.5f;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][0].x = fWndPosX;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][0].y = fWndPosY;
    float fNextPos = (float) m_sCornersSize[CORNER_LEFT_UP].width + fWndPosX;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][2].x = fNextPos;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][4].x = fNextPos;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][2].y = fWndPosY;
    fNextPos = (float) m_sCornersSize[CORNER_LEFT_UP].height + fWndPosY;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][4].y = fNextPos;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][6].y = fNextPos;
    m_vecCornersRenderBuff[CORNER_LEFT_UP][6].x = fWndPosX;
    fNextPos = fWndWidth - (float) m_sCornersSize[CORNER_RIGHT_UP].width;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][0].x = fNextPos;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][0].y = fWndPosY;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][2].y = fWndPosY;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][2].x = fWndWidth;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][4].x = fWndWidth;
    fWndPosY = (float) m_sCornersSize[CORNER_RIGHT_UP].height + fWndPosY;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][4].y = fWndPosY;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][6].y = fWndPosY;
    m_vecCornersRenderBuff[CORNER_RIGHT_UP][6].x = fNextPos;
    fWndPosY = fWndWidth - (float) m_sCornersSize[CORNER_RIGHT_DOWN].width;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][0].x = fWndPosY;
    fNextPos = fWndHeight - (float) m_sCornersSize[CORNER_RIGHT_DOWN].height;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][0].y = fNextPos;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][2].y = fNextPos;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][2].x = fWndWidth;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][4].x = fWndWidth;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][4].y = fWndHeight;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][6].y = fWndHeight;
    m_vecCornersRenderBuff[CORNER_RIGHT_DOWN][6].x = fWndPosY;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][0].x = fWndPosX;
    fWndPosY = fWndHeight - (float) m_sCornersSize[CORNER_LEFT_DOWN].height;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][0].y = fWndPosY;
    fWndWidth = (float) m_sCornersSize[CORNER_LEFT_DOWN].width + fWndPosX;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][2].x = fWndWidth;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][4].x = fWndWidth;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][2].y = fWndPosY;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][4].y = fWndHeight;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][6].y = fWndHeight;
    m_vecCornersRenderBuff[CORNER_LEFT_DOWN][6].x = fWndPosX;
}

void CNIFTileWnd::SetFrameTexture(std::n_string strTexture) {
    std::n_string strCoords[4] = {"",
                                  "",
                                  "",
                                  ""};
    SetCornersTexture(strCoords);

    strCoords[EDGE_LEFT_SIDE] = strTexture;
    strCoords[EDGE_MID_UP] = strTexture;
    strCoords[EDGE_RIGHT_SIDE] = strTexture;
    strCoords[EDGE_MID_DOWN] = strTexture;
    SetEdgesTexture(strCoords);

    OptimizeCornersVerticesPos();
    OptimizeEdgesVerticesPos();
}

void CNIFTileWnd::SetCornersTexture(std::n_string strCorners[4]) {
    OptimizeCornersVerticesPos();

    for (int i = 4; i != 0; i--) {
        m_pCornersTexture[i] = NULL;

        if (!m_strCornersTexture[i].empty()) {
            Fun_CacheTexture_Release(&m_strCornersTexture[i]);
            m_strCornersTexture[i] = "";
        }
    }

    if (!strCorners[CORNER_LEFT_UP].empty())
        m_pCornersTexture[CORNER_LEFT_UP] = Fun_CacheTexture_Create(strCorners[CORNER_LEFT_UP]);
    if (!strCorners[CORNER_RIGHT_UP].empty())
        m_pCornersTexture[CORNER_RIGHT_UP] = Fun_CacheTexture_Create(strCorners[CORNER_RIGHT_UP]);
    if (!strCorners[CORNER_RIGHT_DOWN].empty())
        m_pCornersTexture[CORNER_RIGHT_DOWN] = Fun_CacheTexture_Create(strCorners[CORNER_RIGHT_DOWN]);
    if (!strCorners[CORNER_LEFT_DOWN].empty())
        m_pCornersTexture[CORNER_LEFT_DOWN] = Fun_CacheTexture_Create(strCorners[CORNER_LEFT_DOWN]);

    D3DSURFACE_DESC d3dsTexture;
    if (m_pCornersTexture[CORNER_LEFT_UP] != NULL) {
        m_strCornersTexture[CORNER_LEFT_UP] = strCorners[CORNER_LEFT_UP];
        ((IDirect3DTexture9 *) m_pCornersTexture[CORNER_LEFT_UP])->GetLevelDesc(0, &d3dsTexture);
        m_sCornersSize[CORNER_LEFT_UP].width = d3dsTexture.Width;
        m_sCornersSize[CORNER_LEFT_UP].height = d3dsTexture.Height;
        m_strCornersTexture[CORNER_LEFT_UP] = strCorners[CORNER_LEFT_UP];
    }
    if (m_pCornersTexture[CORNER_RIGHT_UP] != NULL) {
        m_strCornersTexture[CORNER_RIGHT_UP] = strCorners[CORNER_RIGHT_UP];
        ((IDirect3DTexture9 *) m_pCornersTexture[CORNER_RIGHT_UP])->GetLevelDesc(0, &d3dsTexture);
        m_sCornersSize[CORNER_RIGHT_UP].width = d3dsTexture.Width;
        m_sCornersSize[CORNER_RIGHT_UP].height = d3dsTexture.Height;
        m_strCornersTexture[CORNER_RIGHT_UP] = strCorners[CORNER_RIGHT_UP];
    }
    if (m_pCornersTexture[CORNER_RIGHT_DOWN] != NULL) {
        m_strCornersTexture[CORNER_RIGHT_DOWN] = strCorners[CORNER_RIGHT_DOWN];
        ((IDirect3DTexture9 *) m_pCornersTexture[CORNER_RIGHT_DOWN])->GetLevelDesc(0, &d3dsTexture);
        m_sCornersSize[CORNER_RIGHT_DOWN].width = d3dsTexture.Width;
        m_sCornersSize[CORNER_RIGHT_DOWN].height = d3dsTexture.Height;
        m_strCornersTexture[CORNER_RIGHT_DOWN] = strCorners[CORNER_RIGHT_DOWN];
    }
    if (m_pCornersTexture[CORNER_LEFT_DOWN] != NULL) {
        m_strCornersTexture[CORNER_LEFT_DOWN] = strCorners[CORNER_LEFT_DOWN];
        ((IDirect3DTexture9 *) m_pCornersTexture[CORNER_LEFT_DOWN])->GetLevelDesc(0, &d3dsTexture);
        m_sCornersSize[CORNER_LEFT_DOWN].width = d3dsTexture.Width;
        m_sCornersSize[CORNER_LEFT_DOWN].height = d3dsTexture.Height;
        m_strCornersTexture[CORNER_LEFT_DOWN] = strCorners[CORNER_LEFT_DOWN];
    }
}

void CNIFTileWnd::SetEdgesTexture(std::n_string strEdges[4]) {
    OptimizeEdgesVerticesPos();

    // lets clear first
    for (int i = 4; i != 0; i--) {
        m_pEdgesTexture[i] = NULL;

        if (!m_strEdgesTexture[i].empty()) {
            Fun_CacheTexture_Release(&m_strEdgesTexture[i]);
            m_strEdgesTexture[i] = "";
        }
    }

    if (!strEdges[EDGE_LEFT_SIDE].empty())
        m_pEdgesTexture[EDGE_LEFT_SIDE] = Fun_CacheTexture_Create(strEdges[EDGE_LEFT_SIDE]);
    if (!strEdges[EDGE_MID_UP].empty())
        m_pEdgesTexture[EDGE_MID_UP] = Fun_CacheTexture_Create(strEdges[EDGE_MID_UP]);
    if (!strEdges[EDGE_RIGHT_SIDE].empty())
        m_pEdgesTexture[EDGE_RIGHT_SIDE] = Fun_CacheTexture_Create(strEdges[EDGE_RIGHT_SIDE]);
    if (!strEdges[EDGE_MID_DOWN].empty())
        m_pEdgesTexture[EDGE_MID_DOWN] = Fun_CacheTexture_Create(strEdges[EDGE_MID_DOWN]);

    D3DSURFACE_DESC d3dsTexture;
    if (m_pEdgesTexture[EDGE_LEFT_SIDE] != NULL) {
        m_strEdgesTexture[EDGE_LEFT_SIDE] = strEdges[EDGE_LEFT_SIDE];
        ((IDirect3DTexture9 *) m_pEdgesTexture[EDGE_LEFT_SIDE])->GetLevelDesc(0, &d3dsTexture);
        m_sEdgesSize[EDGE_LEFT_SIDE].width = d3dsTexture.Width;
        m_sEdgesSize[EDGE_LEFT_SIDE].height = d3dsTexture.Height;
        m_strEdgesTexture[EDGE_LEFT_SIDE] = strEdges[EDGE_LEFT_SIDE];
    }
    if (m_pEdgesTexture[EDGE_MID_UP] != NULL) {
        m_strEdgesTexture[EDGE_MID_UP] = strEdges[EDGE_MID_UP];
        ((IDirect3DTexture9 *) m_pEdgesTexture[EDGE_MID_UP])->GetLevelDesc(0, &d3dsTexture);
        m_sEdgesSize[EDGE_MID_UP].width = d3dsTexture.Width;
        m_sEdgesSize[EDGE_MID_UP].height = d3dsTexture.Height;
        m_strEdgesTexture[EDGE_MID_UP] = strEdges[EDGE_MID_UP];
    }
    if (m_pEdgesTexture[EDGE_RIGHT_SIDE] != NULL) {
        m_strEdgesTexture[EDGE_RIGHT_SIDE] = strEdges[EDGE_RIGHT_SIDE];
        ((IDirect3DTexture9 *) m_pEdgesTexture[EDGE_RIGHT_SIDE])->GetLevelDesc(0, &d3dsTexture);
        m_sEdgesSize[EDGE_RIGHT_SIDE].width = d3dsTexture.Width;
        m_sEdgesSize[EDGE_RIGHT_SIDE].height = d3dsTexture.Height;
        m_strEdgesTexture[EDGE_RIGHT_SIDE] = strEdges[EDGE_RIGHT_SIDE];
    }
    if (m_pEdgesTexture[EDGE_MID_DOWN] != NULL) {
        m_strEdgesTexture[EDGE_MID_DOWN] = strEdges[EDGE_MID_DOWN];
        ((IDirect3DTexture9 *) m_pEdgesTexture[EDGE_MID_DOWN])->GetLevelDesc(0, &d3dsTexture);
        m_sEdgesSize[EDGE_MID_DOWN].width = d3dsTexture.Width;
        m_sEdgesSize[EDGE_MID_DOWN].height = d3dsTexture.Height;
        m_strEdgesTexture[EDGE_MID_DOWN] = strEdges[EDGE_MID_DOWN];
    }
}

void CNIFTileWnd::NTB_Func_13(std::n_string str, int a3, int a4) {
    SetFrameTexture(str);
}

void CNIFTileWnd::Func_40() {
    UpdateTexturesCoordinates();
}
