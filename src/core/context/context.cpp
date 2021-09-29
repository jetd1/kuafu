//
// Modified by Jet <i@jetd.me> based on Rayex source code.
// Original copyright notice:
//
// Copyright (c) 2021 Christian Hilpert
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the author be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose
// and to alter it and redistribute it freely, subject to the following
// restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#include "core/context/context.hpp"
#include "core/context/global.hpp"

#define STB_IMAGE_IMPLEMENTATION

#ifndef VK_KHR_acceleration_structure
#error "The local Vulkan SDK does not support VK_KHR_acceleration_structure."
#endif

#if !defined( VULKAN_HPP_STORAGE_SHARED )
#define VULKAN_HPP_STORAGE_SHARED
#define VULKAN_HPP_STORAGE_SHARED_EXPORT
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace kuafu {
/// @todo Currently always build with debug utils because an error might cause instant
#ifdef VK_VALIDATION
const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};
std::vector<const char *> extensions = {"VK_EXT_debug_utils"};
#else
const std::vector<const char *> layers = {};
std::vector<const char *> extensions = {};
#endif

std::vector<const char *> deviceExtensions = {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                              VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                              VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                              VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                              VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                              VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                              VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                              VK_KHR_SHADER_CLOCK_EXTENSION_NAME,
                                              VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                                              VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
                                              };

size_t currentFrame = 0;
size_t prevFrame = 0;

Context::~Context() {
    try {                                             // FIXME
        vkCore::global::device.waitIdle();
    } catch (const vk::DeviceLostError& e) {
        KF_WARN("Device lost while quitting. Longer wait time is expected.");
    }
    mScenes.clear();    // maybe not necessary

    // Gui needs to be destroyed manually, as RAII destruction will not be possible.
    if (pGui != nullptr)
      pGui->destroy();

    if (pConfig->mUseDenoiser)
        mDenoiser.destroy();
}

void Context::setGui(const std::shared_ptr<Gui> &gui, bool initialize) {
    if (pGui != nullptr) {
        recreateSwapchain();
        pGui->destroy();
    }

    pGui = gui;

    if (initialize)
        initGui();
}

void Context::init() {
    // Retrieve and add window extensions to other extensions.
    if (pConfig->mPresent) {
      auto windowExtensions = pWindow->getExtensions();
      extensions.insert(extensions.end(), windowExtensions.begin(), windowExtensions.end());

      deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    } else {
//        vkCore::global::dataCopies = 1U;
//        vkCore::global::swapchainImageCount = 1U;

        vkCore::global::dataCopies = pConfig->mMaxImagesInFlight;
        vkCore::global::swapchainImageCount = pConfig->mMaxImagesInFlight;
    }

    if (pConfig->mUseDenoiser) {
        global::logger->warn("Denoiser ON! You must have an NVIDIA GPU with driver version > 470 installed.");

        deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME);

        deviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);

        mDenoiser.initOptiX(
                OPTIX_DENOISER_INPUT_RGB_ALBEDO_NORMAL,
                OPTIX_PIXEL_FORMAT_FLOAT4,
                false);                                 // TODO
        KF_DEBUG("OptiX initialized!");
    }

    // Instance
    mInstance = vkCore::initInstanceUnique(layers, extensions, VK_API_VERSION_1_2);
    KF_DEBUG("Instance initialized!");

#ifdef VK_VALIDATION
    // Debug messenger
    mDebugMessenger.init();
    KF_DEBUG("DebugMessenger initialized!");
