#include "Instance.h"

#include <logik/window/Window.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <fstream>
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

    std::vector<char> ReadBinaryFile(const std::string& filename)
    {
        std::vector<char> buffer;
        std::ifstream in(filename, std::ios::ate | std::ios::binary);

        if (!in.is_open()) { return buffer; }

        const size_t FileSize(in.tellg());
        buffer.resize(FileSize);

        in.seekg(0);
        in.read(buffer.data(), FileSize);

        in.close();

        return buffer;
    }

    std::string GetShaderPath()
    {
        // Hard-coded for now
        return "/home/philjo/code/logik/src/exe/skeleton/";
    }

    VkShaderModule 
    CreateShaderModule(
        VkDevice device,
        const std::vector<char>& codeBuffer)
    {
		VkShaderModuleCreateInfo shaderModuleInfo{};
		shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleInfo.codeSize = codeBuffer.size();
		shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(codeBuffer.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(
            device,
            &shaderModuleInfo,
            nullptr,
            &shaderModule) != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }

        return shaderModule;
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
        if (m_vertShaderModule)
        {
            vkDestroyShaderModule(
                m_vkLogicalDevice,
                m_vertShaderModule,
                nullptr);
        }

        if (m_fragShaderModule)
        {
            vkDestroyShaderModule(
                m_vkLogicalDevice,
                m_fragShaderModule,
                nullptr);
        }

        if (m_swapChain)
        {
            vkDestroySwapchainKHR(m_vkLogicalDevice, m_swapChain, nullptr);
        }

        for (auto imageView : m_swapChainImageViews)
        {
            vkDestroyImageView(m_vkLogicalDevice, imageView, nullptr);
        }

        if (m_vkInstance && m_vkSurfaceKHR)
        {
            vkDestroySurfaceKHR(m_vkInstance, m_vkSurfaceKHR, nullptr);
        }

        if (m_vkLogicalDevice)
        {
            vkDestroyDevice(m_vkLogicalDevice, nullptr);
        }

        if (m_vkInstance)
        {
            vkDestroyInstance(m_vkInstance, nullptr);
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

        m_presentQueueIndex = PresentQueueIndex;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_vkPhysicalDevice, m_vkSurfaceKHR, &capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        m_swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        m_swapChainExtent = {width, height};

        VkSwapchainCreateInfoKHR createSwapChainInfo{};
        createSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createSwapChainInfo.surface = m_vkSurfaceKHR;
        createSwapChainInfo.minImageCount = imageCount;
        createSwapChainInfo.imageFormat = m_swapChainImageFormat;
        createSwapChainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        createSwapChainInfo.imageExtent = m_swapChainExtent;
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

        vkGetSwapchainImagesKHR(
            m_vkLogicalDevice,
            m_swapChain,
            &imageCount,
            nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(
            m_vkLogicalDevice,
            m_swapChain,
            &imageCount,
            m_swapChainImages.data());

        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i{0}; i < m_swapChainImageViews.size(); ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = m_swapChainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_swapChainImageFormat;

            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            
            {
                const VkResult Result =
                    vkCreateImageView(
                        m_vkLogicalDevice,
                        &imageViewCreateInfo,
                        nullptr,
                        &m_swapChainImageViews[i]);
                if (Result != VK_SUCCESS) 
                {
                    throw std::runtime_error("Failed to create image views!");
                }
            }
        }

        if (!CreateGraphicsPipeline())
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }


        return spReturn;
    }

    bool Instance::CreateGraphicsPipeline()
    {
        const std::string VertShaderPath(GetShaderPath() + "vert.spv");
        std::vector<char> vertShaderCode = ReadBinaryFile(VertShaderPath);
        if (vertShaderCode.empty()) { return false; }

        const std::string FragShaderPath(GetShaderPath() + "frag.spv");
        std::vector<char> fragShaderCode = ReadBinaryFile(FragShaderPath);
        if (fragShaderCode.empty()) { return false; }

        m_vertShaderModule = CreateShaderModule(m_vkLogicalDevice, vertShaderCode);
        if (!m_vertShaderModule) { return false; }

        m_fragShaderModule = CreateShaderModule(m_vkLogicalDevice, fragShaderCode);
        if (!m_fragShaderModule) { return false; }

        VkPipelineShaderStageCreateInfo vertStageInfo{};
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = m_vertShaderModule;
        vertStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = m_fragShaderModule;
        fragStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] =
            {vertStageInfo, fragStageInfo};

        //
        // Not accepting vertex input for now; verts are hard-coded in the
        // shader. Will change later.
        //
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = 
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        //
        // Simply match the frame buffer dimensions
        //
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapChainExtent;

        //
        // Combine viewport, scissor parameters into a viewport state
        //
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        //
        // Configure rasterization state
        //
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //
        // Configure multisampling
        //
        
        // TODO: 
        // https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

        return true;
    }
}

