#pragma once

#include <BSLib/BSLib.h>

#include "BasicWindow.h"

namespace Devkit {
    class SoundTool : public BasicWindow {
    public:
        SoundTool();

        void Render();

    private:
        int ImGui_Window_SoundTool_Filter(std::vector<std::n_string> *items, const std::n_wstring &search);

        bool ImGui_Window_SoundTool_FilterMatch(const std::n_wstring &input, const std::n_wstring &search);


    private:
        char buffer[100];
    };
}