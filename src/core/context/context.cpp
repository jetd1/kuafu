//
// Created by jet on 4/9/21.
//

#include "core/context/context.hpp"
#include "core/context/global.hpp"

#define STB_IMAGE_IMPLEMENTATION

#ifndef VK_KHR_acceleration_structure
#error "The local Vulkan SDK does not support VK_KHR_acceleration_structure."
#endif

#define VULKAN_HPP_STORAGE_SHARED
#define VULKAN_HPP_STORAGE_SHARED_EXPORT
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace kuafu {
    const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};

    /// @todo Currently always build with debug utils because an error might cause instant
    std::vector<const char *> extensions = {"VK_EXT_debug_utils"};

    //#ifdef KF_DEBUG
    //  std::vector<const char*> extensions = { "VK_EXT_debug_utils" };
    //#else
    //  std::vector<const char*> extensions;
    //#endif

    std::vector<const char *> deviceExtensions = {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                  VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                  VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                  VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                  VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                                  VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                  VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                  VK_KHR_SHADER_CLOCK_EXTENSION_NAME};

    size_t currentFrame = 0;
    size_t prevFrame = 0;

    Context::~Context() {
        vkCore::global::device.waitIdle();

        // Gui needs to be destroyed manually, as RAII destruction will not be possible.
        if (mGui != nullptr) {
            mGui->destroy();
        }
    }

    void Context::setGui(const std::shared_ptr<Gui> &gui, bool initialize) {
        if (mGui != nullptr) {
            recreateSwapchain();
            mGui->destroy();
        }

        mGui = gui;

        if (initialize) {
            initGui();
        }
    }

    void Context::init() {
//        KF_LOG_TIME_START("Context start up ...");

        // Retrieve and add window extensions to other extensions.
        auto windowExtensions = mWindow->getExtensions();
        extensions.insert(extensions.end(), windowExtensions.begin(), windowExtensions.end());

        // Instance
        mInstance = vkCore::initInstanceUnique(layers, extensions, VK_API_VERSION_1_2);

        // Debug messenger
        mDebugMessenger.init();

        // Surface
        VkSurfaceKHR surface;
        SDL_bool result = SDL_Vulkan_CreateSurface(mWindow->get(), static_cast<VkInstance>(vkCore::global::instance),
                                                   &surface);

        if (result != SDL_TRUE)
            throw std::runtime_error("Failed to create surface");

        mSurface.init(vk::SurfaceKHR(surface), mWindow->getSize());

        // Physical device
        vkCore::global::physicalDevice = vkCore::initPhysicalDevice(); // @todo This function does not check if any feature is available when evaluating a device. Additionally, it is pointless to assign vkCore::global::physicalDevice in here because it doesn't need a unique handle.

        // Reassess the support of the preferred surface settings.
        mSurface.assessSettings();

        // Queues
        vkCore::initQueueFamilyIndices();

        // Logical device
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeatures;
        asFeatures.accelerationStructure = VK_TRUE;

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

        vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
        rayQueryFeatures.rayQuery = VK_TRUE;
        rayQueryFeatures.pNext = &robustness2FeaturesEXT;

        vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        bufferDeviceAddressFeatures.pNext = &rayQueryFeatures;

        vk::PhysicalDeviceFeatures deviceFeatures;
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.shaderInt64 = VK_TRUE;

        vk::PhysicalDeviceFeatures2 deviceFeatures2{deviceFeatures};
        deviceFeatures2.pNext = &bufferDeviceAddressFeatures;

        mDevice = vkCore::initDeviceUnique(deviceExtensions, {}, deviceFeatures2);

        vkCore::global::device = mDevice.get();

        // Retrieve all queue handles.
        vkCore::global::device.getQueue(vkCore::global::graphicsFamilyIndex, 0, &vkCore::global::graphicsQueue);
        vkCore::global::device.getQueue(vkCore::global::transferFamilyIndex, 0, &vkCore::global::transferQueue);

        // Command pools
        mGraphicsCmdPool = vkCore::initCommandPoolUnique(vkCore::global::graphicsFamilyIndex,
                                                         vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        vkCore::global::graphicsCmdPool = mGraphicsCmdPool.get();

        mTransferCmdPool = vkCore::initCommandPoolUnique(vkCore::global::transferFamilyIndex, {});
        vkCore::global::transferCmdPool = mTransferCmdPool.get();

        // Post processing renderer
        //mPostProcessingRenderer.initDepthImage(mSurface.getExtent());
        mPostProcessingRenderer.initRenderPass(mSurface.getFormat());

        // Swapchain
        mSwapchain.init(&mSurface, mPostProcessingRenderer.getRenderPass().get());
        mConfig.mSwapchainNeedsRefresh = false;

        // GUI
        initGui();

        // Create fences and semaphores.
        mSync.init();

        // Path tracer
        mRayTracer.init();
        mConfig.mMaxPathDepth = mRayTracer.getCapabilities().pipelineProperties.maxRayRecursionDepth;
        mRayTracer.initVarianceBuffer(static_cast<float>(mWindow->getWidth()),
                                      static_cast<float>(mWindow->getHeight()));

        mScene.prepareBuffers();

        // Descriptor sets and layouts
        mRayTracer.initDescriptorSet();
        mScene.initSceneDescriptorSets();
        mScene.initGeoemtryDescriptorSets();

        // Default environment map to assure start up.
        mScene.setEnvironmentMap("");
        mScene.uploadEnvironmentMap();
        mScene.removeEnvironmentMap();

        // Update scene descriptor sets.
        mScene.updateSceneDescriptors();

        // Initialize the path tracing pipeline.
        initPipelines();

        mRayTracer.createStorageImage(mSwapchain.getExtent());
        mRayTracer.createShaderBindingTable();

        // Post processing renderer
        mPostProcessingRenderer.initDescriptorSet();
        mPostProcessingRenderer.initPipeline();
        mPostProcessingRenderer.updateDescriptors(mRayTracer.getStorageImageInfo());

        // Initialize and record swapchain command buffers.
        mSwapchainCommandBuffers.init(mGraphicsCmdPool.get(), vkCore::global::swapchainImageCount,
                                      vk::CommandBufferUsageFlagBits::eRenderPassContinue);

//        KF_LOG_TIME_STOP("Context finished");
    }

    void Context::update() {
        updateSettings();

#ifdef KF_VARIANCE_ESTIMATOR
        // update variance
    if (mConfig._updateVariance)
    {
      static uint32_t counter = 0;
      const int maxSize       = 100;
      static std::vector<std::array<float, maxSize>> ppVariances; // vector of vector. outer for each pixel and inner for each accumulated sample

      auto extent     = mSwapchain.getExtent();
      auto pixelCount = 1;
      //extent.width* extent.height;
      ppVariances.resize(pixelCount); // as many ppVariances as pixels

      KF_ASSERT(maxSize > mConfig.mPerPixelSampleRate, "Variance Estimates Out Of Bound");

      // 1. Gathering stage
      bool finishedGatheringPp = false;
      if (counter < mConfig.mPerPixelSampleRate)
      {
        // For each new sample of a frame for each pixel set estimated variance
        for (uint32_t i = 0; i < pixelCount; ++i)
        {
          ppVariances[i][counter] = mRayTracer.getPixelVariance(i);
          std::cout << ppVariances[i][counter] << std::endl;
        }

        ++counter;
      }
      else
      {
        finishedGatheringPp = true;
      }

      // 2. Averaging State
      if (finishedGatheringPp)
      {
        counter = 0;

        std::vector<float> ppSum(pixelCount, 0.0F);

        // Sum up samples for each pixel and store results separately
        for (uint32_t i = 0; i < mConfig.mPerPixelSampleRate; ++i)
        {
          for (uint32_t j = 0; j < pixelCount; ++j)
          {
            ppSum[j] += ppVariances[j][i];
          }
        }

        float avg = 0.0F;
        // calculate final average
        for (size_t i = 0; i < ppSum.size(); ++i)
        {
          // Do not forget to take the average of the previous sample sum per pixel
          avg += ppSum[i] / static_cast<float>(mConfig.mPerPixelSampleRate);
        }

        mConfig._variance = avg / pixelCount;
      }
    }

    //std::cout << mRayTracer.getPixelVariance() << std::endl;
#endif

        uint32_t imageIndex = mSwapchain.getCurrentImageIndex();
        uint32_t maxFramesInFlight = static_cast<uint32_t>(mSync.getMaxFramesInFlight());

        mScene.uploadCameraBuffer(imageIndex % maxFramesInFlight);

        // If the scene is empty add a dummy triangle so that the acceleration structures can be built successfully.

        // Move dummy behind camera
        if (mScene.mDummy) {
            mScene.translateDummy();
        }

        if (mScene.mGeometryInstances.empty()) {
            mScene.addDummy();
        } else if (mScene.mDummy) {
            mScene.removeDummy();
        }

        if (mScene._uploadEnvironmentMap) {
            mSync.waitForFrame(prevFrame);
            mScene.uploadEnvironmentMap();
            mScene.updateSceneDescriptors();
        }

        if (mScene._uploadGeometries) {
            mScene.uploadGeometries();
            mScene.updateGeoemtryDescriptors();
        }

        if (mScene._uploadGeometryInstancesToBuffer) {
            mScene.uploadGeometryInstances();

            // @TODO Try to call this as few times as possible.
            mRayTracer.createBottomLevelAS(mScene.mVertexBuffers, mScene.mIndexBuffers, mScene.mGeometries);
            mRayTracer.buildTlas(mScene.mGeometryInstances,
                                  vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                                  vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
            mRayTracer.updateDescriptors();
        } else {
            mRayTracer.updateTlas(mScene.mGeometryInstances,
                                   vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                                   vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
        }

        // Increment frame counter for jitter cam.
        if (mConfig._accumulateFrames)
            ++global::frameCount;
        else
            global::frameCount = -1;
    }

    void Context::prepareFrame() {
        mSwapchain.acquireNextImage(mSync.getImageAvailableSemaphore(currentFrame), nullptr);
    }

    void Context::submitFrame() {
        uint32_t imageIndex = mSwapchain.getCurrentImageIndex();
        size_t imageIndex_t = static_cast<size_t>(imageIndex);

        // Check if a previous frame is using the current image.
        if (mSync.getImageInFlight(imageIndex)) {
            mSync.waitForFrame(currentFrame);
        }

        // This will mark the current image to be in use by this frame.
        mSync.getImageInFlight(imageIndex_t) = mSync.getInFlightFence(currentFrame);

        vk::CommandBuffer cmdBuf = mSwapchainCommandBuffers.get(imageIndex);

        // Reset the signaled state of the current frame's fence to the unsignaled one.
        auto currentInFlightFence_t = mSync.getInFlightFence(currentFrame);
        vkCore::global::device.resetFences(1, &currentInFlightFence_t);

        // Submits / executes the current image's / framebuffer's command buffer.
        vk::PipelineStageFlags pWaitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        auto waitSemaphore = mSync.getImageAvailableSemaphore(currentFrame);
        auto signaleSemaphore = mSync.getFinishedRenderSemaphore(currentFrame);

        vk::SubmitInfo submitInfo(1,                   // waitSemaphoreCount
                                  &waitSemaphore,      // pWaitSemaphores
                                  &pWaitDstStageMask,  // pWaitDstStageMask
                                  1,                   // commandBufferCount
                                  &cmdBuf,             // pCommandBuffers
                                  1,                   // signalSemaphoreCount
                                  &signaleSemaphore); // pSignalSemaphores

        vkCore::global::graphicsQueue.submit(submitInfo, currentInFlightFence_t);

        // Tell the presentation engine that the current image is ready.
        vk::PresentInfoKHR presentInfo(1,                          // waitSemaphoreCount
                                       &signaleSemaphore,          // pWaitSemaphores
                                       1,                          // swapchainCount
                                       &vkCore::global::swapchain, // pSwapchains
                                       &imageIndex,                // pImageIndices
                                       nullptr);                  // pResults

        // This try catch block is only necessary on Linux for whatever reason. Without it, resizing the window will result in an unhandled throw of vk::Result::eErrorOutOfDateKHR.
        try {
            vk::Result result = vkCore::global::graphicsQueue.presentKHR(presentInfo);
            if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
                mConfig.triggerSwapchainRefresh();
//                KF_WARN("Swapchain out of data or suboptimal.");
            }
        }
        catch (...) {
            mConfig.triggerSwapchainRefresh();
        }

        prevFrame = currentFrame;
        currentFrame = (currentFrame + 1) % mSync.getMaxFramesInFlight();
    }

    void Context::updateSettings() {
        if (mConfig._maxGeometryChanged || mConfig._maxTexturesChanged) {
            mSync.waitForFrame(prevFrame);

            mConfig._maxGeometryChanged = false;
            mConfig._maxTexturesChanged = false;

            mScene.mVertexBuffers.resize(mConfig._maxGeometry);
            mScene.mIndexBuffers.resize(mConfig._maxGeometry);
            mScene._materialIndexBuffers.resize(mConfig._maxGeometry);
            mScene._textures.resize(mConfig._maxTextures);

            mScene.initGeoemtryDescriptorSets();

            mConfig.mPipelineNeedsRefresh = true;
        }

        // Handle pipeline refresh
        if (mConfig.mPipelineNeedsRefresh) {
            mConfig.mPipelineNeedsRefresh = false;

            // Calling wait idle, because pipeline recreation is assumed to be a very rare event to happen.
            vkCore::global::device.waitIdle();

            initPipelines();
            mRayTracer.createShaderBindingTable();
        }

        // Handle swapchain refresh
        if (mConfig.mSwapchainNeedsRefresh) {
            mConfig.mSwapchainNeedsRefresh = false;

            recreateSwapchain();
        }
    }

    void Context::render() {
        update();

        if (mWindow->minimized())
            return;

        if (mWindow->changed()) {
            mCamera->mProjNeedsUpdate = true;
            return;
        }

        prepareFrame();
        recordSwapchainCommandBuffers();
        submitFrame();
    }

    void Context::recreateSwapchain() {
//        KF_LOG_TIME_START("Re-creating swapchain ...");

        // Waiting idle because this event is considered to be very rare.
        vkCore::global::device.waitIdle();

        // Clean up existing swapchain and dependencies.
        mSwapchain.destroy();

        // Recreating the swapchain.
        mSwapchain.init(&mSurface, mPostProcessingRenderer.getRenderPass().get());

        // Recreate storage image with the new swapchain image size and update the path tracing descriptor set to use the new storage image view.
        mRayTracer.createStorageImage(mSwapchain.getExtent());

        const auto &storageImageInfo = mRayTracer.getStorageImageInfo();
        mPostProcessingRenderer.updateDescriptors(storageImageInfo);

        mRayTracer.updateDescriptors();

        if (mGui != nullptr) {
            mGui->recreate(mSwapchain.getExtent());
        }

        // Update the camera screen size to avoid image stretching.
        auto screenSize = mSwapchain.getExtent();
        mCamera->setSize(screenSize.width, screenSize.height);

        mConfig.mSwapchainNeedsRefresh = false;

//        KF_LOG_TIME_STOP("Finished re-creating swapchain");
    }

    void Context::initPipelines() {
//        KF_LOG_TIME_START("Initializing graphic pipelines ...");

        // path tracing pipeline
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {mRayTracer.getDescriptorSetLayout(),
                                                                     mScene._sceneDescriptors.layout.get(),
                                                                     mScene._geometryDescriptors.layout.get()};

        mRayTracer.createPipeline(descriptorSetLayouts);
        mConfig.mPipelineNeedsRefresh = false;

//        KF_LOG_TIME_STOP("Finished graphic pipelines initialization");
    }

    void Context::initGui() {
        if (mGui != nullptr) {
            mGui->init(mWindow->get(), &mSurface, mSwapchain.getExtent(),
                       mPostProcessingRenderer.getRenderPass().get());
        }
    }

    void Context::recordSwapchainCommandBuffers() {
        mSync.waitForFrame(prevFrame);

        RtPushConstants pushConstants = {mConfig._clearColor,
                                         global::frameCount,
                                         mConfig.mPerPixelSampleRate,
                                         mConfig.mPathDepth,
                                         static_cast<uint32_t>(mScene._useEnvironmentMap),
                                         static_cast<uint32_t>(mConfig._russianRoulette),
                                         mConfig._russianRouletteMinBounces,
                                         mConfig._nextEventEstimation,
                                         mConfig._nextEventEstimationMinBounces};

        for (size_t imageIndex = 0; imageIndex < mSwapchainCommandBuffers.get().size(); ++imageIndex) {
            vk::CommandBuffer cmdBuf = mSwapchainCommandBuffers.get(imageIndex);

            mSwapchainCommandBuffers.begin(imageIndex);
            {
                cmdBuf.pushConstants(
                        mRayTracer.getPipelineLayout(),
                        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
                        vk::ShaderStageFlagBits::eClosestHitKHR,
                        0,
                        sizeof(RtPushConstants),
                        &pushConstants);

                cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, mRayTracer.getPipeline());

                size_t index = imageIndex % mSync.getMaxFramesInFlight();

                std::vector<vk::DescriptorSet> descriptorSets = {mRayTracer.getDescriptorSet(index),
                                                                 mScene.mSceneDescriptorSets[index],
                                                                 mScene.mGeometryDescriptorSets[index]};

                cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                          mRayTracer.getPipelineLayout(),
                                          0,
                                          static_cast<uint32_t>(descriptorSets.size()),
                                          descriptorSets.data(),
                                          0,
                                          nullptr);

                // rt
                mRayTracer.trace(cmdBuf, mSwapchain.getImage(imageIndex), mSwapchain.getExtent());

                // pp
                mPostProcessingRenderer.beginRenderPass(cmdBuf, mSwapchain.getFramebuffer(imageIndex),
                                                        mSwapchain.getExtent());
                {
                    mPostProcessingRenderer.render(cmdBuf, mSwapchain.getExtent(), index);

                    // imGui
                    if (mGui != nullptr) {
                        mGui->renderDrawData(cmdBuf);
                    }
                }
                mPostProcessingRenderer.endRenderPass(cmdBuf);
            }
            mSwapchainCommandBuffers.end(imageIndex);
        }
    }
}
