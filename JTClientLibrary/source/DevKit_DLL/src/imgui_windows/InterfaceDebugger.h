#pragma once

#include "BasicWindow.h"
namespace Devkit {
    class InterfaceDebugger : public BasicWindow {
    public:
        InterfaceDebugger();

        void Render();

    private:
        bool interface_draw_outline;
        bool interface_draw_marker;
    };
}// namespace Devkit