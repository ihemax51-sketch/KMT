#pragma once

#include "BasicWindow.h"

namespace Devkit {
    class EntityExplorer : public BasicWindow{

        struct EntityListColumnHeader {
            const char *label;
            float size;
        };


    public:
        EntityExplorer();

        void Render();

        void MenuItem();

    private:
        bool bShow;
    };
}// namespace Devkit