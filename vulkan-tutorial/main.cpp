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
#include <fstream>

#ifdef DEBUG
const bool enableValidation = true;
#else
const bool enableValidation = false;
#endif


static const std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
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


static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file!");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
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
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
        createSemaphores();

        std::cout << "App Initialized" << std::endl;
    }

    ~HelloTriangleApplication() {
        std::cout << "App Destroying" << std::endl;

        mDevice.waitIdle();

        mDevice.destroySemaphore(mSemaphoreImageReady);
        mDevice.destroySemaphore(mSemaphoreRenderFinished);

        mDevice.destroyCommandPool(mCommandPool);
        for (auto framebuffer: mFramebuffers)
            mDevice.destroyFramebuffer(framebuffer);

        mDevice.destroyPipeline(mGraphicsPipeline);
        mDevice.destroyPipelineLayout(mPipelineLayout);
        mDevice.destroyRenderPass(mRenderPass);
        mDevice.destroyShaderModule(mVertShaderModule);
        mDevice.destroyShaderModule(mFragShaderModule);

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
            drawFrame();
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
    std::vector<vk::Framebuffer> mFramebuffers;

    vk::ShaderModule mVertShaderModule;
    vk::ShaderModule mFragShaderModule;

    vk::PipelineLayout mPipelineLayout;
    vk::RenderPass mRenderPass;
    vk::Pipeline mGraphicsPipeline;

    vk::CommandPool mCommandPool;
    std::vector<vk::CommandBuffer> mCommandBuffers;

    vk::Semaphore mSemaphoreImageReady;
    vk::Semaphore mSemaphoreRenderFinished;

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
                VK_API_VERSION_1_1
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
        const float queuePriority = 1.0;

        for (auto queueFamily : uniqueQueueFamilies)
            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));

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
        if (std::count(availablePresentModes.begin(), availablePresentModes.end(), vk::PresentModeKHR::eMailbox))
            return vk::PresentModeKHR::eMailbox;
        return vk::PresentModeKHR::eFifo;
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

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment(
                {}, mSwapchainFormat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
        vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass({},vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef);
        vk::SubpassDependency dependency(
                VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits(0), vk::AccessFlagBits::eColorAttachmentWrite);
        mRenderPass = mDevice.createRenderPass(vk::RenderPassCreateInfo(
                {}, 1, &colorAttachment, 1, &subpass, 1, &dependency));
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("../vulkan-tutorial/shaders/vert.spv");
        auto fragShaderCode = readFile("../vulkan-tutorial/shaders/frag.spv");

        mVertShaderModule = createShaderModule(vertShaderCode);
        mFragShaderModule = createShaderModule(fragShaderCode);

        auto vertShaderStageInfo = vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eVertex, mVertShaderModule, "main");
        auto fragShaderStageInfo = vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eFragment, mFragShaderModule, "main");

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{vertShaderStageInfo, fragShaderStageInfo};

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);
        vk::PipelineInputAssemblyStateCreateInfo  inputAssembly({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

        vk::Viewport viewport(0.f, 0.f, mSwapchainExtent.width, mSwapchainExtent.height, 0.f, 1.f);
        vk::Rect2D scissor({0, 0}, mSwapchainExtent);
        vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer({
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .depthBiasConstantFactor = 0.0f, // Optional
                .depthBiasClamp = 0.0f, // Optional
                .depthBiasSlopeFactor = 0.0f, // Optional
                .lineWidth = 1.0f,
        });

        vk::PipelineMultisampleStateCreateInfo multisampling({
                 .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                 .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                 .sampleShadingEnable = VK_FALSE,
                 .minSampleShading = 1.0f, // Optional
                 .pSampleMask = nullptr, // Optional
                 .alphaToCoverageEnable = VK_FALSE, // Optional
                 .alphaToOneEnable = VK_FALSE, // Optional
        });

        vk::PipelineColorBlendAttachmentState colorBlendAttachment({
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
//            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
//            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
//            .colorBlendOp = VK_BLEND_OP_ADD, // Optional
//            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
//            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
//            .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
        });

        vk::PipelineColorBlendStateCreateInfo colorBlending({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
//            .logicOp = VK_LOGIC_OP_COPY, // Optional
            .attachmentCount = 1,
            .pAttachments = (VkPipelineColorBlendAttachmentState*)(&colorBlendAttachment),
//            .blendConstants = {0.f, 0.f, 0.f, 0.f}, // Optional
        });

        mPipelineLayout = mDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo(
                {}, 0, nullptr, 0, nullptr));


        vk::GraphicsPipelineCreateInfo pipelineInfo(
                {}, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
                &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending,
                nullptr, mPipelineLayout,mRenderPass, 0,
                nullptr, -1);

        auto graphicsPipeline = mDevice.createGraphicsPipeline(nullptr, pipelineInfo);
        switch (graphicsPipeline.result) {
            case vk::Result::eSuccess:
                mGraphicsPipeline = graphicsPipeline.value;
                break;
            default:
                throw std::runtime_error("fail to create graphics pipeline");
        }
    }

    void createFramebuffers() {
        mFramebuffers.resize(mSwapchainImageViews.size());
        for (size_t i = 0; i < mSwapchainImageViews.size(); i++) {
            std::vector<vk::ImageView> attachments = { mSwapchainImageViews[i] };
            vk::FramebufferCreateInfo framebufferInfo(
                    {}, mRenderPass, attachments, mSwapchainExtent.width, mSwapchainExtent.height, 1);

            mFramebuffers[i] = mDevice.createFramebuffer(framebufferInfo);
        }
    }

    vk::ShaderModule createShaderModule(const std::vector<char>& code) {
        return mDevice.createShaderModule({{}, code.size(), reinterpret_cast<const uint32_t*>(code.data())});
    }

    void createCommandBuffers() {
        auto queueFamilyIndices = findQueueFamilyIndices(mPhysicalDevice);
        mCommandPool = mDevice.createCommandPool({{}, queueFamilyIndices.graphicsFamily.value()});
        mCommandBuffers = mDevice.allocateCommandBuffers({
            mCommandPool, vk::CommandBufferLevel::ePrimary,
            static_cast<uint32_t>(mFramebuffers.size())});

        for (size_t i = 0; i < mCommandBuffers.size(); i++) {
            auto& commandBuffer = mCommandBuffers[i];
            commandBuffer.begin({{}, nullptr});

            vk::ClearValue clearColor(std::array<float, 4>({0.f, 0.f, 0.f, 1.f}));
            commandBuffer.beginRenderPass({
                mRenderPass, mFramebuffers[i],{{0, 0}, mSwapchainExtent},
                1, &clearColor}, vk::SubpassContents::eInline);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);
            commandBuffer.draw(3, 1, 0, 0);
            commandBuffer.endRenderPass();
            commandBuffer.end();
        }
    }

    void createSemaphores() {
        mSemaphoreImageReady = mDevice.createSemaphore({});
        mSemaphoreRenderFinished = mDevice.createSemaphore({});
    }

    void drawFrame() {
        uint32_t imageIdx;
        auto imageIdx_ = mDevice.acquireNextImageKHR(mSwapchain, UINT64_MAX, mSemaphoreImageReady, {});
        switch (imageIdx_.result) {
            case vk::Result::eSuccess:
                imageIdx = imageIdx_.value;
                break;
            default:
                throw std::runtime_error("fail to acquire next image");
        }

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submitInfo(
                1, &mSemaphoreImageReady, waitStages,
                1, &mCommandBuffers[imageIdx],
                1, &mSemaphoreRenderFinished);
        mGraphicsQueue.submit({submitInfo});

        vk::PresentInfoKHR presentInfo(
                1, &mSemaphoreRenderFinished, 1, &mSwapchain, &imageIdx, nullptr);
        auto result = mPresentQueue.presentKHR(presentInfo);
        switch (result) {
            case vk::Result::eSuccess:
                break;
            default:
                throw std::runtime_error("fail to present");
        }
        mPresentQueue.waitIdle();
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