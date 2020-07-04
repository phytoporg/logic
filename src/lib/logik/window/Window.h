//
// Windowing functionality in one nice package.
//
// Use a logik Instance to create a window.
//

#pragma once

#include <string>
#include <memory>
#include <functional>

struct GLFWwindow;

namespace logik
{
    class Window
    {
    public:
        ~Window();

        void PollEvents(std::function<void()> fnTick);
        GLFWwindow* GetWindow();

    private:
        Window(uint32_t width, uint32_t height, const std::string& title);
        Window(const Window& window) = delete;

        GLFWwindow*  m_pWindow;

        friend class Instance;
    };

    using WindowPtr = std::shared_ptr<Window>;
}

