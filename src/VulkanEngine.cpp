/**
 * @class   VulkanEngine
 * @brief   Main class for the Vulkan engine, for now
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#include "VulkanEngine.hpp"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <set>
#include <fstream>

#ifdef NDEBUG
         const bool enableValidationLayers = false;
#else
         const bool enableValidationLayers = true;
#endif

VkResult VulkanEngine::CreateDebugReportCallbackEXT(VkInstance instance,
                                             const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugReportCallbackEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanEngine::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

void VulkanEngine::run() {
    _initWindow();
    _initVulkan();
    _mainLoop();
}

void VulkanEngine::_initWindow() {
    glfwInit();

    /**
     * GLFW_NO_API is how we indicate that we want the window
     * to support Vulkan
     */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    /**
     * TODO: for now resizing in Vulkan is cumbersome, so don't do it
     */
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void VulkanEngine::_initVulkan() {
    _createInstance();
    _setupDebugCallback();
    _createSurface();
    _pickPhysicalDevice();
    _createLogicalDevice();
    _createSwapChain();
    _createImageViews();
    _createRenderPass();
    _createGraphicsPipeline();
    _createFramebuffers();
    _createCommandPool();
    _createCommandBuffers();
    _createSemaphores();
}

void VulkanEngine::_mainLoop() {
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        _drawFrame();
    }

    vkDeviceWaitIdle(_device);
}

void VulkanEngine::_createInstance() {
    /* Check the validation layers */
    if (enableValidationLayers && !_checkValidationLayerSupport()) {
        throw std::runtime_error("ERROR validation layers requested, but not available!");
    }

    /* Query extensions */
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> vkExtensions(extensionCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkExtensions.data());

    std::cout << "Available Vulkan extensions:" << std::endl;
    for (const auto& extension : vkExtensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
    }

    /* Application info */
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Engine Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Gabriell Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    /* Instance creation info */
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    /* Get required extensions info */
    auto requiredExtensions = _getRequiredExtensions();

    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    /* Validation layers */
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = _validationLayers.size();
        createInfo.ppEnabledLayerNames = _validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    /* Create Vulkan instance */
    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("ERROR creating Vulkan instance: " + std::to_string(result));
    }
}

void VulkanEngine::_createLogicalDevice()
{
    QueueFamilyIndices indices = _findQueueFamilies(_physicalDevice);

    /* Queue families creation info */
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};

        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    /* Logical device creation */
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

    VkPhysicalDeviceFeatures deviceFeatures = {};
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = _deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    /* Validation layers */
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = _validationLayers.size();
        createInfo.ppEnabledLayerNames = _validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, _device.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily, 0, &_presentQueue);
}