#endif

    // Surface
    if (pConfig->mPresent) {
        VkSurfaceKHR surface;
        SDL_bool result = SDL_Vulkan_CreateSurface(
                pWindow->get(), static_cast<VkInstance>(vkCore::global::instance), &surface);
        if (result != SDL_TRUE)
            throw std::runtime_error("Failed to create surface");

        mSurface.init(vk::SurfaceKHR(surface), pWindow->getSize());
        KF_DEBUG("Surface initialized!");
    }

    // Physical device
    vkCore::global::physicalDevice = vkCore::initPhysicalDevice(); // @todo This function does not check if any feature is available when evaluating a device. Additionally, it is pointless to assign vkCore::global::physicalDevice in here because it doesn't need a unique handle.
    KF_DEBUG("physicalDevice initialized!");

    // Reassess the support of the preferred surface settings.
    if (pConfig->mPresent)
        mSurface.assessSettings();

    // Queues
    vkCore::initQueueFamilyIndices();
    KF_DEBUG("QueueFamilyIndices initialized!");

    // Logical device
    vk::PhysicalDeviceSynchronization2FeaturesKHR synchronization2Features;
    synchronization2Features.synchronization2 = VK_TRUE;
    synchronization2Features.pNext = nullptr;

    vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures;
    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
    timelineSemaphoreFeatures.pNext = &synchronization2Features;

    vk::PhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures;
    shaderClockFeatures.shaderSubgroupClock = VK_TRUE;
    shaderClockFeatures.pNext = &timelineSemaphoreFeatures;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeatures;
    asFeatures.accelerationStructure = VK_TRUE;
    asFeatures.pNext = &shaderClockFeatures;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures;
    rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
    rtPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
    rtPipelineFeatures.rayTraversalPrimitiveCulling = VK_TRUE;
    rtPipelineFeatures.pNext = &asFeatures;

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
    indexingFeatures.runtimeDescriptorArray = VK_TRUE;
    indexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
    indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    indexingFeatures.pNext = &rtPipelineFeatures;

    vk::PhysicalDeviceRobustness2FeaturesEXT robustness2FeaturesEXT;
    robustness2FeaturesEXT.nullDescriptor = VK_TRUE;
    robustness2FeaturesEXT.pNext = &indexingFeatures;

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.pNext = &robustness2FeaturesEXT;

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.shaderInt64 = VK_TRUE;

    vk::PhysicalDeviceFeatures2 deviceFeatures2{deviceFeatures};
    deviceFeatures2.pNext = &bufferDeviceAddressFeatures;

    mDevice = vkCore::initDeviceUnique(deviceExtensions, {}, deviceFeatures2);
    KF_DEBUG("Device initialized!");

    vkCore::global::device = mDevice.get();

    // Retrieve all queue handles.
    vkCore::global::device.getQueue(vkCore::global::graphicsFamilyIndex, 0, &vkCore::global::graphicsQueue);
    vkCore::global::device.getQueue(vkCore::global::transferFamilyIndex, 0, &vkCore::global::transferQueue);

    // Command pools
    mGraphicsCmdPool = vkCore::initCommandPoolUnique(vkCore::global::graphicsFamilyIndex,
                                                     vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    KF_DEBUG("GraphicsCmdPool initialized!");
    vkCore::global::graphicsCmdPool = mGraphicsCmdPool.get();

    mTransferCmdPool = vkCore::initCommandPoolUnique(vkCore::global::transferFamilyIndex, {});
    KF_DEBUG("TransferCmdPool initialized!");
    vkCore::global::transferCmdPool = mTransferCmdPool.get();

    // Post processing renderer
    //mPostProcessingRenderer.initDepthImage(getExtent());
    mPostProcessingRenderer.initRenderPass(getFormat());
    KF_DEBUG("RenderPass initialized!");

    // Swapchain
    if (pConfig->mPresent) {
        mSwapchain.init(&mSurface, mPostProcessingRenderer.getRenderPass().get());
        KF_DEBUG("Swapchain initialized!");
        pConfig->mSwapchainNeedsRefresh = false;
    } else {
        getCamera()->mFrames->init(
                pConfig->mMaxImagesInFlight,
                getExtent(), getFormat(), getColorSpace(),
                mPostProcessingRenderer.getRenderPass().get());
    }

    // GUI
    initGui();
    KF_DEBUG("Gui initialized!");

    // Create fences and semaphores.
    mSync.init(pConfig->mPresent ? 2 : pConfig->mMaxImagesInFlight);
    KF_DEBUG("Sync initialized!");

    // Path tracer
    mRayTracer.init();
    KF_DEBUG("RayTracer initialized!");
    pConfig->mMaxPathDepth = mRayTracer.getCapabilities().pipelineProperties.maxRayRecursionDepth;

    // Descriptor sets and layouts
    mRayTracer.initDescriptorSet();
    mCurrentScene->init();
    KF_DEBUG("Descriptors initialized!");

    // Initialize the path tracing pipeline.
    initPipelines();
    KF_DEBUG("Pipelines initialized!");

    mRayTracer.setRenderTargets(getCamera()->getRenderTargets());
    mRayTracer.createStorageImage(getExtent());
    KF_DEBUG("Images initialized!");

    mRayTracer.createShaderBindingTable();
    KF_DEBUG("ShaderBindingTable initialized!");

    // Denoiser
    if (pConfig->mUseDenoiser)
        mDenoiser.allocateBuffers(getExtent());
    else {
        vk::SemaphoreTypeCreateInfo timelineCreateInfo;
        timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
        timelineCreateInfo.initialValue  = 0;

        vk::SemaphoreCreateInfo sci;
        sci.pNext        = &timelineCreateInfo;
        sci.flags        = vk::SemaphoreCreateFlagBits(0);

        mCmdSemaphore = vkCore::global::device.createSemaphoreUnique(sci);
    }

    // Post processing renderer
    mPostProcessingRenderer.initDescriptorSet();
    mPostProcessingRenderer.initPipeline();
    mPostProcessingRenderer.updateDescriptors(mRayTracer.getStorageImageInfo("rgba"));
    KF_DEBUG("PostProcessingRenderer initialized!");

    // Initialize command buffers.
    mCommandBuffers.init(mGraphicsCmdPool.get(), vkCore::global::swapchainImageCount,
                         vk::CommandBufferUsageFlagBits::eRenderPassContinue);
    mCommandBuffers2.init(mGraphicsCmdPool.get(), vkCore::global::swapchainImageCount,
                          vk::CommandBufferUsageFlagBits::eRenderPassContinue);
    KF_DEBUG("CommandBuffers initialized!");
}

