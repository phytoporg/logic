#include "Instance.h"

#include <logik/window/Window.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

//REMOVEME
#include <iostream>

namespace
{
    template<typename LambdaType>
    std::optional<uint32_t> FindQueueFamilyIndex(
        VkPhysicalDevice device,
        LambdaType fnIsGood)
    {
        std::optional<uint32_t> index;

        uint32_t queueFamilyCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queueFamilyCount, queueFamilies.data());
        int i{0};
        for (const auto& queueFamily : queueFamilies)
        {
            if (fnIsGood(queueFamily, i))
            {
                index = i;
            }

            if (index.has_value())
            {
                break;
            }

            ++i;
        }

        return index;
    }

	std::vector<const char*> GetRequiredExtensions() 
    {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(
			glfwExtensions,
            glfwExtensions + glfwExtensionCount);

		return extensions;
	}

	std::vector<const char*> GetRequiredDeviceExtensions() 
    {
        return std::vector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	}
}

namespace logik
{
    Instance::Instance(const std::string& appName)
        : m_appName(appName),
          m_vkInstance(VK_NULL_HANDLE),
          m_vkLogicalDevice(VK_NULL_HANDLE),
          m_vkGraphicsQueue(VK_NULL_HANDLE),
          m_vkSurfaceKHR(VK_NULL_HANDLE)
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
        std::vector<const char*> extensions = GetRequiredExtensions();
        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        //
        // TODO: Validation layers?
        //
        createInfo.enabledLayerCount = 0;

        {
            const VkResult Result = vkCreateInstance(
                &createInfo, nullptr, &m_vkInstance);
            if (Result != VK_SUCCESS)
            {
                throw std::runtime_error(
                        "Failed to create instance! VkResult = " +
                        std::to_string(static_cast<uint32_t>(Result)));
            }
        }

        uint32_t deviceCount{0};
        vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
        if (!deviceCount)
        {
            throw std::runtime_error("Could not find a physical device.");
        }

        std::vector<VkPhysicalDevice> vkDevices;
        vkDevices.resize(deviceCount);
        vkEnumeratePhysicalDevices(
            m_vkInstance, &deviceCount, vkDevices.data());

        //
        // TODO: everything else
        //
        m_vkPhysicalDevice = vkDevices.front();
        std::optional<uint32_t> graphicsQueueIndex =
            FindQueueFamilyIndex(
                m_vkPhysicalDevice,
                [](VkQueueFamilyProperties queueFamily, int /* queueIdx */)
                {
                    return queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                });
        if (!graphicsQueueIndex.has_value())
        {
            throw std::runtime_error("Couldn't find gfx queue family index.");
        }

        const uint32_t GraphicsQueueIndex = graphicsQueueIndex.value();

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = GraphicsQueueIndex;
        queueCreateInfo.queueCount       = 1;

        float queuePriority{1.f};
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        auto deviceExtensions = GetRequiredDeviceExtensions();
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        deviceCreateInfo.enabledLayerCount = 0;

        {
            const VkResult Result = 
                vkCreateDevice(
                    m_vkPhysicalDevice,
                    &deviceCreateInfo,
                    nullptr,
                    &m_vkLogicalDevice);
            if (Result != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }
        }

        vkGetDeviceQueue(
            m_vkLogicalDevice, GraphicsQueueIndex, 0, &m_vkGraphicsQueue);
        m_graphicsQueueIndex = GraphicsQueueIndex;
    }

    Instance::~Instance()
    {
        if (m_vkInstance && m_vkSurfaceKHR)
        {
            vkDestroySurfaceKHR(m_vkInstance, m_vkSurfaceKHR, nullptr);
        }

        if (m_vkInstance)
        {
            vkDestroyInstance(m_vkInstance, nullptr);
        }

        if (m_vkLogicalDevice)
        {
            vkDestroyDevice(m_vkLogicalDevice, nullptr);
        }

        if (m_swapChain)
        {
            vkDestroySwapchainKHR(m_vkLogicalDevice, m_swapChain, nullptr);
        }

        glfwTerminate();
    }

    WindowPtr Instance::CreateWindow(
        uint32_t width, uint32_t height, const std::string& title)
    {
        std::shared_ptr<Window> spReturn(new Window(width, height, title));

        {
            const VkResult Result =
                glfwCreateWindowSurface(
                    m_vkInstance,
                    spReturn->GetWindow(),
                    nullptr,
                    &m_vkSurfaceKHR);
            if (Result != VK_SUCCESS) 
            {
                return nullptr;
            }
        }

        std::optional<uint32_t> presentQueueIndex =
            FindQueueFamilyIndex(
                m_vkPhysicalDevice,
                [this](VkQueueFamilyProperties queueFamily, int queueIdx)
                {
                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(
                        m_vkPhysicalDevice,
                        queueIdx,
                        m_vkSurfaceKHR,
                        &presentSupport);

                    return presentSupport;
                });
        if (!presentQueueIndex.has_value())
        {
            return nullptr;
        }

		float queuePriority = 1.f;

        const uint32_t PresentQueueIndex{presentQueueIndex.value()};
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = PresentQueueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

        vkGetDeviceQueue(
            m_vkLogicalDevice,
            PresentQueueIndex,
            0,
            &m_vkPresentQueue);

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_vkPhysicalDevice, m_vkSurfaceKHR, &capabilities);

        const uint32_t ImageCount = capabilities.minImageCount + 1;

        VkSwapchainCreateInfoKHR createSwapChainInfo{};
        createSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createSwapChainInfo.surface = m_vkSurfaceKHR;
        createSwapChainInfo.minImageCount = ImageCount;
        createSwapChainInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        createSwapChainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        createSwapChainInfo.imageExtent = {width, height};
        createSwapChainInfo.imageArrayLayers = 1;
        createSwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = 
        {
            m_graphicsQueueIndex, PresentQueueIndex
        };

		if (m_vkGraphicsQueue != m_vkPresentQueue) {
			createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createSwapChainInfo.queueFamilyIndexCount = 2;
			createSwapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createSwapChainInfo.queueFamilyIndexCount = 0; // Optional
			createSwapChainInfo.pQueueFamilyIndices = nullptr; // Optional
		}

        createSwapChainInfo.preTransform = capabilities.currentTransform;
        createSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createSwapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        createSwapChainInfo.clipped = VK_TRUE;
        createSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

        {
            const VkResult Result =
                vkCreateSwapchainKHR(
                    m_vkLogicalDevice,
                    &createSwapChainInfo,
                    nullptr,
                    &m_swapChain);
            if (Result != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create swap chain!");
            }
        }

        m_presentQueueIndex = PresentQueueIndex;

        return spReturn;
    }
}

