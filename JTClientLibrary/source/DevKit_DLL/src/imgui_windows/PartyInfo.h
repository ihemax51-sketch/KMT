#pragma once

#include "BasicWindow.h"

namespace Devkit {
    class PartyInfo : public BasicWindow {
        struct ListColumnHeader {
            const char *label;
            float size;
        };

    public:
        PartyInfo();

        void Render();
    };
}
