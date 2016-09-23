#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>

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

    operator T() const {
        return object;
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

class VulkanEngine {
    public:
        void run() {
			initWindow();
            initVulkan();
            mainLoop();
        }

    private:
        const int WIDTH = 800;
        const int HEIGHT = 600;

        void initWindow() {
            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

            _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        }

        void initVulkan() {
            createInstance();
            setupDebugCallback();
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

        GLFWwindow* _window;
        VDeleter<VkInstance> _instance {vkDestroyInstance};
        VDeleter<VkDebugReportCallbackEXT> _callbackHandle{_instance, DestroyDebugReportCallbackEXT};

        const std::vector<const char*> _validationLayers = {
            "VK_LAYER_LUNARG_standard_validation"
        };


#ifdef NDEBUG
         const bool enableValidationLayers = false;
#else
         const bool enableValidationLayers = true;
#endif
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
