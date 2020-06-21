//
// Main Logik instance object. The root of all LOGIK.
//

#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>

namespace logik
{
    class Window;
    using WindowPtr = std::shared_ptr<Window>;

    class Instance
    {
    public:
        Instance(const std::string& appName);
        ~Instance();

        WindowPtr CreateWindow(
            uint32_t width, uint32_t height, const std::string& title);

    private:
        VkInstance       m_vkInstance;
        VkPhysicalDevice m_vkPhysicalDevice;
        VkDevice         m_vkLogicalDevice;

        VkQueue      m_vkGraphicsQueue;
        uint32_t     m_graphicsQueueIndex;

        std::string  m_appName;

        VkSurfaceKHR m_vkSurfaceKHR;

        VkQueue      m_vkPresentQueue;
        uint32_t     m_presentQueueIndex;

        VkSwapchainKHR m_swapChain;
    };
}

