#include "Instance.h"

#include <logik/window/Window.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace logik
{
    Instance::Instance(const std::string& appName)
        : m_appName(appName)
    {
        if (const int Result = glfwInit(); Result != GLFW_TRUE)
        {
            throw std::runtime_error("Failed to initialize glfw");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "No engine";
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount{0};
        const char** glfwExtensions;

        //
        // TODO: Set up glfw
        //
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount   = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        //
        // TODO: Validation layers?
        //
        createInfo.enabledLayerCount = 0;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create instance! VkResult = " +
                    std::to_string(static_cast<uint32_t>(result)));
        }
    }

    WindowPtr Instance::CreateWindow(
            uint32_t width, uint32_t height, const std::string& title)
    {
        std::shared_ptr<Window> spReturn(new Window(width, height, title));
        return spReturn;
    }
}

