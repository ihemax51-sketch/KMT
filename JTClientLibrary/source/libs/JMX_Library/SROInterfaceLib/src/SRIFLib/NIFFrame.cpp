//
// Created by Kurama on 2/25/2023.
//

#include "NIFFrame.h"

#include "GFX3DFunction/RStateMgr.h"
#include "SimpleViewer/VBDynamic.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CNIFFrame, 0x0115df28)

bool CNIFFrame::OnCreate(long ln) {
    return reinterpret_cast<bool (__thiscall *)(CNIFFrame *, long)>(0x00473670)(this, ln);

    // InitializeFrameTextures is messed up
#if 0
    // Its getting cal but as inline,
    CNIFTileWnd::OnCreate(ln);

    InitializeFrameTextures();
    field_0348 = false;

    return true;
#endif // if 0
}

void CNIFFrame::RenderMyself() {
    reinterpret_cast<void (__thiscall *)(CNIFFrame *)>(0x004736a0)(this);

    // there is a something wrong with the edges so we will use the orignal one
#if 0
    EnableTextureAlphaOperations(D3DCOLOR_RGBA(0, 0, 0, 0));

    g_RStateMgr.SetDeviceFVFState(0x104);

    // Corners...
    int local_3c = 0;
    g_RStateMgr.SetTextureForStage(0, m_pCornersTexture[CORNER_LEFT_UP]);
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecCornersRenderBuff[CORNER_LEFT_UP],
                                                  sizeof(m_vecCornersRenderBuff[CORNER_LEFT_UP]), &local_3c) !=
        0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_3c, 2);
    }

    g_RStateMgr.SetTextureForStage(0, m_pCornersTexture[CORNER_RIGHT_UP]);
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecCornersRenderBuff[CORNER_RIGHT_UP],
                                                  sizeof(m_vecCornersRenderBuff[CORNER_RIGHT_UP]), &local_3c) !=
        0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_3c, 2);
    }

    g_RStateMgr.SetTextureForStage(0, m_pCornersTexture[CORNER_RIGHT_DOWN]);
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecCornersRenderBuff[CORNER_RIGHT_DOWN],
                                                  sizeof(m_vecCornersRenderBuff[CORNER_RIGHT_DOWN]), &local_3c) !=
        0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_3c, 2);
    }

    g_RStateMgr.SetTextureForStage(0, m_pCornersTexture[CORNER_LEFT_DOWN]);
    if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecCornersRenderBuff[CORNER_LEFT_DOWN],
                                                  sizeof(m_vecCornersRenderBuff[CORNER_LEFT_DOWN]), &local_3c) !=
        0) {
        IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
        g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
        g_RStateMgr.FUN_00470060(6, local_3c, 2);
    }

    wnd_rect rcBounds = GetBounds();
    float fRenderX = (float) rcBounds.pos.x - 0.5f;
    float fRenderWidth = (float) (rcBounds.size.width + rcBounds.pos.x) - 0.5f;
    float fRenderY = (float) rcBounds.pos.y - 0.5f;
    float fRenderHeight = (float) (rcBounds.size.height + rcBounds.pos.y) - 0.5f;

    if (m_pEdgesTexture[EDGE_MID_UP] != NULL) {
        int nEdgeBoundWidth =
                (rcBounds.size.width - m_sCornersSize[CORNER_RIGHT_UP].width) - m_sCornersSize[CORNER_LEFT_UP].width;

        // if the are same same pixels size?
        if ((nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) != 0) {
            // Use the full texture
            m_vecEdgesRenderBuff[EDGE_MID_UP][3].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_MID_UP][3].z = 0.0f;
            m_vecEdgesRenderBuff[EDGE_MID_UP][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_MID_UP][5].z = 1.0f;
        }

        g_RStateMgr.SetTextureForStage(0, m_pEdgesTexture[EDGE_MID_UP]);

        int cEdges = 0;
        float fEdgeRenderY = ((float) m_sEdgesSize[EDGE_MID_UP].height + fRenderY);

        // if we have the image pixel fucked up
        if ((nEdgeBoundWidth / m_sEdgesSize[EDGE_MID_UP].width) > 0) {
            // we will create multi texture to get it as fixed pixel
            for (; cEdges < (nEdgeBoundWidth / m_sEdgesSize[EDGE_MID_UP].width); cEdges++) {
                int nNextEdgePosX = (m_sEdgesSize[EDGE_MID_UP].width * cEdges);
                float fFirstMidUpEdgeRenderX = ((float) m_sCornersSize[CORNER_LEFT_UP].width + fRenderX);
                float fNextEdgeRenderX = ((float) nNextEdgePosX + fFirstMidUpEdgeRenderX);

                m_vecEdgesRenderBuff[EDGE_MID_UP][0].x = fNextEdgeRenderX;
                m_vecEdgesRenderBuff[EDGE_MID_UP][0].y = fRenderY;

                m_vecEdgesRenderBuff[EDGE_MID_UP][2].x = fNextEdgeRenderX;
                m_vecEdgesRenderBuff[EDGE_MID_UP][4].x = fNextEdgeRenderX;

                m_vecEdgesRenderBuff[EDGE_MID_UP][2].y = fRenderY;

                m_vecEdgesRenderBuff[EDGE_MID_UP][4].y = fEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_MID_UP][6].y = fEdgeRenderY;
                m_vecEdgesRenderBuff[EDGE_MID_UP][6].x = fNextEdgeRenderX;

                if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_MID_UP],
                                                              sizeof(m_vecEdgesRenderBuff[EDGE_MID_UP]), &local_3c) !=
                    0) {
                    IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                    g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                    g_RStateMgr.FUN_00470060(6, local_3c, 2);
                }
            }
        }

        // we will create the last edge texture
        if ((nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) != 0) {
            m_vecEdgesRenderBuff[EDGE_MID_UP][3].y =
                    ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) /
                     (float) m_sEdgesSize[EDGE_MID_UP].width);
            m_vecEdgesRenderBuff[EDGE_MID_UP][3].z = 0.0f;

            m_vecEdgesRenderBuff[EDGE_MID_UP][5].y =
                    ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) /
                     (float) m_sEdgesSize[EDGE_MID_UP].width);
            m_vecEdgesRenderBuff[EDGE_MID_UP][5].z = 1.0f;

            float fRenderLastEdgeX = (((float) m_sCornersSize[CORNER_LEFT_UP].width + fRenderX) +
                                      ((float) (m_sEdgesSize[EDGE_MID_UP].width * cEdges)));
            m_vecEdgesRenderBuff[EDGE_MID_UP][0].x = fRenderLastEdgeX;
            m_vecEdgesRenderBuff[EDGE_MID_UP][0].y = fRenderY;

            m_vecEdgesRenderBuff[EDGE_MID_UP][2].x = ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) +
                                                      fRenderLastEdgeX);
            m_vecEdgesRenderBuff[EDGE_MID_UP][4].x = ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_UP].width) +
                                                      fRenderLastEdgeX);
            m_vecEdgesRenderBuff[EDGE_MID_UP][2].y = fRenderY;
            m_vecEdgesRenderBuff[EDGE_MID_UP][4].y = fEdgeRenderY;

            m_vecEdgesRenderBuff[EDGE_MID_UP][6].y = fEdgeRenderY;
            m_vecEdgesRenderBuff[EDGE_MID_UP][6].x = fRenderLastEdgeX;

            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_MID_UP],
                                                          sizeof(m_vecEdgesRenderBuff[EDGE_MID_UP]), &local_3c) !=
                0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_3c, 2);
            }
        }
    }

    if (m_pEdgesTexture[EDGE_MID_DOWN] != NULL) {
        int nEdgeBoundWidth =
                (rcBounds.size.width - m_sCornersSize[CORNER_RIGHT_DOWN].width) -
                m_sCornersSize[CORNER_LEFT_DOWN].width;

        // if the are same same pixels size?
        if ((nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) != 0) {
            // Use the full texture
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].z = 0.0f;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].z = 1.0f;
        }

        g_RStateMgr.SetTextureForStage(0, m_pEdgesTexture[EDGE_MID_DOWN]);

        int cEdges = 0;
        float fEdgeRenderY = (fRenderHeight - (float) m_sEdgesSize[EDGE_MID_DOWN].height);

        // if we have the image pixel fucked up
        if ((nEdgeBoundWidth / m_sEdgesSize[EDGE_MID_DOWN].width) > 0) {
            // we will create multi texture to get it as fixed pixel
            for (; cEdges < (nEdgeBoundWidth / m_sEdgesSize[EDGE_MID_DOWN].width); cEdges++) {
                int nNextEdgePosX = (m_sEdgesSize[EDGE_MID_DOWN].width * cEdges);
                float fFirstMidDownEdgeRenderX = ((float) m_sCornersSize[CORNER_LEFT_DOWN].width + fRenderX);
                float fNextEdgeRenderX = ((float) nNextEdgePosX + fFirstMidDownEdgeRenderX);

                m_vecEdgesRenderBuff[EDGE_MID_DOWN][0].x = fNextEdgeRenderX;
                m_vecEdgesRenderBuff[EDGE_MID_DOWN][0].y = fEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_MID_DOWN][2].x = fNextEdgeRenderX;
                m_vecEdgesRenderBuff[EDGE_MID_DOWN][4].x = fNextEdgeRenderX;

                m_vecEdgesRenderBuff[EDGE_MID_DOWN][2].y = fEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_MID_DOWN][4].y = fRenderHeight;

                m_vecEdgesRenderBuff[EDGE_MID_DOWN][6].y = fRenderHeight;
                m_vecEdgesRenderBuff[EDGE_MID_DOWN][6].x = fNextEdgeRenderX;

                if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_MID_DOWN],
                                                              sizeof(m_vecEdgesRenderBuff[EDGE_MID_DOWN]), &local_3c) !=
                    0) {
                    IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                    g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                    g_RStateMgr.FUN_00470060(6, local_3c, 2);
                }
            }
        }

        // we will create the last edge texture
        if ((nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) != 0) {
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].y =
                    ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) /
                     (float) m_sEdgesSize[EDGE_MID_DOWN].width);
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][3].z = 0.0f;

            m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].y =
                    ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) /
                     (float) m_sEdgesSize[EDGE_MID_DOWN].width);
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][5].z = 1.0f;

            float fRenderLastEdgeX = (((float) m_sCornersSize[CORNER_LEFT_DOWN].width + fRenderX) +
                                      ((float) (m_sEdgesSize[EDGE_MID_DOWN].width * cEdges)));
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][0].x = fRenderLastEdgeX;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][0].y = fEdgeRenderY;

            m_vecEdgesRenderBuff[EDGE_MID_DOWN][2].x = ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) +
                                                        fRenderLastEdgeX);
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][4].x = ((float) (nEdgeBoundWidth % m_sEdgesSize[EDGE_MID_DOWN].width) +
                                                        fRenderLastEdgeX);
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][2].y = fEdgeRenderY;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][4].y = fRenderHeight;

            m_vecEdgesRenderBuff[EDGE_MID_DOWN][6].y = fRenderHeight;
            m_vecEdgesRenderBuff[EDGE_MID_DOWN][6].x = fRenderLastEdgeX;

            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_MID_DOWN],
                                                          sizeof(m_vecEdgesRenderBuff[EDGE_MID_DOWN]), &local_3c) !=
                0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_3c, 2);
            }
        }
    }

    if (m_pEdgesTexture[EDGE_LEFT_SIDE] != NULL) {
        int nEdgeBoundSize =
                (rcBounds.size.height - m_sCornersSize[CORNER_LEFT_DOWN].height) -
                m_sCornersSize[CORNER_LEFT_UP].height;

        // if the are same same pixels size?
        if ((nEdgeBoundSize % m_sEdgesSize[EDGE_LEFT_SIDE].height) != 0) {
            // Use the full texture
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].z = 1.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].y = 0.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].z = 1.0f;
        }

        g_RStateMgr.SetTextureForStage(0, m_pEdgesTexture[EDGE_LEFT_SIDE]);

        int cEdges = 0;

        // if we have the image pixel fucked up
        if ((nEdgeBoundSize / m_sEdgesSize[EDGE_LEFT_SIDE].height) > 0) {
            // we will create multi texture to get it as fixed pixel
            for (; cEdges < (nEdgeBoundSize / m_sEdgesSize[EDGE_LEFT_SIDE].height); cEdges++) {
                int nNextEdgePosY = (m_sEdgesSize[EDGE_LEFT_SIDE].height * cEdges);
                float fFirstLeftSideEdgeRenderY = ((float) m_sCornersSize[CORNER_LEFT_UP].height + fRenderY);
                float fNextEdgeRenderY = ((float) nNextEdgePosY + fFirstLeftSideEdgeRenderY);

                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][0].x = fRenderX;
                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][0].y = fNextEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][2].x = fRenderHeight;
                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][4].x = fRenderHeight;

                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][2].y = fNextEdgeRenderY;

                float fNextNextLeftSideEdgeRenderY = fFirstLeftSideEdgeRenderY + (float) nNextEdgePosY;
                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][4].y = fNextNextLeftSideEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][6].y = fNextNextLeftSideEdgeRenderY;
                m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][6].x = fRenderX;

                if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_LEFT_SIDE],
                                                              sizeof(m_vecEdgesRenderBuff[EDGE_LEFT_SIDE]),
                                                              &local_3c) !=
                    0) {
                    IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                    g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                    g_RStateMgr.FUN_00470060(6, local_3c, 2);
                }
            }
        }

        // we will create the last edge texture
        if ((nEdgeBoundSize % m_sEdgesSize[EDGE_LEFT_SIDE].height) != 0) {
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][5].z = (
                    (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_LEFT_SIDE].height) /
                    (float) m_sEdgesSize[EDGE_LEFT_SIDE].height);

            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][7].y = 0.0f;
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][7].z = (
                    (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_LEFT_SIDE].height) /
                    (float) m_sEdgesSize[EDGE_LEFT_SIDE].height);

            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][0].x = fRenderX;
            float fRenderLastEdgeY = (((float) m_sCornersSize[CORNER_LEFT_UP].height + fRenderY) +
                                      ((float) (m_sEdgesSize[EDGE_LEFT_SIDE].height * cEdges)));
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][0].y = fRenderLastEdgeY;

            float fRenderLastEdgeX = (float) m_sCornersSize[CORNER_LEFT_UP].width + fRenderX;
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][2].x = fRenderLastEdgeX;
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][4].x = fRenderLastEdgeX;

            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][2].y = fRenderLastEdgeY;

            float fRenderLastEdgeYPerv =
                    fRenderLastEdgeY + (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_LEFT_SIDE].height);
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][4].y = fRenderLastEdgeYPerv;

            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][6].y = fRenderLastEdgeYPerv;
            m_vecEdgesRenderBuff[EDGE_LEFT_SIDE][6].x = fRenderX;

            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_LEFT_SIDE],
                                                          sizeof(m_vecEdgesRenderBuff[EDGE_LEFT_SIDE]), &local_3c) !=
                0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_3c, 2);
            }
        }
    }

    if (m_pEdgesTexture[EDGE_RIGHT_SIDE] != NULL) {
        int nEdgeBoundSize =
                (rcBounds.size.height - m_sCornersSize[CORNER_RIGHT_DOWN].height) -
                m_sCornersSize[CORNER_RIGHT_UP].height;

        // if the are same same pixels size?
        if ((nEdgeBoundSize % m_sEdgesSize[EDGE_RIGHT_SIDE].height) != 0) {
            // Use the full texture
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].z = 1.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].y = 0.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].z = 1.0f;
        }

        g_RStateMgr.SetTextureForStage(0, m_pEdgesTexture[EDGE_RIGHT_SIDE]);

        int cEdges = 0;
        float fEdgeRenderX = (fRenderWidth - (float) m_sEdgesSize[EDGE_RIGHT_SIDE].width);

        // if we have the image pixel fucked up
        if ((nEdgeBoundSize / m_sEdgesSize[EDGE_RIGHT_SIDE].height) > 0) {
            // we will create multi texture to get it as fixed pixel
            for (; cEdges < (nEdgeBoundSize / m_sEdgesSize[EDGE_RIGHT_SIDE].height); cEdges++) {
                int nNextEdgePosY = (m_sEdgesSize[EDGE_RIGHT_SIDE].height * cEdges);
                float fFirstRightSideEdgeRenderY = ((float) m_sCornersSize[CORNER_RIGHT_UP].height + fRenderY);

                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][0].x = fEdgeRenderX;
                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][0].y = fRenderHeight;

                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][2].y = fRenderHeight;
                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][2].x = fRenderWidth;

                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][4].x = fRenderWidth;
                float fNextNextRightSideEdgeRenderY = fFirstRightSideEdgeRenderY + (float) nNextEdgePosY;
                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][4].y = fNextNextRightSideEdgeRenderY;

                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][6].y = fNextNextRightSideEdgeRenderY;
                m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][6].x = fEdgeRenderX;

                if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE],
                                                              sizeof(m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE]),
                                                              &local_3c) !=
                    0) {
                    IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                    g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                    g_RStateMgr.FUN_00470060(6, local_3c, 2);
                }
            }
        }

        // we will create the last edge texture
        if ((nEdgeBoundSize % m_sEdgesSize[EDGE_RIGHT_SIDE].height) != 0) {
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].y = 1.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][5].z = (
                    (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_RIGHT_SIDE].height) /
                    (float) m_sEdgesSize[EDGE_RIGHT_SIDE].height);

            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].y = 0.0f;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][7].z = (
                    (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_RIGHT_SIDE].height) /
                    (float) m_sEdgesSize[EDGE_RIGHT_SIDE].height);

            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][0].x = fEdgeRenderX;
            float fRenderLastEdgeY = (((float) m_sCornersSize[CORNER_RIGHT_UP].height + fRenderY) +
                                      ((float) (m_sEdgesSize[EDGE_RIGHT_SIDE].height * cEdges)));
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][0].y = fRenderLastEdgeY;

            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][2].y = fRenderLastEdgeY;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][2].x = fRenderWidth;

            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][4].x = fRenderWidth;
            float fRenderLastEdgeYPerv =
                    fRenderLastEdgeY + (float) (nEdgeBoundSize % m_sEdgesSize[EDGE_RIGHT_SIDE].height);
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][4].y = fRenderLastEdgeYPerv;

            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][6].y = fRenderLastEdgeYPerv;
            m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE][6].x = fEdgeRenderX;

            if (g_pDynamicVertexBuffer->IVBDynamic_Func_6(m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE],
                                                          sizeof(m_vecEdgesRenderBuff[EDGE_RIGHT_SIDE]), &local_3c) !=
                0) {
                IDirect3DVertexBuffer9 *iVar5 = g_pDynamicVertexBuffer->IVBDynamic_Func_5();
                g_RStateMgr.m_pDevice->SetStreamSource(0, iVar5, 0, 0x18);
                g_RStateMgr.FUN_00470060(6, local_3c, 2);
            }
        }
    }

    // Back to main one
    DefaultTextureAlphaOperations();
#endif // if 0
}

void CNIFFrame::SetFrameTexture(std::n_string strTexture) {
    std::n_string strCorners[4] = {strTexture + "left_up.ddj", strTexture + "right_up.ddj",
                                   strTexture + "right_down.ddj", strTexture + "left_down.ddj"};
    SetCornersTexture(strCorners);

    std::n_string strEdges[4] = {strTexture + "left_side.ddj", strTexture + "mid_up.ddj",
                                 strTexture + "right_side.ddj", strTexture + "mid_down.ddj"};
    SetEdgesTexture(strEdges);
}