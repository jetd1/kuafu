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
#pragma once

#include "core/window.hpp"
#include "core/camera.hpp"
#include "core/gui.hpp"
#include "core/scene.hpp"
#include "core/postprocess.hpp"
#include "core/rt/rt.hpp"
#include "core/image.hpp"

#ifdef KUAFU_OPTIX_DENOISER
#include "core/denoiser.hpp"
#endif

namespace kuafu {

    class Context {
        std::shared_ptr<Window> pWindow = nullptr;
        vk::UniqueInstance mInstance;

#ifdef VK_VALIDATION
        vkCore::DebugMessenger mDebugMessenger;
#endif

        vkCore::Surface mSurface;     // TODO: kuafu_urgent: update when window changes.
        vk::UniqueDevice mDevice;
        vk::UniqueCommandPool mGraphicsCmdPool;
        vk::UniqueCommandPool mTransferCmdPool;

        RayTracer mRayTracer;
        PostProcessingRenderer mPostProcessingRenderer;

        // Timeline semaphores
        uint64_t mFenceValue = 0;

#ifdef KUAFU_OPTIX_DENOISER
        DenoiserOptix mDenoiser;
#endif

        vkCore::CommandBuffer mCommandBuffers2;
        vk::UniqueSemaphore mCmdSemaphore;

        // on screen use only
        size_t mCurrentFrame = 0;
        size_t mPrevFrame = 0;
        vkCore::Swapchain mSwapchain;
        vkCore::Sync mSwapchainSync;

        vkCore::CommandBuffer mCommandBuffers;

        std::shared_ptr<Gui> pGui = nullptr;

        std::vector<std::unique_ptr<Scene>> mScenes;
        Scene* mCurrentScene;
        std::shared_ptr<Config> pConfig;

        /// Used to set the GUI that will be used.
        ///
        /// The GUI can be changed at runtime. This enables the user to swap between different pre-built GUIs on the fly.
        /// @param gui A pointer to a GUI object that will be rendered on top of the final image.
        /// @param initialize If true, the GUI object will be initialized (false if not specified).
        void setGui(const std::shared_ptr<Gui> &gui, bool initialize = false);

        void init();

        /// Used to update and upload uniform buffers.
        void update();

        /// Retrieves an image from the swapchain and presents it.
        void render();

        void initGui();

        void initPipelines();

        /// Records commands to the swapchain command buffers that will be used for rendering.
        /// @todo Rasterization has been removed for now. Might want to re-add rasterization support with RT-compatible shaders again.
        void recordSwapchainCommandBuffers();

        /// Handles swapchain and pipeline recreations triggered by the user using setters provided in kuafu::Settings.
        void updateSettings();

        /// Recreates the swapchain and re-records the swapchain command buffers.
        void recreateSwapchain();

        /// Acquires the next image from the swapchain.
        void prepareFrame();

        void submitWithTLSemaphore(const vk::CommandBuffer& cmdBuf);
        void submitFrame(const vk::CommandBuffer& cmdBuf);

        std::vector<uint8_t> downloadLatestFrameFromSwapchain();

        [[nodiscard]] inline auto  getCamera() const { return mCurrentScene->mCurrentCamera; }

        inline auto& getSync() { return pConfig->mPresent ? mSwapchainSync : getCamera()->mSync; }
        inline auto  getCurrentImageIndex() { return pConfig->mPresent ? mSwapchain.getCurrentImageIndex() : getCamera()->mFrames->getCurrentImageIndex(); }
        inline auto  getImage(size_t idx) { return pConfig->mPresent ? mSwapchain.getImage(idx) : getCamera()->mFrames->getImage(idx); }
        inline auto& getFramebuffer(size_t idx) { return pConfig->mPresent ?
                 mSwapchain.getFramebuffer(idx) : getCamera()->mFrames->getFramebuffer(idx).get(); }
        inline auto  getFormat() { return pConfig->mPresent ? mSurface.getFormat() : pConfig->mFormat; }
        inline auto  getColorSpace() { return pConfig->mPresent ? mSurface.getColorSpace() : pConfig->mColorSpace; }
        inline auto  getExtent() { return pConfig->mPresent ?
                     mSwapchain.getExtent() : getCamera() ?
                     vk::Extent2D{ static_cast<uint32_t>(getCamera()->getWidth()),
                                   static_cast<uint32_t>(getCamera()->getHeight())} : vk::Extent2D{1, 1}; }

#ifdef KUAFU_OPTIX_DENOISER
        inline auto getCmdSemaphore() { return pConfig->mUseDenoiser ? mDenoiser.getTLSemaphore() : mCmdSemaphore.get(); };
#else
        inline auto getCmdSemaphore() { return mCmdSemaphore.get(); };
#endif

        inline auto getCurrentFrameIndex() { return pConfig->mPresent ? mCurrentFrame : getCamera()->mFrames->mCurrentFrame; }
        inline auto getPrevFrameIndex() { return pConfig->mPresent ? mPrevFrame : getCamera()->mFrames->mPrevFrame; }
        inline void incFrameIdx() {
//            KF_DEBUG("IDX: PREV={}, CUR={}", getPrevFrameIndex(), getCurrentFrameIndex());
            if (pConfig->mPresent) {
                mPrevFrame = mCurrentFrame;
                mCurrentFrame = (mCurrentFrame + 1) % getSync().getMaxFramesInFlight();
//                KF_DEBUG("IDX UPDATED: PREV={}, CUR={}", mPrevFrame, mCurrentFrame);
            } else {
                if (getCamera()->mFirst)
                    getCamera()->mFirst = false;
                auto currentFrame = getCamera()->mFrames->mCurrentFrame;
                getCamera()->mFrames->mPrevFrame = currentFrame;
                getCamera()->mFrames->mCurrentFrame = (currentFrame + 1) % getSync().getMaxFramesInFlight();
//                KF_DEBUG("IDX UPDATED: PREV={}, CUR={}",
//                         getCamera()->mFrames->mPrevFrame, getCamera()->mFrames->mCurrentFrame);
            }
        }

    public:
        friend Kuafu;

        Context() = default;

        ~Context();

        Context(const Context &) = delete;

        Context(const Context &&) = delete;

        Context &operator=(const Context &) = delete;

        Context &operator=(const Context &&) = delete;
    };

}