void Context::update() {
    updateSettings();

    auto imageIndex = getCurrentImageIndex();
    auto maxFramesInFlight = static_cast<uint32_t>(mSync.getMaxFramesInFlight());

    // If the scene is empty add a dummy triangle so that the acceleration structures can be built successfully.

    // Move dummy behind camera
    if (mCurrentScene->mDummy) {
        mCurrentScene->translateDummy();
    }

    if (mCurrentScene->mGeometryInstances.empty()) {
        mCurrentScene->addDummy();
    } else if (mCurrentScene->mDummy) {
        mCurrentScene->removeDummy();
    }

    if (mCurrentScene->mUploadEnvironmentMap) {
        mSync.waitForFrame(prevFrame);
        mCurrentScene->uploadEnvironmentMap();
        mCurrentScene->updateSceneDescriptors();
    }

    if (mCurrentScene->mUploadGeometries) {                // will upload active light tex in this step
        mCurrentScene->uploadGeometries();
        mCurrentScene->updateGeometryDescriptors();
    }

    if (mCurrentScene->mUploadGeometryInstancesToBuffer) {
        mCurrentScene->uploadGeometryInstances();

        // @TODO Try to call this as few times as possible.
        mRayTracer.createBottomLevelAS(mCurrentScene->mVertexBuffers, mCurrentScene->mIndexBuffers, mCurrentScene->mGeometries);
        mRayTracer.buildTlas(mCurrentScene->mGeometryInstances,
                             vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                             vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
        mRayTracer.updateDescriptors();
    } else {
        mRayTracer.updateTlas(mCurrentScene->mGeometryInstances,
                              vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                              vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
    }

    mCurrentScene->uploadUniformBuffers(imageIndex % maxFramesInFlight);

    // Increment frame counter for jitter cam.
    if (pConfig->mAccumulateFrames)
        ++global::frameCount;
    else
        global::frameCount = -1;
}

void Context::prepareFrame() {
    if (pConfig->mPresent) {
        mSwapchain.acquireNextImage(mSync.getImageAvailableSemaphore(currentFrame), nullptr);
    } else {
//        mFrames.acquireNextImage(mSync.getImageAvailableSemaphore(currentFrame));
        getCamera()->mFrames->acquireNextImage();            // FIXME
    }
}

void Context::submitWithTLSemaphore(const vk::CommandBuffer& cmdBuf)
{
    auto imageIndex = static_cast<size_t>(getCurrentImageIndex());
    if (mSync.getImageInFlight(imageIndex))
        mSync.waitForFrame(currentFrame);

    mSync.getImageInFlight(imageIndex) = mSync.getInFlightFence(currentFrame);
    auto currentInFlightFence = mSync.getInFlightFence(currentFrame);
    auto result = vkCore::global::device.resetFences(1, &currentInFlightFence);
    KF_ASSERT(result == vk::Result::eSuccess, "Failed to reset fences");

    // Increment for signaling
    mFenceValue++;

    vk::CommandBufferSubmitInfoKHR cmdBufInfo;
    cmdBufInfo.setCommandBuffer(cmdBuf);

    vk::SemaphoreSubmitInfoKHR waitSemaphore;
    waitSemaphore.setSemaphore(mSync.getImageAvailableSemaphore(currentFrame));
    waitSemaphore.setStageMask(vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput);

    vk::SemaphoreSubmitInfoKHR signalSemaphore;
    signalSemaphore.setSemaphore(getCmdSemaphore());
    signalSemaphore.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
    signalSemaphore.setValue(mFenceValue);

    vk::SubmitInfo2KHR submits;
    submits.setCommandBufferInfos(cmdBufInfo);
    if (pConfig->mPresent)                                 // TODO: FIXME
        submits.setWaitSemaphoreInfos(waitSemaphore);      // TODO: FIXME
    submits.setSignalSemaphoreInfos(signalSemaphore);

    vkCore::global::graphicsQueue.submit2KHR(submits);
}


//--------------------------------------------------------------------------------------------------
// Convenient function to call for submitting the rendering command
//
void Context::submitFrame(const vk::CommandBuffer& cmdBuf)
{
    auto currentInFlightFence = mSync.getInFlightFence(currentFrame);
    auto currentFinishedSemaphore = mSync.getFinishedRenderSemaphore(currentFrame);

    vk::CommandBufferSubmitInfoKHR cmdBufInfo;
    cmdBufInfo.setCommandBuffer(cmdBuf);

    vk::SemaphoreSubmitInfoKHR waitSemaphore;
    waitSemaphore.setSemaphore(getCmdSemaphore());
    waitSemaphore.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);
    waitSemaphore.setValue(mFenceValue);

    vk::SemaphoreSubmitInfoKHR signalSemaphore;
    signalSemaphore.setSemaphore(currentFinishedSemaphore);
    signalSemaphore.setStageMask(vk::PipelineStageFlagBits2KHR::eAllCommands);

    vk::SubmitInfo2KHR submits;
    submits.setCommandBufferInfos(cmdBufInfo);
    submits.setWaitSemaphoreInfos(waitSemaphore);
    submits.setSignalSemaphoreInfos(signalSemaphore);

    vkCore::global::graphicsQueue.submit2KHR(submits, currentInFlightFence);


    if (pConfig->mPresent) {
        uint32_t imageIndex = getCurrentImageIndex();

        vk::PresentInfoKHR presentInfo(1,                          // waitSemaphoreCount
                                       &currentFinishedSemaphore,          // pWaitSemaphores
                                       1,                          // swapchainCount
                                       &vkCore::global::swapchain, // pSwapchains
                                       &imageIndex,                // pImageIndices
                                       nullptr);                  // pResults

        try {
            vk::Result result = vkCore::global::graphicsQueue.presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
                pConfig->triggerSwapchainRefresh();
                KF_WARN("Swapchain out of data or suboptimal.");
            }
        }
        catch (...) {
            pConfig->triggerSwapchainRefresh();
        }
    } else {
        vk::PipelineStageFlags pWaitDstStageMask = vk::PipelineStageFlagBits::eAllCommands;

        vk::SubmitInfo submitInfo(1,                   // waitSemaphoreCount
                                  &currentFinishedSemaphore,      // pWaitSemaphores
                                  &pWaitDstStageMask,  // pWaitDstStageMask
                                  0,                   // commandBufferCount
                                  nullptr, // commandBuffer             // pCommandBuffers
                                  0,                   // signalSemaphoreCount
                                  nullptr); // pSignalSemaphores

        vkCore::global::graphicsQueue.submit(submitInfo, nullptr);
    }

    prevFrame = currentFrame;
    currentFrame = (currentFrame + 1) % mSync.getMaxFramesInFlight();
}


