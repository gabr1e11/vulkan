/**
 * @class   VulkanEngine
 * @brief   Main class for the Vulkan engine, for now
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#pragma once

#include "VulkanApi.hpp"
#include "VDeleter.hpp"
#include <vector>

class VulkanEngine {
    public:
        void run();

    private:
        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600;

        GLFWwindow* _window{NULL};                                           /**> GLFW Window handle */
        VDeleter<VkInstance> _instance {vkDestroyInstance};                  /**> Main Vulkan instance */
        VDeleter<VkDebugReportCallbackEXT>
            _callbackHandle{_instance, DestroyDebugReportCallbackEXT};       /**> Callback handle for the validation layers */
        VDeleter<VkSurfaceKHR> _surface{_instance, vkDestroySurfaceKHR};     /**> Vulkan window surface */

        VkPhysicalDevice _physicalDevice{VK_NULL_HANDLE};                    /**> Vulkan physical device */
        VDeleter<VkDevice> _device{vkDestroyDevice};                         /**> Vulkan logical device */
        VDeleter<VkSwapchainKHR> _swapChain{_device, vkDestroySwapchainKHR}; /**> Swap chain for the logical device */

        VkQueue _graphicsQueue;                                              /**> Graphics commands queue */
        VkQueue _presentQueue;                                               /**> Presentation commands queue */

        std::vector<VkImage> _swapChainImages;                               /**> Images belonging to the swap chain, used
                                                                                  to render the final frame to */
        VkFormat _swapChainImageFormat;                                      /**> Format for the swap chain images */
        VkExtent2D _swapChainExtent;                                         /**> Size of the swap chain images */

        std::vector<VDeleter<VkImageView>> _swapChainImageViews;             /**> View for the swap chain images, used
                                                                                  to access the actual image */

        VDeleter<VkRenderPass> _renderPass{_device, vkDestroyRenderPass};    /**> Render pass??? */
        VDeleter<VkPipelineLayout>
            _pipelineLayout{_device, vkDestroyPipelineLayout};               /**> Layout for the graphics pipeline */
        VDeleter<VkPipeline> _graphicsPipeline{_device, vkDestroyPipeline};  /**> Graphics pipeline instance */

        std::vector<VDeleter<VkFramebuffer>> _swapChainFramebuffers;         /**> Framebuffers associated with the swap chain */

        VDeleter<VkCommandPool> _commandPool{_device, vkDestroyCommandPool}; /**> Command pool used to allocate command buffers */
        std::vector<VkCommandBuffer> _commandBuffers;                        /**> Command buffers to hold presentation and graphics commands */

        VDeleter<VkSemaphore>
            _imageAvailableSemaphore{_device, vkDestroySemaphore};           /**> Semaphore used to signal availability of a swap chain image */
        VDeleter<VkSemaphore>
            _renderFinishedSemaphore{_device, vkDestroySemaphore};           /**> Semaphore used to signal that render has been finished, so
                                                                                  image can be presented */

        const std::vector<const char*> _validationLayers = {                 /**> List of validation layers to be enabled */
            "VK_LAYER_LUNARG_standard_validation"
        };
        const std::vector<const char*> _deviceExtensions = {                 /**> Device extensions to be enabled: */
            VK_KHR_SWAPCHAIN_EXTENSION_NAME                                  /**>    Swap chain is window manager specific, so it needs to be enabled separatedly */
        };

        /**
         * Utility class to find the right family for the
         * graphics and presentation queues
         */
        struct QueueFamilyIndices {
            int graphicsFamily = -1;
            int presentFamily = -2;

            bool isComplete() {
                return graphicsFamily >= 0 && presentFamily >= 0;
            }
        };

        /**
         * Details supported by the swap chain to choose the one we need
         */
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        void _initWindow();
        void _initVulkan();
        void _mainLoop();
        void _createInstance();
        void _createLogicalDevice();
        void _createSurface();
        bool _checkValidationLayerSupport();
        std::vector<const char*> _getRequiredExtensions();
        void _setupDebugCallback();
        void _pickPhysicalDevice();
        bool _isDeviceSuitable(VkPhysicalDevice device);
        bool _checkDeviceExtensionsSupport(VkPhysicalDevice device);
        QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice device);
        SwapChainSupportDetails _querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR _chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR _chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
        VkExtent2D _chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void _createSwapChain();
        void _createImageViews();
        void _createGraphicsPipeline();
        void _createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule);
        void _createRenderPass();
        void _createFramebuffers();
        void _createCommandPool();
        void _createCommandBuffers();
        void _drawFrame();
        void _createSemaphores();

        static std::vector<char> _readFile(const std::string& filename);
        static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(
                VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT objType,
                uint64_t obj,
                size_t location,
                int32_t code,
                const char* layerPrefix,
                const char* msg,
                void* userData);
        static VkResult CreateDebugReportCallbackEXT(VkInstance instance,
                const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDebugReportCallbackEXT* pCallback);
        static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
};
