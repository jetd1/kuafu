//
// Created by jet on 4/9/21.
//

#include "core/gui.hpp"
#include "core/context/global.hpp"

namespace kuafu {
    void Gui::configure() {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
    }

    void Gui::init(SDL_Window *window, const vkCore::Surface *const surface, vk::Extent2D swapchainImageExtent,
                   vk::RenderPass renderPass) {
        _window = window;
        _swapchainImageExtent = swapchainImageExtent;
//
//        configure();
//
////        KF_ASSERT( ImGui_ImplSDL2_InitForVulkan( _window ), "Failed to init ImGui for Vulkan" );
//        assert(ImGui_ImplSDL2_InitForVulkan(_window));
//
//        initDescriptorPool();
//
//        ImGui_ImplVulkan_InitInfo init_info{};
//        init_info.Instance = static_cast<VkInstance>( vkCore::global::instance );
//        init_info.PhysicalDevice = static_cast<VkPhysicalDevice>( vkCore::global::physicalDevice );
//        init_info.Device = static_cast<VkDevice>( vkCore::global::device );
//        init_info.QueueFamily = vkCore::global::graphicsFamilyIndex;
//        init_info.Queue = static_cast<VkQueue>( vkCore::global::graphicsQueue );
//        init_info.PipelineCache = NULL;
//        init_info.DescriptorPool = static_cast<VkDescriptorPool>( _descriptorPool.get());
//        init_info.Allocator = NULL;
//        init_info.MinImageCount = surface->getCapabilities().minImageCount + 1;
//        init_info.ImageCount = static_cast<uint32_t>( vkCore::global::swapchainImageCount );
//
////        KF_ASSERT( ImGui_ImplVulkan_Init( &init_info, static_cast<VkRenderPass>( renderPass ) ), "Failed to initialize Vulkan for ImGui." );
//        assert(ImGui_ImplVulkan_Init(&init_info, static_cast<VkRenderPass>( renderPass )));
//
//        initFonts();
    }

    void Gui::recreate(vk::Extent2D swapchainImageExtent) {
//        _swapchainImageExtent = swapchainImageExtent;
    }

    void Gui::render() {
//        ImGui::ShowDemoWindow();
    }

    void Gui::renderDrawData(vk::CommandBuffer commandBuffer) {
//        ImGui_ImplSDL2_NewFrame(_window);
//        ImGui::NewFrame();
//
//        render();
//        ImGui::Render();
//
//        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>( commandBuffer ));
    }

    void Gui::destroy() {
//        ImGui_ImplVulkan_Shutdown();
//        ImGui_ImplSDL2_Shutdown();
//        ImGui::DestroyContext();
    }

    void Gui::initDescriptorPool() {
//        std::vector<vk::DescriptorPoolSize> poolSizes = {{vk::DescriptorType::eSampler,              1000},
//                                                         {vk::DescriptorType::eCombinedImageSampler, 1000},
//                                                         {vk::DescriptorType::eSampledImage,         1000},
//                                                         {vk::DescriptorType::eStorageImage,         1000},
//                                                         {vk::DescriptorType::eUniformTexelBuffer,   1000},
//                                                         {vk::DescriptorType::eStorageTexelBuffer,   1000},
//                                                         {vk::DescriptorType::eUniformBuffer,        1000},
//                                                         {vk::DescriptorType::eStorageBuffer,        1000},
//                                                         {vk::DescriptorType::eUniformBufferDynamic, 1000},
//                                                         {vk::DescriptorType::eStorageBufferDynamic, 1000},
//                                                         {vk::DescriptorType::eInputAttachment,      1000}};
//
//        _descriptorPool = vkCore::initDescriptorPoolUnique(poolSizes, vkCore::global::swapchainImageCount);
    }

    void Gui::initFonts() {
//        _commandPool = vkCore::initCommandPoolUnique(vkCore::global::graphicsFamilyIndex,
//                                                     vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
//
//        vkCore::CommandBuffer commandBuffer(_commandPool.get());
//        commandBuffer.begin();
//
////        KF_ASSERT( ImGui_ImplVulkan_CreateFontsTexture( static_cast<VkCommandBuffer>( commandBuffer.get( 0 ) ) ), "Failed to create ImGui fonts texture." );
//        assert(ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>( commandBuffer.get(0))));
//
//        commandBuffer.end();
//        commandBuffer.submitToQueue(vkCore::global::graphicsQueue);
    }
}
