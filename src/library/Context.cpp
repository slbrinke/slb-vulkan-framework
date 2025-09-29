#include "Context.h"

Context::Context(int width, int height, const char* title, bool enableValidationLayers) {
    createWindow(width, height, title);
    createInstance(title, enableValidationLayers);
    pickPhysicalDevice();
    createLogicalDevice(enableValidationLayers);
    createCommandPool();
}

Context::~Context() {
    m_window = nullptr;
}

std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &Context::getWindow() {
    return m_window;
}

VkSurfaceKHR Context::getSurface() {
    return m_surface;
}

VkDevice Context::getDevice() {
    return m_logicalDevice;
}

VkCommandPool Context::getCommandPool() {
    return m_commandPool;
}

VkQueue Context::getComputeQueue() {
    return m_computeQueue;
}

VkQueue Context::getGraphicsQueue() {
    return m_graphicsQueue;
}

VkQueue Context::getPresentQueue() {
    return m_presentQueue;
}

std::array<uint32_t,2> Context::getQueueFamilyIndices() {
    return {m_queueFamilyIndices.computeAndGraphicsIndex, m_queueFamilyIndices.presentIndex};
}

SwapChainSupport Context::getSwapChainSupport() {
    return querySwapChainSupport(m_physicalDevice);
}

float Context::getMaxSamplerAnisotropy() {
    return m_maxSamplerAnisotropy;
}

VkSampleCountFlagBits Context::getMaxSamples() {
    return m_maxSamples;
}

PFN_vkVoidFunction Context::getExtensionFunction(const char *functionName) {
    return vkGetInstanceProcAddr(m_instance, functionName);
}

uint32_t Context::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
    for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++) {
        if(typeFilter & (1 << i) &&
           (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("CONTEXT ERROR: Could not find a suitable memory type");
}

VkFormat Context::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                      VkFormatFeatureFlags features) {
    for(VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);
        if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("CONTEXT ERROR: Could not find supported format.");
}

VkCommandBuffer Context::startSingleCommand() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Context::endSingleCommand(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &commandBuffer);
}

void Context::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                           VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not create buffer.");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_logicalDevice, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not allocate buffer memory.");
    }

    vkBindBufferMemory(m_logicalDevice, buffer, bufferMemory, 0);
}

void Context::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = startSingleCommand();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleCommand(commandBuffer);
}

