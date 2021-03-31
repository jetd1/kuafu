//
// Created by jet on 3/29/21.
//
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <set>

static const int NVIDIA_VENDOR_ID = 0x10DE;

#ifdef DEBUG
const bool enableValidation = true;
#else
const bool enableValidation = false;
#endif


static const std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        throw vk::ExtensionNotPresentError("vkCreateDebugUtilsMessengerEXT not present");
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() and presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication(int width = 800, int height = 800) {
        std::cout << "App Initializing" << std::endl;

        initWindow(width, height);
        createInstance();
        initDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();

        std::cout << "App Initialized" << std::endl;
    }

    ~HelloTriangleApplication() {
        std::cout << "App Destroying" << std::endl;

        for (auto imageView: mSwapchainImageViews)
            mDevice.destroyImageView(imageView);

        mDevice.destroySwapchainKHR(mSwapchain);
        mDevice.destroy();
        if (enableValidation)
            DestroyDebugUtilsMessengerEXT(mInstance, mMessengerEXT, nullptr);
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mInstance.destroy();
        glfwDestroyWindow(mWindow);
        glfwTerminate();

        std::cout << "App Destroyed" << std::endl;
    }

    void run() {
        while (!glfwWindowShouldClose(mWindow)) {
            glfwPollEvents();
        }
    }

