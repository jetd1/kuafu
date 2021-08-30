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

#include "core/gui.hpp"
#include "core/context/global.hpp"

namespace kuafu {
void Gui::configure() {
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
}

void Gui::init(SDL_Window *window, const vkCore::Surface *const surface, vk::Extent2D swapchainImageExtent,
               vk::RenderPass renderPass) {
  pWindow = window;
    mSwapchainImageExtent = swapchainImageExtent;
//
//        configure();
//
////        KF_ASSERT( ImGui_ImplSDL2_InitForVulkan( pWindow ), "Failed to init ImGui for Vulkan" );
//        assert(ImGui_ImplSDL2_InitForVulkan(pWindow));
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
//        init_info.DescriptorPool = static_cast<VkDescriptorPool>( mDescriptorPool.get());
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
//        mSwapchainImageExtent = swapchainImageExtent;
}

void Gui::render() {
//        ImGui::ShowDemoWindow();
}

void Gui::renderDrawData(vk::CommandBuffer commandBuffer) {
//        ImGui_ImplSDL2_NewFrame(pWindow);
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
//        mDescriptorPool = vkCore::initDescriptorPoolUnique(poolSizes, vkCore::global::swapchainImageCount);
}

void Gui::initFonts() {
//        mCommandPool = vkCore::initCommandPoolUnique(vkCore::global::graphicsFamilyIndex,
//                                                     vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
//
//        vkCore::CommandBuffer commandBuffer(mCommandPool.get());
//        commandBuffer.begin();
//
////        KF_ASSERT( ImGui_ImplVulkan_CreateFontsTexture( static_cast<VkCommandBuffer>( commandBuffer.get( 0 ) ) ), "Failed to create ImGui fonts texture." );
//        assert(ImGui_ImplVulkan_CreateFontsTexture(static_cast<VkCommandBuffer>( commandBuffer.get(0))));
//
//        commandBuffer.end();
//        commandBuffer.submitToQueue(vkCore::global::graphicsQueue);
}
}
