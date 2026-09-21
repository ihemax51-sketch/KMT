#pragma once

#include <GFXMainFrame/Process.h>

#include "BasicWindow.h"

namespace Devkit {
    class ProcessViewer : public BasicWindow {
        struct ColumnHeader {
            const char *label;
            float width;
        };

    public:
        ProcessViewer();

        void Render();

    private:
        void RenderThreadlist(const CProcess *proc) const;

        void RenderSomeMap(const CProcess *proc) const;
    };
}
