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
                                              VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                              VK_KHR_SHADER_CLOCK_EXTENSION_NAME};

size_t currentFrame = 0;
size_t prevFrame = 0;

Context::~Context() {
    vkCore::global::device.waitIdle();

    // Gui needs to be destroyed manually, as RAII destruction will not be possible.
    if (pGui != nullptr) {
      pGui->destroy();
    }
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
//        KF_LOG_TIME_START("Context start up ...");

    // Retrieve and add window extensions to other extensions.
    auto windowExtensions = pWindow->getExtensions();
    extensions.insert(extensions.end(), windowExtensions.begin(), windowExtensions.end());

    // Instance
    mInstance = vkCore::initInstanceUnique(layers, extensions, VK_API_VERSION_1_2);

#ifdef VK_VALIDATION
    // Debug messenger
    mDebugMessenger.init();
#endif

    // Surface
    VkSurfaceKHR surface;
    SDL_bool result = SDL_Vulkan_CreateSurface(
        pWindow->get(), static_cast<VkInstance>(vkCore::global::instance),
                                               &surface);

    if (result != SDL_TRUE)
        throw std::runtime_error("Failed to create surface");

    mSurface.init(vk::SurfaceKHR(surface), pWindow->getSize());

    // Physical device
    vkCore::global::physicalDevice = vkCore::initPhysicalDevice(); // @todo This function does not check if any feature is available when evaluating a device. Additionally, it is pointless to assign vkCore::global::physicalDevice in here because it doesn't need a unique handle.

    // Reassess the support of the preferred surface settings.
    mSurface.assessSettings();

    // Queues
    vkCore::initQueueFamilyIndices();

    // Logical device
    vk::PhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures;
    shaderClockFeatures.shaderSubgroupClock = VK_TRUE;

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
    pConfig->mSwapchainNeedsRefresh = false;

    // GUI
    initGui();

    // Create fences and semaphores.
    mSync.init();

    // Path tracer
    mRayTracer.init();
    pConfig->mMaxPathDepth = mRayTracer.getCapabilities().pipelineProperties.maxRayRecursionDepth;
    mRayTracer.initVarianceBuffer(static_cast<float>(pWindow->getWidth()),
                                  static_cast<float>(pWindow->getHeight()));

    mScene.prepareBuffers();

    // Descriptor sets and layouts
    mRayTracer.initDescriptorSet();
    mScene.initSceneDescriptorSets();
    mScene.initGeometryDescriptorSets();

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
if (pConfig->mUpdateVariance)
{
  static uint32_t counter = 0;
  const int maxSize       = 100;
  static std::vector<std::array<float, maxSize>> ppVariances; // vector of vector. outer for each pixel and inner for each accumulated sample

  auto extent     = mSwapchain.getExtent();
  auto pixelCount = 1;
  //extent.width* extent.height;
  ppVariances.resize(pixelCount); // as many ppVariances as pixels

  KF_ASSERT(maxSize > pConfig->mPerPixelSampleRate, "Variance Estimates Out Of Bound");

  // 1. Gathering stage
  bool finishedGatheringPp = false;
  if (counter < pConfig->mPerPixelSampleRate)
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
    for (uint32_t i = 0; i < pConfig->mPerPixelSampleRate; ++i)
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
      avg += ppSum[i] / static_cast<float>(pConfig->mPerPixelSampleRate);
    }

    pConfig->mVariance = avg / pixelCount;
  }
}

