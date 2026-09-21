#include "BasicWindow.h"
#include "imgui/imgui.h"

using namespace Devkit;

BasicWindow::BasicWindow(const std::string &name, bool show) : m_sName(name), m_bShow(show) {
}

const std::string &BasicWindow::getName() const {
    return m_sName;
}

void BasicWindow::MenuItem() {
    ImGui::MenuItem(m_sName.c_str(), 0, &m_bShow);
}
