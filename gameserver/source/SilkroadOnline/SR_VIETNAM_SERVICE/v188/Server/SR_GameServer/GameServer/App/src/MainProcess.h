//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include "ServerFramework/ServerProcessMain.h"

class CMainProcess : public ServerFramework::CServerProcessMain {
public:
    // Impl
    void _OnProcessMessage(MSG_HANDLE);
    void _OnQueueTimer(MSG_HANDLE);

private:
};