private:
    GLFWwindow* mWindow;
    vk::Instance mInstance;

    vk::Device mDevice;
    vk::PhysicalDevice mPhysicalDevice;

    vk::Queue mGraphicsQueue;
    vk::Queue mPresentQueue;

    vk::SurfaceKHR mSurface;
    vk::SwapchainKHR mSwapchain;
    vk::Format mSwapchainFormat;
    vk::Extent2D mSwapchainExtent;
    std::vector<vk::Image> mSwapchainImages;
    std::vector<vk::ImageView> mSwapchainImageViews;

    VkDebugUtilsMessengerEXT mMessengerEXT;

    void initWindow(int width, int height) {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        mWindow = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    }

    void initDebugMessenger() {
        if (!enableValidation)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        assert(CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mMessengerEXT) == VK_SUCCESS);
    }

    void createInstance() {
        vk::ApplicationInfo appInfo(
                "Hello Triangle",
                VK_MAKE_VERSION(1, 0, 0),
                "No Engine",
                VK_MAKE_VERSION(1, 0, 0),
                VK_API_VERSION_1_0
        );
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        std::vector<const char*> layers;

        if (enableValidation) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
        vk::InstanceCreateInfo createInfo({}, &appInfo, layers, extensions);
        if (enableValidation) {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
        }
        mInstance = vk::createInstance(createInfo);
    }

    void pickPhysicalDevice() {
        auto physicalDevices = mInstance.enumeratePhysicalDevices();
        for (const auto& physicalDevice: physicalDevices)
            if (isDeviceSuitable(physicalDevice)) {
                mPhysicalDevice = physicalDevice;
                std::cout << "Using " << physicalDevice.getProperties().deviceName << std::endl;
                break;
            }

        if (mPhysicalDevice == static_cast<void*>(VK_NULL_HANDLE))
            throw std::runtime_error("no valid device found!");
    }

    bool isDeviceSuitable(const vk::PhysicalDevice& device) {
        auto deviceProperties = device.getProperties();
        auto deviceFeatures = device.getFeatures();

        return findQueueFamilyIndices(device).isComplete() \
           and checkDeviceExtensionSupport(device) \
           and isSwapChainSupportAdequate(device);
    }

    bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
        auto availableExtensions = device.enumerateDeviceExtensionProperties();

        std::set<std::string> unsatisfiedExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
        for (const auto& extension: availableExtensions)
            unsatisfiedExtensions.erase(extension.extensionName);

        return unsatisfiedExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilyIndices(const vk::PhysicalDevice& device) {
        QueueFamilyIndices indices;
        auto queueFamilyProperties = device.getQueueFamilyProperties();

        int idx = 0;
        for (const auto& queueFamilyProperty: queueFamilyProperties) {
            if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics)
                indices.graphicsFamily = idx;

            if (device.getSurfaceSupportKHR(idx, mSurface))
                indices.presentFamily = idx;

            if (indices.isComplete())
                break;

            idx++;
        }

        return indices;
    }

    void createLogicalDevice() {
        auto indices = findQueueFamilyIndices(mPhysicalDevice);

        std::set<uint32_t> uniqueQueueFamilies = {
                indices.graphicsFamily.value(),
                indices.presentFamily.value()
        };
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        for (auto queueFamily : uniqueQueueFamilies)
            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1));

        auto deviceCreateInfo = vk::DeviceCreateInfo({}, queueCreateInfos, {}, requiredDeviceExtensions, {});
        mDevice = mPhysicalDevice.createDevice(deviceCreateInfo);
        mGraphicsQueue = mDevice.getQueue(indices.graphicsFamily.value(), 0);
        mPresentQueue = mDevice.getQueue(indices.presentFamily.value(), 0);
    }

    void createSurface() {
        VkSurfaceKHR tmpSurface;
        if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &tmpSurface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
        mSurface = tmpSurface;
    }

    SwapChainSupportDetails getSwapChainSupportDetails(const vk::PhysicalDevice& device) {
        return {
                .capabilities = device.getSurfaceCapabilitiesKHR(mSurface),
                .formats = device.getSurfaceFormatsKHR(mSurface),
                .presentModes = device.getSurfacePresentModesKHR(mSurface)
        };
    }

    bool isSwapChainSupportAdequate(const vk::PhysicalDevice& device) {
        auto details = getSwapChainSupportDetails(device);
        return !details.formats.empty() && !details.presentModes.empty();
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb
             && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        throw std::runtime_error("no supported surfaces format found");
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        if (!std::count(availablePresentModes.begin(), availablePresentModes.end(), vk::PresentModeKHR::eMailbox))
            return vk::PresentModeKHR::eFifo;
        return vk::PresentModeKHR::eMailbox;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(mWindow, &width, &height);

            vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    void createSwapChain() {
        auto swapchainSupport = getSwapChainSupportDetails(mPhysicalDevice);
        auto surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
        auto presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
        mSwapchainExtent = chooseSwapExtent(swapchainSupport.capabilities);
        mSwapchainFormat = surfaceFormat.format;

        auto imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
            imageCount = swapchainSupport.capabilities.maxImageCount;

        auto queueFamilyIndices_ = findQueueFamilyIndices(mPhysicalDevice);
        uint32_t queueFamilyIndices[] = {queueFamilyIndices_.graphicsFamily.value(), queueFamilyIndices_.presentFamily.value()};

        vk::SharingMode imageSharingMode = vk::SharingMode::eExclusive;
        uint32_t queueFamilyIndexCount = 0;
        uint32_t* pQueueFamilyIndices = nullptr;

        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            imageSharingMode = vk::SharingMode::eConcurrent;
            queueFamilyIndexCount = 2;
            pQueueFamilyIndices = queueFamilyIndices;
        }

        auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR(
                {}, mSurface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, mSwapchainExtent,
                1, vk::ImageUsageFlagBits::eColorAttachment, imageSharingMode, queueFamilyIndexCount, pQueueFamilyIndices,
                swapchainSupport.capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, VK_TRUE, nullptr
        );
        mSwapchain = mDevice.createSwapchainKHR(swapchainCreateInfo);
        mSwapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);
    }

    void createImageViews() {
        mSwapchainImageViews.resize(mSwapchainImages.size());
        for (size_t i = 0; i < mSwapchainImages.size(); i++) {
            auto imageViewCreateInfo = vk::ImageViewCreateInfo(
                    {}, mSwapchainImages[i], vk::ImageViewType::e2D, mSwapchainFormat, vk::ComponentMapping{},
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            mSwapchainImageViews[i] = mDevice.createImageView(imageViewCreateInfo);
        }
    }

    void createGraphicsPipeline() {

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}