#pragma once

#include "BasicWindow.h"

namespace Devkit {
    class Inventory : public BasicWindow {
    public:
        Inventory();

        /// \brief Create the window
        void Render();
    };
}// namespace Devkit