std::vector<uint8_t> Context::downloadLatestFrame() {
    mSync.waitForFrame(prevFrame);
    vk::Image image = getImage(prevFrame);
    vk::Format format = getFormat();
    vk::Extent3D extent {getExtent(), 1};
    vk::ImageLayout layout = vk::ImageLayout::ePresentSrcKHR;

    return vkCore::download<uint8_t>(image, format, layout, extent);
}


void Context::updateSettings() {
    if (pConfig->mMaxGeometryChanged || pConfig->mMaxTexturesChanged) {
        mSync.waitForFrame(prevFrame);

        pConfig->mMaxGeometryChanged = false;
        pConfig->mMaxTexturesChanged = false;

        mCurrentScene->mVertexBuffers.resize(pConfig->mMaxGeometry);
        mCurrentScene->mIndexBuffers.resize(pConfig->mMaxGeometry);
        mCurrentScene->mMaterialIndexBuffers.resize(pConfig->mMaxGeometry);
        mCurrentScene->mTextures.resize(pConfig->mMaxTextures);

        mCurrentScene->initGeometryDescriptorSets();

        pConfig->triggerPipelineRefresh();
    }

    // Handle pipeline refresh
    if (pConfig->mPipelineNeedsRefresh) {
        pConfig->mPipelineNeedsRefresh = false;

        // Calling wait idle, because pipeline recreation is assumed to be a very rare event to happen.
        vkCore::global::device.waitIdle();

        initPipelines();
        mRayTracer.createShaderBindingTable();
    }

    // Handle swapchain refresh
    if (pConfig->mSwapchainNeedsRefresh) {
        pConfig->mSwapchainNeedsRefresh = false;

        recreateSwapchain();
    }
}

