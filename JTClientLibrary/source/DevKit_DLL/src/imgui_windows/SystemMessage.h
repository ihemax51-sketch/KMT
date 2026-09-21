#pragma once

#include "BasicWindow.h"

namespace Devkit {
    class SystemMessage : public BasicWindow {
    public:
        SystemMessage();

        void Render();

    private:
        float m_color[3];
        char m_buffer[200];
    };
}
