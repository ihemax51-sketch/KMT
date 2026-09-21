//
// Created by Kurama on 2/24/2023.
//
#pragma once

#include "NIFWnd.h"

#define CORNER_LEFT_UP 0
#define CORNER_RIGHT_UP 1
#define CORNER_RIGHT_DOWN 2
#define CORNER_LEFT_DOWN 3

#define EDGE_LEFT_SIDE 0
#define EDGE_MID_UP 1
#define EDGE_RIGHT_SIDE 2
#define EDGE_MID_DOWN 3

class CNIFTileWnd : public CNIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CNIFTileWnd, 0x0115df48)

public:
    /// \address 004748c0
    ~CNIFTileWnd();

public:
    /// \address 00474960
    bool OnCreate(long ln) override;

    /// \address 004757d0
    void Func_40() override;

    /// \address 00474d00
    void UpdateTexturesCoordinates() override;

public:
    /// \address 00475710
    void NTB_Func_13(std::n_string str, int a3, int a4) override;

private:
    /// \address 00474ed0
    virtual void SetFrameTexture(std::n_string strTexture);

protected:
    /// \address 00474980
    void InitializeFrameTextures();

private:
    /// \address 00474aa0
    void ReleaseTextures();

protected:
    /// \address 00474b20
    void OptimizeEdgesVerticesPos();

    /// \address 00474c10
    void OptimizeCornersVerticesPos();

    /// \address 004750a0
    void SetCornersTexture(std::n_string strCorners[4]);

    /// \address 00475400
    void SetEdgesTexture(std::n_string strEdges[4]);

public:
protected:
    bool field_0348; //0x0348
    IDirect3DBaseTexture9 *m_pCornersTexture[4]; //0x034C
    IDirect3DBaseTexture9 *m_pEdgesTexture[4]; //0x035C
    std::n_string m_strCornersTexture[4]; //0x036C
    std::n_string m_strEdgesTexture[4]; //0x03DC
    D3DVECTOR m_vecCornersRenderBuff[4][8]; //0x044C
    D3DVECTOR m_vecEdgesRenderBuff[4][8]; //0x05CC
    wnd_size m_sCornersSize[4]; //0x074C
    wnd_size m_sEdgesSize[4]; //0x076C
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x078C)
        ENSURE_OFFSET(m_vecCornersRenderBuff, 0x044C)
        ENSURE_OFFSET(m_vecEdgesRenderBuff, 0x05CC)
    END_FIXTURE()

    RUN_FIXTURE(CNIFTileWnd)
};