void VulkanEngine::_createSurface()
{
    if (glfwCreateWindowSurface(_instance, _window, nullptr, _surface.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create window surface!");
    }
}

bool VulkanEngine::_checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    /* Print layers information */
    if (layerCount > 0) {
        std::cout << "Available layers: " << std::endl;
    } else {
        std::cout << "No layers availabe" << std::endl;
    }

    for (const auto& layerProperties : availableLayers) {
        std::cout << "\t" << layerProperties.layerName << std::endl;
    }

    /* Check requested layers */
    for (const char* layerName : _validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            std::cout << "ERROR requested layer " << layerName << " not found" << std::endl;
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanEngine::_getRequiredExtensions() {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanEngine::_setupDebugCallback()
{
    if (!enableValidationLayers) {
        return;
    }

    VkDebugReportCallbackCreateInfoEXT createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = _debugCallback;

    if (CreateDebugReportCallbackEXT(_instance, &createInfo, nullptr, &_callbackHandle) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback!");
    }
}

void VulkanEngine::_pickPhysicalDevice()
{
    VkPhysicalDevice device = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("ERROR failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    /* Show physical devices on screen */
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        fprintf(stderr, "[%s] physical device\n", props.deviceName);
        fprintf(stderr, "\tAPI version:    %08x\n", props.apiVersion);
        fprintf(stderr, "\tDriver version: %08x\n", props.driverVersion);
        fprintf(stderr, "\tVendor ID:      %08x\n", props.vendorID);
        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                fprintf(stderr,  "\tType:           Other\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                fprintf(stderr,  "\tType:           Integrated GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                fprintf(stderr,  "\tType:           Discrete GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                fprintf(stderr,  "\tType:           Virtual GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                fprintf(stderr,  "\tType:           CPU\n");
                break;
            default:
                fprintf(stderr,  "\tType:           Unknown\n");
        }
    }

    /* Pick the device */
    for (const auto& device : devices) {
        if (_isDeviceSuitable(device)) {
            _physicalDevice = device;
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("ERROR failed to find a suitable GPU!");
    }
}

bool VulkanEngine::_isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = _findQueueFamilies(device);

    bool extensionsSupported = _checkDeviceExtensionsSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = _querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        extensionsSupported &&
        deviceFeatures.geometryShader &&
        indices.isComplete() &&
        swapChainAdequate;
}

bool VulkanEngine::_checkDeviceExtensionsSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(_deviceExtensions.begin(), _deviceExtensions.end());

    std::cout << "Available Device extensions:" << std::endl;
    for (const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << std::endl;
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::_debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* layerPrefix,
        const char* msg,
        void* userData)
{
    std::cerr << "[VK] ERROR validation layer: " << msg << std::endl;
    return VK_FALSE;
}

VulkanEngine::QueueFamilyIndices VulkanEngine::_findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

VulkanEngine::SwapChainSupportDetails VulkanEngine::_querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanEngine::_chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    /* VK_FORMAT_UNDEFINED means the surface has no preferred format, choose
     * the one we need */
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    /* If not then try to find if our preferred format is available */
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::_chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::_chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    /* If the window manager requires a specific resolution, use that one */
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    /* Otherwise just choose the one we need that fits in the boundaries */
    VkExtent2D actualExtent = {WIDTH, HEIGHT};

    actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
}

void VulkanEngine::_createSwapChain() {
    SwapChainSupportDetails swapChainSupport = _querySwapChainSupport(_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = _chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = _chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = _chooseSwapExtent(swapChainSupport.capabilities);

    /* Set the number of images in the swap chain */
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    /* Create the swap chain */
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /* Setup the ownership of the swap chain images by the queue families */
    QueueFamilyIndices indices = _findQueueFamilies(_physicalDevice);
    uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    /* No transformation when blitting to window surface */
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    /* Ignore the alpha channel when blitting */
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    /* Enable clipping for obscured pixels in the window surface (best performance) */
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    /* Create the swap chain */
    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, _swapChain.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create swap chain!");
    }

    /* Create chain images */
    vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;
}

void VulkanEngine::_createImageViews() {
    _swapChainImageViews.resize(_swapChainImages.size(), VDeleter<VkImageView>{_device, vkDestroyImageView});

    for (uint32_t i = 0; i < _swapChainImages.size(); i++) {

        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &createInfo, nullptr, _swapChainImageViews[i].replace()) != VK_SUCCESS) {
            throw std::runtime_error("ERROR failed to create image views!");
        }
    }
}

void VulkanEngine::_createGraphicsPipeline() {
    auto vertShaderCode = _readFile("glsl/triangle.vert.spv");
    auto fragShaderCode = _readFile("glsl/triangle.frag.spv");

    VDeleter<VkShaderModule> vertShaderModule{_device, vkDestroyShaderModule};
    VDeleter<VkShaderModule> fragShaderModule{_device, vkDestroyShaderModule};

    _createShaderModule(vertShaderCode, vertShaderModule);
    _createShaderModule(fragShaderCode, fragShaderModule);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChainExtent.width;
    viewport.height = (float) _swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = _swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; /// Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, _pipelineLayout.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, _graphicsPipeline.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create graphics pipeline!");
    }
}

void VulkanEngine::_createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = (uint32_t*) code.data();

    if (vkCreateShaderModule(_device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create shader module!");
    }
}

void VulkanEngine::_createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subPass;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, _renderPass.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create render pass!");
    }
}

void VulkanEngine::_createFramebuffers() {
    _swapChainFramebuffers.resize(_swapChainImageViews.size(), VDeleter<VkFramebuffer>{_device, vkDestroyFramebuffer});

    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            _swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, _swapChainFramebuffers[i].replace()) != VK_SUCCESS) {
            throw std::runtime_error("ERROR failed to create framebuffer!");
        }
    }
}

void VulkanEngine::_createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = _findQueueFamilies(_physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(_device, &poolInfo, nullptr, _commandPool.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create command pool!");
    }
}

void VulkanEngine::_createCommandBuffers() {
    _commandBuffers.resize(_swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) _commandBuffers.size();

    if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to allocate command buffers!");
    }

    for (size_t i = 0; i < _commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        vkBeginCommandBuffer(_commandBuffers[i], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _swapChainExtent;

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

        vkCmdDraw(_commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(_commandBuffers[i]);

        if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("ERROR failed to record command buffer!");
        }
    }
}

void VulkanEngine::_drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(_device, _swapChain, std::numeric_limits<uint64_t>::max(),
            _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(_presentQueue, &presentInfo);
}

void VulkanEngine::_createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, _imageAvailableSemaphore.replace()) != VK_SUCCESS ||
            vkCreateSemaphore(_device, &semaphoreInfo, nullptr, _renderFinishedSemaphore.replace()) != VK_SUCCESS) {
        throw std::runtime_error("ERROR failed to create semaphores!");
    }
}

std::vector<char> VulkanEngine::_readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("ERROR failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
