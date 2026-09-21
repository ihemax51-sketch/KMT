#pragma once

#include <SRIFLib/NIFMainFrame.h>
#include "IFStatic.h"


class CWorldMapGuideData;
class CNIFWorldMap : public CNIFMainFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CNIFWorldMap, 0xEE933C)

GFX_DECLARE_MESSAGE_MAP(CNIFWorldMap)
public:
    GFX_MSGMAP* MessageMap();

    void OnClickPingButton();
    bool OnWMCreate(long ln);
    void OnWMRenderMySelf();
    void UpdateButtonPosition();
    void OnTimerIMPL(int timerId);
    int GetRegionTypeMaybe(unsigned short region,float x,float y,float z);
    void FUN_00622b60(int p1, int p2);
    void SetMapMode();
    void FUN_00624f40(CWorldMapGuideData* p1, undefined4 p2, undefined4 p3);
    void TEST(int p1);
    void FUN_0061c5a0(int p1, int p2);
    bool CenterFieldMapAt(int regionX, int regionY, int mapPositionX, int mapPositionZ);
    void FUN_006217e0(int p1);
    void FUN_006256e0(int p1);
    void Fun_0061a620();
    void Fun_00622f00(undefined4 p1, undefined4 p2,undefined4 p3,undefined4  p4,undefined4  p5,undefined4 p6,undefined4 p7,undefined4 p8,
                      undefined4 p9, undefined4 p10, undefined4 p11);
    int FUN_00614170(undefined4 p1, short p2, float p3, float p4, float p5, undefined4 p6);

    void FUN_00621c60(float p1, float p2);
    void FUN_00620550(int p1, int p2);
    bool OnMouseActions(Event3D* mouseData);
    undefined1 OnCloseWndIMPL();
public:
    char pad_0798[4]; //0x0798
    int m_ClickedMapPostion_x; //0x079C
    int m_ClickedMapPostion_y; //0x07A0un
    int m_CursorPostion_x; //0x07A4
    int m_CursorPostion_y; //0x07A8
    int Is0OutSide2IsInside; //0x07AC
    byte N00001B50; //0x07B0
    byte IsAuto1and0IsManual; //0x07B1
    byte N00002471; //0x07B2
    byte N0000247F; //0x07B3
    char pad_07B4[56]; //0x07B4
    int N00001B5F; //0x07EC
    char pad_07F0[1432]; //0x07F0
    float N00001CCA; //0x0D88
    float N00001CCB; //0x0D8C
    char pad_0D90[16]; //0x0D90
    float N00001CD0; //0x0DA0
    float N00001CD1; //0x0DA4
    char pad_0DA8[16]; //0x0DA8
    float N00001CD6; //0x0DB8
    float N00001CD7; //0x0DBC
    char pad_0DC0[34828]; //0x0DC0
    float m_currentPosX; //0x95CC
    float m_currentPosY; //0x95D0
    char pad_95D4[16]; //0x95D4
    float X2; //0x95E4
    char pad_95E8[24]; //0x95E8
    float Y2; //0x9600
    char pad_9604[424]; //0x9604
    float UNK; //0x97AC
    float UNK2; //0x97B0
    float PAD97B4;
    float PAD97B8;
    float RenderingCharIconX; //0x97BC
    float RenderingCharIconY; //0x97C0
    float RenderingCharIconZ; //0x97C4
    float InitialDir; //0x97C8
    int MapType1IsSmall0IsBig; //0x97CC
    int Normal0Town1Dungeon2;
    char pad_97D0[20]; //0x97D0
    int m_stateX; //0x97E8
    char pad_97EC[8]; //0x97EC
    int m_viewWidth; //0x97F4
    float m_stateY; //0x97F8
    float m_viewHeight; //0x97FC
    char pad_9800[32]; //0x9800
    int *m_currentPos; //0x9820
    int *N00003F5A; //0x9824
    char pad_9828[376]; //0x9828
    int *m_CurrentDungeonInfo; //0x99A0
    char pad_99A4[102408]; //0x99A4
    byte N0000A3BC; //0x229AC
    byte field_229ad; //0x229AD
    char pad_229AE[62]; //0x229AE
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x229EC)
        ENSURE_OFFSET(m_currentPosX,0x95cc)
        ENSURE_OFFSET(m_currentPosY,0x95d0)
        ENSURE_OFFSET(m_stateX,0x97e8)
        ENSURE_OFFSET(m_viewWidth,0x97f4)
        ENSURE_OFFSET(m_stateY,0x97f8)
        ENSURE_OFFSET(m_viewHeight,0x97fc)
        ENSURE_OFFSET(m_currentPos,0x9820)
    END_FIXTURE()

    RUN_FIXTURE(CNIFWorldMap)




};
