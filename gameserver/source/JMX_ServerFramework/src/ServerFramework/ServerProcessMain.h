//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include "ServerProcessBase.h"

namespace ServerFramework {
    class CServerProcessMain : public CServerProcessBase {
    public:
        void RegisterMsgHandlers() override;

    public:
        virtual void OnCordSetup(CMsg *pMsg);

        virtual void FlushMsg(CMsg *pMsg);

        virtual void OnCertificationReq(CMsg *pMsg);

        virtual void OnCertificatedServer(CMassiveMsg *pMassiveMsg);

        virtual void OnFileTransferReq(CMsg *pMsg);

        virtual void OnFileTransfer(CMsg *pMsg);

        virtual void Func_26();

        virtual void Func_27();

        virtual void Func_28();

        virtual void Func_29();

        virtual void OnServerSessionCreate(CMsg *pMsg);

        virtual void OnServerSessionLose(CMsg *pMsg);

        virtual void OnHandShakingSession(CMsg *pMsg);

        virtual void Func_33();

        virtual void Func_34();

        virtual void Func_35();

        virtual void Func_36();

        virtual void Func_37();

        virtual void Func_38();

        virtual void Func_39();

        virtual void Func_40();

        virtual void Func_41();

        virtual void Func_42();

        virtual void Func_43();

        virtual void Func_44();

        virtual void Func_45();

        virtual void Func_46();

        virtual void Func_47();

        virtual void Func_48();

    public:
    private:
        char pad_400b8[12]; //0x400b8 std::map
        char pad_400c4[4]; //0x400c4 class trigger ptr
    };
}