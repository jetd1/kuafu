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

#include "stdafx.hpp"

namespace kuafu {
/// The post processing renderer acts as a second render pass for enabling post processing operations, such as gamma correction.
/// @ingroup API
class PostProcessingRenderer {
public:
    auto getRenderPass() const -> const vkCore::RenderPass & { return mRenderPass; }

    auto getPipeline() const -> const vk::Pipeline { return mPipeline.get(); }

    auto getPipelineLayout() const -> const vk::PipelineLayout { return mPipelineLayout.get(); }

    //void initDepthImage( vk::Extent2D extent );

    void initRenderPass(vk::Format format);

    void initDescriptorSet();

    /// @param imageInfo The descriptor image info of the path tracer's storage image.
    void updateDescriptors(const vk::DescriptorImageInfo &imageInfo);

    void initPipeline();

    void beginRenderPass(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer, vk::Extent2D size);

    void endRenderPass(vk::CommandBuffer commandBuffer);

    /// Records the draw calls to a given command buffer.
    /// @param imageIndex The current swapchain image index for selecting the correct descriptor set.
    void render(vk::CommandBuffer commandBuffer, vk::Extent2D size, size_t imageIndex);

private:
    //vkCore::Image _depthImage;
    //vk::UniqueImageView _depthImageView;

    vkCore::RenderPass mRenderPass;

    vk::UniquePipeline mPipeline;
    vk::UniquePipelineLayout mPipelineLayout;

    vkCore::Descriptors mDescriptors;
    std::vector<vk::DescriptorSet> mDescriptorSets;
};
}
