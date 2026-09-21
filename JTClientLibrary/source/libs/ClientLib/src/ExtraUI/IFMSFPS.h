#pragma once
#include "IFDragableArea.h"
#include "IFGauge.h"
#include "IFStatic.h"
#include <sys\timeb.h>
#include "ClientNet/MsgStreamBuffer.h"

class CIFMSFPS : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFMSFPS)

public:
    CIFMSFPS(void);
    ~CIFMSFPS(void);

    bool OnCreate(long ln) override;

    void OnMovementRequest(CMsgStreamBuffer& pMsg);
    void OnMovementResponse(CMsgStreamBuffer &pMsg);
    bool UpdateLocation;
    virtual void OnUpdate() override;

    struct MovementDestination {
        short RegionID;
        short PosX;
        short PosY;
        short PosZ;

        bool MovementDestination::operator==(const MovementDestination &rhs) const {
            return (RegionID == rhs.RegionID && PosX == rhs.PosX && PosY == rhs.PosY && PosZ == rhs.PosZ);
        }
    };


    int m_FPS;
    time_t m_LastFPSRefresh;
    MovementDestination m_LastMovementDestinationRequest;
    timeb m_LastMovementRequestTime;

public:
    CIFStatic *m_count0;
    CIFStatic *m_count1;

void UpdateMenuSize();
};