//std::cout << mRayTracer.getPixelVariance() << std::endl;
#endif

    uint32_t imageIndex = mSwapchain.getCurrentImageIndex();
    uint32_t maxFramesInFlight = static_cast<uint32_t>(mSync.getMaxFramesInFlight());

    mScene.uploadUniformBuffers(imageIndex % maxFramesInFlight);

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

    if (mScene.mUploadEnvironmentMap) {
        mSync.waitForFrame(prevFrame);
        mScene.uploadEnvironmentMap();
        mScene.updateSceneDescriptors();
    }

    if (mScene.mUploadGeometries) {
        mScene.uploadGeometries();
        mScene.updateGeoemtryDescriptors();
    }

    if (mScene.mUploadGeometryInstancesToBuffer) {
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
    if (pConfig->mAccumulateFrames)
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

    // Download every frame
    // TODO: get this into command buffer
    mLatestFrame = vkCore::download<uint8_t>(
            mSwapchain.getImage(imageIndex),
            mSurface.getFormat(), vk::ImageLayout::ePresentSrcKHR,
            vk::Extent3D{mSurface.getExtent(), 1});

//        mSwapchain.setImageLayout(imageIndex,
//                                  vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::ePresentSrcKHR);

    vk::CommandBuffer cmdBuf = mSwapchainCommandBuffers.get(imageIndex);

    // Reset the signaled state of the current frame's fence to the unsignaled one.
    auto currentInFlightFence_t = mSync.getInFlightFence(currentFrame);
    auto result = vkCore::global::device.resetFences(1, &currentInFlightFence_t);
    KF_ASSERT(result == vk::Result::eSuccess, "KF: failed to reset fences");

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
            pConfig->triggerSwapchainRefresh();
            KF_WARN("Swapchain out of data or suboptimal.");
        }
    }
    catch (...) {
        pConfig->triggerSwapchainRefresh();
    }

    prevFrame = currentFrame;
    currentFrame = (currentFrame + 1) % mSync.getMaxFramesInFlight();
}

void Context::updateSettings() {
    if (pConfig->mMaxGeometryChanged || pConfig->mMaxTexturesChanged) {
        mSync.waitForFrame(prevFrame);

        pConfig->mMaxGeometryChanged = false;
        pConfig->mMaxTexturesChanged = false;

        mScene.mVertexBuffers.resize(pConfig->mMaxGeometry);
        mScene.mIndexBuffers.resize(pConfig->mMaxGeometry);
        mScene.mMaterialIndexBuffers.resize(pConfig->mMaxGeometry);
        mScene.mTextures.resize(pConfig->mMaxTextures);

        mScene.initGeometryDescriptorSets();

        pConfig->mPipelineNeedsRefresh = true;
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

    if (pWindow->minimized()) {
        KF_WARN("pWindow->minimized()");
        return;
    }

    if (pWindow->changed()) {
        KF_WARN("pWindow->changed()");
        mScene.mCurrentCamera->mProjNeedsUpdate = true;
//            pCamera->mProjNeedsUpdate = true;
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

    if (pGui != nullptr) {
      pGui->recreate(mSwapchain.getExtent());
    }

    // Update the camera screen size to avoid image stretching.
    auto screenSize = mSwapchain.getExtent();
    mScene.mCurrentCamera->setSize(screenSize.width, screenSize.height);
//        pCamera->setSize(screenSize.width, screenSize.height);

    pConfig->mSwapchainNeedsRefresh = false;

//        KF_LOG_TIME_STOP("Finished re-creating swapchain");
}

void Context::initPipelines() {
//        KF_LOG_TIME_START("Initializing graphic pipelines ...");

    // path tracing pipeline
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {mRayTracer.getDescriptorSetLayout(),
                                                                 mScene.mSceneDescriptors.layout.get(),
                                                                 mScene.mGeometryDescriptors.layout.get()};

    mRayTracer.createPipeline(descriptorSetLayouts);
    pConfig->mPipelineNeedsRefresh = false;

//        KF_LOG_TIME_STOP("Finished graphic pipelines initialization");
}

void Context::initGui() {
    if (pGui != nullptr) {
      pGui->init(pWindow->get(), &mSurface, mSwapchain.getExtent(),
                   mPostProcessingRenderer.getRenderPass().get());
    }
}

void Context::recordSwapchainCommandBuffers() {
    mSync.waitForFrame(prevFrame);

    RtPushConstants pushConstants = {pConfig->mClearColor,
                                     global::frameCount,
                                     pConfig->mPerPixelSampleRate,
                                     pConfig->mPathDepth,
                                     static_cast<uint32_t>(mScene.mUseEnvironmentMap),
                                     static_cast<uint32_t>(pConfig->mRussianRoulette),
                                     pConfig->mRussianRouletteMinBounces,
                                     pConfig->mNextEventEstimation,
                                     pConfig->mNextEventEstimationMinBounces};

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
                if (pGui != nullptr) {
                  pGui->renderDrawData(cmdBuf);
                }
            }
            mPostProcessingRenderer.endRenderPass(cmdBuf);
        }
        mSwapchainCommandBuffers.end(imageIndex);
    }
}
}