void Context::render() {
    update();

    if (pConfig->mPresent) {
        if (pWindow->minimized()) {
            KF_WARN("Window minimized! New frames will not be rendered.");
            return;
        }

        if (pWindow->changed()) {
            KF_INFO("Window size changed!");
            getCamera()->mProjNeedsUpdate = true;
            return;
        }
    }

    prepareFrame();
    recordSwapchainCommandBuffers();
//    submitFrame();
}

void Context::recreateSwapchain() {
    // Waiting idle because this event is considered to be very rare.
    vkCore::global::device.waitIdle();

    if (pConfig->mPresent) {
        KF_DEBUG("Recreating Swapchain...");

        // Clean up existing swapchain and dependencies.
        mSwapchain.destroy();

        // Recreating the swapchain.
        mSwapchain.init(&mSurface, mPostProcessingRenderer.getRenderPass().get());

        // Update the camera screen size to avoid image stretching.
        auto width = static_cast<int>(getExtent().width);
        auto height = static_cast<int>(getExtent().height);
        if (width != getCamera()->getWidth() || height != getCamera()->getHeight())
          getCamera()->setSize(width, height);
    } else {
        KF_DEBUG("Switching Framebuffer...");

        getCamera()->mFrames->init(
                pConfig->mMaxImagesInFlight, getExtent(),
                getFormat(), getColorSpace(),
                mPostProcessingRenderer.getRenderPass().get());
    }

    // Recreate storage image with the new swapchain image size and update the path tracing descriptor set to use the new storage image view.
    mRayTracer.setRenderTargets(getCamera()->getRenderTargets());
    mRayTracer.createStorageImage(getExtent());

    if (pConfig->mUseDenoiser)
        mDenoiser.allocateBuffers(getExtent());

    mPostProcessingRenderer.updateDescriptors(mRayTracer.getStorageImageInfo("rgba"));
    mRayTracer.updateDescriptors();

    if (pGui != nullptr)
      pGui->recreate(getExtent());

    pConfig->mSwapchainNeedsRefresh = false;
}

