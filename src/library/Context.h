#ifndef SLBVULKAN_CONTEXT_H
#define SLBVULKAN_CONTEXT_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <stdexcept>
#include <iostream>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/**
 * Destructor for the glfw window.
 * Required to store it using smart pointers.
 */
struct DestroyGLFWwindow {
    void operator()(GLFWwindow* ptr) {
        glfwDestroyWindow(ptr);
    }
};

/** Names of the relevant vulkan validation layers */
const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

/**
 * Queue family indices suitable for compute, graphics, and present queue.
 * 
 * Always in the context of a physical device.
 * The indices refer to the order in which queue families are listed by vkGetPhysicalDeviceQueueFamilyProperties.
 */
struct QueueFamilyIndices {
    uint32_t computeAndGraphicsIndex = static_cast<uint32_t>(-1); /**< Index of the queue family used for compute and graphics commands */
    bool computeAndGraphicsFound = false; /**< True if the physical device supports a queue family suitable for compute and graphics commands */
    uint32_t presentIndex = static_cast<uint32_t>(-1); /**< Index of the queue family used as present queue */
    bool presentFound = false; /**< True if the physical device supports a queue family suitable as a present queue */
};

/**
 * Properties of a physical device relevant for the creation of a swap chain.
 */
struct SwapChainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/** Names of the extensions the physical device has to support. */
const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

/**
 * Vulkan context required for all simulation and rendering.
 * 
 * Manages glfw window and vulkan surface and sets up instance, device, etc.
 * Code mainly follows the vulkan tutorial by Alexander Overvoorde: https://vulkan-tutorial.com/
 */
class Context {
public:
    /**
     * Initialize render context with instance, devices, window, and surface.
     * 
     * @param width width of the display window in number of pixels
     * @param height height of the display window in number of pixels
     * @param title name displayed in the window header and stored in the application info
     * @param enableValidationLayers if true vulkan validation layers are activated
     */
    Context(int width, int height, const char* title, bool enableValidationLayers = true);
    ~Context();

    std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &getWindow();
    VkDevice getDevice();

    /**
     * Destroy all vulkan components.
     */
    void cleanUp();

private:
    /**
     * Create a glfw window to render to.
     * 
     * @param width width of the window in number of pixels
     * @param height height of the window in number of pixels
     * @param title name displayed in the window header
     */
    void createWindow(int width, int height, const char* title);

    /**
     * Create vulkan instance, debug messenger, and surface.
     * 
     * Instance extensions required by glfw and debug messenger are enabled.
     * If validation layers are enabled a debug messenger is set up with debugCallback.
     * Lastly the vulkan surface is created.
     * 
     * @param title name stored in the application info
     * @param enableValidationLayers if true vulkan validation layers are activated
     */
    void createInstance(const char* title, bool enableValidationLayers);

    /**
     * Query the queue families supported by a physical device.
     * 
     * Find queue families suitable for compute, graphics, and present queue.
     * Rendering and compute use the same queue to keep synchronization simple.
     * Present is handled by a separate queue.
     * 
     * @param device physical device checked for queue family support
     * @return struct containing the queue family indices if found
     */
    QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device);

    /**
     * Check if a physical device is suitable for swap chain creation.
     * 
     * @param device physical device checked for swap chain support
     */
    SwapChainSupport querySwapChainSupport(VkPhysicalDevice device);

    /**
     * Check if a physical device can be used in this context.
     * 
     * The device has to be a GPU that supports geometry shaders.
     * It has to provide the necessary queues and swap chain support and support the required device extensions.
     * 
     * @param device physical device
     */
    bool isDeviceSuitable(VkPhysicalDevice device);

    /**
     * Find a suitable physical device on the computer.
     * 
     * Additional information such as maximum sampler anisotropy, maximum msaa samples, and timestamp period is stored.
     */
    void pickPhysicalDevice();

    /**
     * Create logical device to communicate with the physical device.
     * 
     * Device features that are enabled include:
     * multiview rendering to be able to render to multiple image views in a single renderpass,
     * query reset to measure rendering time.
     * After creation the three queues are requested from the logical device.
     * 
     * @param enableValidationLayers if true vulkan validation layers are activated
     */
    void createLogicalDevice(bool enableValidationLayers);

    /**
     * Create a command pool that compute and graphics commands can be allocated from later.
     */
    void createCommandPool();

    VkInstance m_instance = VK_NULL_HANDLE; /**< Vulkan instance as a base for the entire application */
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE; /**< Vulkan debug messenger handling the callback for debug output */
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; /**< Vulkan representation of the physical device the application runs on */
    VkDevice m_logicalDevice = VK_NULL_HANDLE; /**< Logical device as interface for the physical device */

    std::unique_ptr<GLFWwindow, DestroyGLFWwindow> m_window = nullptr; /**< Glfw window to display rendered images */
    VkSurfaceKHR m_surface = VK_NULL_HANDLE; /**< Vulkan surface connecting rendered images with the output window */

    QueueFamilyIndices m_queueFamilyIndices; /**< Vulkan queue family indices of the selected physical device */
    VkQueue m_computeQueue = VK_NULL_HANDLE; /**< Vulkan queue used for compute commands */
    VkQueue m_graphicsQueue = VK_NULL_HANDLE; /**< Vulkan queue used for graphics commands */
    VkQueue m_presentQueue = VK_NULL_HANDLE; /**< Vulkan present queue */

    VkCommandPool m_commandPool = VK_NULL_HANDLE; /**< Pool to allocate vulkan commands from. */

    float m_maxSamplerAnisotropy = 0.0f; /**< Maximum number of samples used when sampling a texture */
    VkSampleCountFlagBits m_maxSamples = VK_SAMPLE_COUNT_1_BIT; /**< Maximum number of samples used for MSAA */
    float m_timeStampPeriod = 0.0f; /**< Number of nanoseconds required for a timestamp to be incremented by 1 */
};

#endif //SLBVULKAN_CONTEXT_H