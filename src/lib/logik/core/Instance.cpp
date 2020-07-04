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
          m_vkSurfaceKHR(VK_NULL_HANDLE),
          m_swapChain(VK_NULL_HANDLE),
          m_vertShaderModule(VK_NULL_HANDLE),
          m_fragShaderModule(VK_NULL_HANDLE),
          m_pipelineLayout(VK_NULL_HANDLE),
          m_renderPass(VK_NULL_HANDLE),
          m_graphicsPipeline(VK_NULL_HANDLE),
          m_commandPool(VK_NULL_HANDLE),
          m_imageAvailableSemaphore(VK_NULL_HANDLE),
          m_renderFinishedSemaphore(VK_NULL_HANDLE)
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
        if (m_imageAvailableSemaphore)
        {
            vkDestroySemaphore(
                m_vkLogicalDevice, m_renderFinishedSemaphore, nullptr);
        }

        if (m_imageAvailableSemaphore)
        {
            vkDestroySemaphore(
                m_vkLogicalDevice, m_imageAvailableSemaphore, nullptr);
        }

        if (m_commandPool)
        {
            vkDestroyCommandPool(m_vkLogicalDevice, m_commandPool, nullptr);
        }

        for (VkFramebuffer frameBuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_vkLogicalDevice, frameBuffer, nullptr);
        }

        if (m_graphicsPipeline)
        {
            vkDestroyPipeline(m_vkLogicalDevice, m_graphicsPipeline, nullptr);
        }

        if (m_renderPass)
        {
            vkDestroyRenderPass(
                m_vkLogicalDevice,
                m_renderPass,
                nullptr);
        }

        if (m_pipelineLayout)
        {
            vkDestroyPipelineLayout(
                m_vkLogicalDevice,
                m_pipelineLayout,
                nullptr);
        }

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
        // Configure multisampling (disabled)
        //
        VkPipelineMultisampleStateCreateInfo multiSamplingInfo{};
        multiSamplingInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multiSamplingInfo.sampleShadingEnable = VK_FALSE;
        multiSamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multiSamplingInfo.minSampleShading = 1.0f; // Optional
        multiSamplingInfo.pSampleMask = nullptr; // Optional
        multiSamplingInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        multiSamplingInfo.alphaToOneEnable = VK_FALSE; // Optional

        //
        // No depth/stencil tests for the moment.
        //
        
        //
        // Color blending modes (no blending atm)
        //
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = 
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        //
        // TODO: This would be a good place to configure a VkDynamicState if 
        // we were using one.
        //

        //
        // Pipeline layout config
        //

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(
            m_vkLogicalDevice,
            &pipelineLayoutInfo,
            nullptr,
            &m_pipelineLayout) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        //
        // Color buffer attachment
        //
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format  = m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //
        // Color out parameter in vertex shader?
        //
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        //
        // Create the render pass
        //
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(
            m_vkLogicalDevice,
            &renderPassInfo,
            nullptr,
            &m_renderPass) != VK_SUCCESS) 
        {
			throw std::runtime_error("failed to create render pass!");
		}

        //
        // Finally create the pipeline!
        //
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multiSamplingInfo;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(
            m_vkLogicalDevice,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_graphicsPipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
        }

        //
        // Finally create them swap chain framebuffers
        //
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) 
		{
			VkImageView attachments[] = { m_swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(
                m_vkLogicalDevice,
                &framebufferInfo,
                nullptr,
                &m_swapChainFramebuffers[i]) != VK_SUCCESS) 
            {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}

        //
        // Command pools
        //
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_graphicsQueueIndex;
        poolInfo.flags = 0; // Optional

        if (vkCreateCommandPool(
            m_vkLogicalDevice,
            &poolInfo,
            nullptr,
            &m_commandPool) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create command pool!");
        }

        //
        // Command buffers
        //
        m_commandBuffers.resize(m_swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount =
            static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(
            m_vkLogicalDevice,
            &allocInfo,
            m_commandBuffers.data()) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        //
        // Set up the draw commands.
        //
        for (size_t i = 0; i < m_commandBuffers.size(); i++) 
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(
                m_commandBuffers[i],
                &beginInfo) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_renderPass;
            renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = m_swapChainExtent;

            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(
                m_commandBuffers[i],
                &renderPassInfo,
                VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(
                m_commandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_graphicsPipeline);
            vkCmdDraw(
                m_commandBuffers[i],
                3, // Vertex count
                1, // Instance count
                0, // First vertex
                0);// First instance
            vkCmdEndRenderPass(m_commandBuffers[i]);
            if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        //
        // Create semaphores
        //

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(
            m_vkLogicalDevice,
            &semaphoreInfo,
            nullptr,
            &m_imageAvailableSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error(
                    "Failed to create image available semaphore");
        }

        if (vkCreateSemaphore(
            m_vkLogicalDevice,
            &semaphoreInfo,
            nullptr,
            &m_renderFinishedSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error(
                    "Failed to create render finished semaphore");
        }

        return true;
    }

    void Instance::DrawFrame()
    {
        uint32_t imageIndex;
        vkAcquireNextImageKHR(
            m_vkLogicalDevice,
            m_swapChain,
            UINT64_MAX,
            m_imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] =
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(
            m_vkGraphicsQueue,
            1,
            &submitInfo,
            VK_NULL_HANDLE) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
    }
}