void Context::initPipelines() {
    KF_DEBUG("Recreating Pipeline...");
    // path tracing pipeline
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {mRayTracer.getDescriptorSetLayout(),
                                                                 mCurrentScene->mSceneDescriptors.layout.get(),
                                                                 mCurrentScene->mGeometryDescriptors.layout.get()};

    mRayTracer.createPipeline(descriptorSetLayouts);
    pConfig->mPipelineNeedsRefresh = false;
}

void Context::initGui() {
    if (pGui != nullptr)
      pGui->init(pWindow->get(), &mSurface, getExtent(),
                   mPostProcessingRenderer.getRenderPass().get());
}

void Context::recordSwapchainCommandBuffers() {
    mSync.waitForFrame(prevFrame);

    RtPushConstants pushConstants = {
            mCurrentScene->getClearColor(),
            global::frameCount,
            pConfig->mPerPixelSampleRate,
            pConfig->mPathDepth,
            static_cast<uint32_t>(mCurrentScene->mUseEnvironmentMap),
            static_cast<uint32_t>(pConfig->mRussianRoulette),
            pConfig->mRussianRouletteMinBounces,
            pConfig->mNextEventEstimation,
            pConfig->mNextEventEstimationMinBounces };   // TODO: remove unused

    size_t imageIndex = getCurrentImageIndex();

    vk::CommandBuffer cmdBuf = mCommandBuffers.get(imageIndex);
    vk::CommandBuffer cmdBuf2 = mCommandBuffers2.get(imageIndex);

    size_t index = imageIndex % mSync.getMaxFramesInFlight();

    mCommandBuffers.begin(imageIndex);
    {
        cmdBuf.pushConstants(
                mRayTracer.getPipelineLayout(),
                vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
                vk::ShaderStageFlagBits::eClosestHitKHR,
                0,
                sizeof(RtPushConstants),
                &pushConstants);

        cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, mRayTracer.getPipeline());

        std::vector<vk::DescriptorSet> descriptorSets = {mRayTracer.getDescriptorSet(index),
                                                         mCurrentScene->mSceneDescriptorSets[index],
                                                         mCurrentScene->mGeometryDescriptorSets[index]};

        cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                  mRayTracer.getPipelineLayout(),
                                  0,
                                  static_cast<uint32_t>(descriptorSets.size()),
                                  descriptorSets.data(),
                                  0,
                                  nullptr);

        // rt
        mRayTracer.trace(cmdBuf, getImage(imageIndex), getExtent());

        // denoise
        if (pConfig->mUseDenoiser)
            mDenoiser.imageToBuffer(cmdBuf, {
                mRayTracer.getStorageImage("rgba"),
                mRayTracer.getStorageImage("albedo"),
                mRayTracer.getStorageImage("normal")
            });

    }
    mCommandBuffers.end(imageIndex);
    submitWithTLSemaphore(cmdBuf);


    if(pConfig->mUseDenoiser)
        mDenoiser.denoiseImageBuffer(mFenceValue);


    mCommandBuffers2.begin(imageIndex);
    {
        if(pConfig->mUseDenoiser)
            mDenoiser.bufferToImage(cmdBuf2, mRayTracer.getStorageImage("rgba"));

        // pp
        mPostProcessingRenderer.beginRenderPass(cmdBuf2, getFramebuffer(imageIndex), getExtent());
        {
            mPostProcessingRenderer.render(cmdBuf2, getExtent(), index);

            if (pGui != nullptr)
                pGui->renderDrawData(cmdBuf2);
        }
        mPostProcessingRenderer.endRenderPass(cmdBuf2);
    }
    mCommandBuffers2.end(imageIndex);
    submitFrame(cmdBuf2);
}
}