void Context::createWindow(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = std::unique_ptr<GLFWwindow, DestroyGLFWwindow>(glfwCreateWindow(width, height, title, nullptr, nullptr));
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

void Context::createInstance(const char* title, bool enableValidationLayers) {
    //check validation layer support
    if(enableValidationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for(const char* layerName : validationLayers) {
            bool layerFound = false;
            for(const auto& layerProperties : availableLayers) {
                if(strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if(!layerFound) {
                throw std::runtime_error("CONTEXT ERROR: Validation layers not supported");
            }
        }
    }

    //init (optional) application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = title;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    //instance create info
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    //gather required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if(enableValidationLayers) {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); //debug information
    }
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    //debug messenger info
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    if(enableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();

        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = debugCallback;
        debugInfo.pUserData = nullptr;
        instanceInfo.pNext = &debugInfo;
    }
    else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
    }

    //create instance
    if(vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not create instance");
    }

    //create debug messenger
    if(enableValidationLayers) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)getExtensionFunction("vkCreateDebugUtilsMessengerEXT");
        if(func == nullptr || func(m_instance, &debugInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("CONTEXT ERROR: Could not set up debug messenger.");
        }
    }    

    //create surface
    if(glfwCreateWindowSurface(m_instance, m_window.get(), nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not create window surface.");
    }
}

QueueFamilyIndices Context::findQueueFamilyIndices(VkPhysicalDevice device) {
    QueueFamilyIndices queueFamilyIndices{};
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    //find graphics queue index
    for(uint32_t i=0; i<queueFamilyCount; i++) {
        if((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
           (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            queueFamilyIndices.computeAndGraphicsFound = true;
            queueFamilyIndices.computeAndGraphicsIndex = i;
            break;
        }
    }
    //find present queue index
    VkBool32 presentSupport = false;
    for(uint32_t i=0; i<queueFamilyCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if(presentSupport) {
            queueFamilyIndices.presentFound = true;
            queueFamilyIndices.presentIndex = i;
            break;
        }
    }
    return queueFamilyIndices;
}

SwapChainSupport Context::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupport details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if(formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if(presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

bool Context::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if(deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || !deviceFeatures.geometryShader) {
        return false;
    }

    QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(device);
    if(!queueFamilyIndices.computeAndGraphicsFound || !queueFamilyIndices.presentFound) {
        return false;
    }

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for(const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    if(!requiredExtensions.empty()) {
        return false;
    }

    SwapChainSupport swapChainSupport = querySwapChainSupport(device);
    if(swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        return false;
    }

    return true;
}

void Context::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if(deviceCount == 0) {
        throw std::runtime_error("CONTEXT ERROR: No physical devices found");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for(const auto &device : devices) {
        if(isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }

    if(m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("CONTEXT ERROR: Could not pick a physical device");

    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
        std::cout << "   CONTEXT: Picked physical device: " << deviceProperties.deviceName << std::endl;

        m_queueFamilyIndices = findQueueFamilyIndices(m_physicalDevice);
        
        m_maxSamplerAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
        VkSampleCountFlags deviceSampleCounts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
        std::vector<VkSampleCountFlagBits> sampleCounts = {
                VK_SAMPLE_COUNT_1_BIT,
                VK_SAMPLE_COUNT_2_BIT,
                VK_SAMPLE_COUNT_4_BIT,
                VK_SAMPLE_COUNT_8_BIT,
                VK_SAMPLE_COUNT_16_BIT,
                VK_SAMPLE_COUNT_32_BIT,
                VK_SAMPLE_COUNT_64_BIT,
        };
        for(auto sampleCount : sampleCounts) {
            if(deviceSampleCounts & sampleCount) {
                m_maxSamples = sampleCount;
            }
        }

        m_timeStampPeriod = deviceProperties.limits.timestampPeriod;
    }
}

void Context::createLogicalDevice(bool enableValidationLayers) {
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::set<uint32_t> queueFamilySet = {m_queueFamilyIndices.computeAndGraphicsIndex, m_queueFamilyIndices.presentIndex};
    for(uint32_t queueFamilyIndex : queueFamilySet) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamilyIndex;
        queueInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueInfo.pQueuePriorities = &queuePriority;
        queueInfos.emplace_back(queueInfo);
    }

    //physical device features information
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &deviceFeatures);
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceMultiviewFeatures multiviewFeatures{};
    multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    deviceFeatures2.pNext = &multiviewFeatures;
    auto func = (PFN_vkGetPhysicalDeviceFeatures2) vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceFeatures2");
    if(func != nullptr) {
        func(m_physicalDevice, &deviceFeatures2);
        deviceInfo.pNext = &deviceFeatures2;

        VkPhysicalDeviceHostQueryResetFeaturesEXT queryReset{};
        queryReset.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
        queryReset.hostQueryReset = VK_TRUE;
        deviceFeatures2.pNext = &queryReset;
    }

    if(enableValidationLayers) {
        deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceInfo.enabledLayerCount = 0;
    }

    //create logical device
    if(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not create logical device");
    }
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.computeAndGraphicsIndex, 0, &m_computeQueue);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.computeAndGraphicsIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentIndex, 0, &m_presentQueue);
}

void Context::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamilyIndices.computeAndGraphicsIndex;
    if(vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("CONTEXT ERROR: Could not create command pool");
    }
}

void Context::cleanUp() {
    if(m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
    }
    if(m_logicalDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(m_logicalDevice, nullptr);
    }
    if(m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
    if(m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)getExtensionFunction("vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(m_instance, nullptr);
}