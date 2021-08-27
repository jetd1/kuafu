//
// Created by jet on 4/9/21.
//

#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <vkCore/vkCore.hpp>
#include "core/window.hpp"
#include "core/camera.hpp"
#include "core/gui.hpp"
#include "core/scene.hpp"
#include "core/postprocess.hpp"
#include "core/rt/rt.hpp"

namespace kuafu {

    class Kuafu;

    class Context {
        std::shared_ptr<Window> mWindow = nullptr;
        std::shared_ptr<Camera> mCamera = nullptr;
        vk::UniqueInstance mInstance;

#ifdef VK_VALIDATION
        vkCore::DebugMessenger mDebugMessenger;
#endif

        vkCore::Surface mSurface;
        vk::UniqueDevice mDevice;
        vk::UniqueCommandPool mGraphicsCmdPool;
        vk::UniqueCommandPool mTransferCmdPool;

        RayTracer mRayTracer;
        PostProcessingRenderer mPostProcessingRenderer;

        vkCore::Sync mSync;
        vkCore::Swapchain mSwapchain;
        vkCore::CommandBuffer mSwapchainCommandBuffers;

        std::shared_ptr<Gui> mGui = nullptr;

        Scene mScene;
        Config mConfig;


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

        /// Submits the swapchain command buffers to a queue and presents an image on the screen.
        void submitFrame();


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
