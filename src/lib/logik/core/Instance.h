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

        WindowPtr CreateWindow(
            uint32_t width, uint32_t height, const std::string& title);

    private:
        VkInstance m_vkInstance;

        std::string m_appName;
    };
}

