#pragma once

#include <string>


namespace Devkit {
    /// A basic window with a menu item
    class BasicWindow {
    public:
        /// \param name Name of the window, used as menu item text and window title
        /// \param show Default window visible state
        BasicWindow(const std::string &name, bool show);

        /// Render MenuItem in-place
        void MenuItem();

    protected:
        const std::string &getName() const;

    private:
        std::string m_sName;

    protected:
        bool m_bShow;
    };
}// namespace Devkit
