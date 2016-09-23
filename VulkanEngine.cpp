#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <vector>
#include <cstring>
#include <set>

VkResult CreateDebugReportCallbackEXT(VkInstance instance,
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

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

template <typename T>
class VDeleter {
public:
    VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

    VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VDeleter() {
        cleanup();
    }

    T* operator &() {
        cleanup();
        return &object;
    }

    T* replace() {
        cleanup();
        return &object;
    }

    operator T() const {
        return object;
    }

    void operator=(T rhs) {
        cleanup();
        object = rhs;
    }

    template<typename V>
        bool operator==(V rhs) {
            return object == T(rhs);
        }

private:
    T object{VK_NULL_HANDLE};
    std::function<void(T)> deleter;

    void cleanup() {
        if (object != VK_NULL_HANDLE) {
            deleter(object);
        }
        object = VK_NULL_HANDLE;
    }
};

#ifdef NDEBUG
         const bool enableValidationLayers = false;
#else
         const bool enableValidationLayers = true;
#endif

class VulkanEngine {
    public:
        void run() {
			initWindow();
            initVulkan();
            mainLoop();
        }

    private:
        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600;

        GLFWwindow* _window{NULL};
        VDeleter<VkInstance> _instance {vkDestroyInstance};
        VDeleter<VkDebugReportCallbackEXT> _callbackHandle{_instance, DestroyDebugReportCallbackEXT};
        VDeleter<VkSurfaceKHR> _surface{_instance, vkDestroySurfaceKHR};

        VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};
        VDeleter<VkDevice> _device{vkDestroyDevice};
        VDeleter<VkSwapchainKHR> _swapChain{_device, vkDestroySwapchainKHR};

        VkQueue _graphicsQueue;
        VkQueue _presentQueue;

        std::vector<VkImage> _swapChainImages;
        VkFormat _swapChainImageFormat;
        VkExtent2D _swapChainExtent;

        const std::vector<const char*> _validationLayers = {
            "VK_LAYER_LUNARG_standard_validation"
        };
        const std::vector<const char*> _deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        struct QueueFamilyIndices {
            int graphicsFamily = -1;
            int presentFamily = -2;

            bool isComplete() {
                return graphicsFamily >= 0 && presentFamily >= 0;
            }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        void initWindow() {
            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        }

        void initVulkan() {
            createInstance();
            setupDebugCallback();
            createSurface();
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapChain();
        }

        void mainLoop() {
            while (!glfwWindowShouldClose(_window)) {
                glfwPollEvents();
            }
        }

        void createInstance() {
            /* Check the validation layers */
            if (enableValidationLayers && !checkValidationLayerSupport()) {
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
            auto requiredExtensions = getRequiredExtensions();

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

        void createLogicalDevice()
        {
            QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

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

        void createSurface()
        {
            if (glfwCreateWindowSurface(_instance, _window, nullptr, _surface.replace()) != VK_SUCCESS) {
                throw std::runtime_error("ERROR failed to create window surface!");
            }
        }

        bool checkValidationLayerSupport() {
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

        std::vector<const char*> getRequiredExtensions() {
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

        void setupDebugCallback()
        {
            if (!enableValidationLayers) {
                return;
            }

            VkDebugReportCallbackCreateInfoEXT createInfo = {};

            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            createInfo.pfnCallback = debugCallback;

            if (CreateDebugReportCallbackEXT(_instance, &createInfo, nullptr, &_callbackHandle) != VK_SUCCESS) {
                    throw std::runtime_error("failed to set up debug callback!");
            }
        }

        void pickPhysicalDevice()
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
                if (isDeviceSuitable(device)) {
                    _physicalDevice = device;
                    break;
                }
            }

            if (_physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("ERROR failed to find a suitable GPU!");
            }
        }

        bool isDeviceSuitable(VkPhysicalDevice device)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            QueueFamilyIndices indices = findQueueFamilies(device);

            bool extensionsSupported = checkDeviceExtensionsSupport(device);

            bool swapChainAdequate = false;
            if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }

            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                   extensionsSupported &&
                   deviceFeatures.geometryShader &&
                   indices.isComplete() &&
                   swapChainAdequate;
        }

        bool checkDeviceExtensionsSupport(VkPhysicalDevice device)
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

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
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

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
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

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
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

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

        void createSwapChain() {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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
            QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
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

};

int main() {
    VulkanEngine engine;

    try {
        engine.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
