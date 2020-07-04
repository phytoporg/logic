#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace logik
{
    Window::Window(uint32_t width, uint32_t height, const std::string& title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        //
        // TODO: Remove once resize events are properly handled.
        //
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_pWindow =
            glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }

    void Window::PollEvents(std::function<void()> fnTick)
    {
        while(!glfwWindowShouldClose(m_pWindow)) 
        {
            glfwPollEvents();
            fnTick();
        }
    }

    GLFWwindow* Window::GetWindow()
    {
        return m_pWindow;
    }
}

