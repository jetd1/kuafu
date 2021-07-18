//
// Created by jet on 4/9/21.
//

#pragma once

#include "stdafx.hpp"

namespace kuafu {
    /// The post processing renderer acts as a second render pass for enabling post processing operations, such as gamma correction.
    /// @ingroup API
    class PostProcessingRenderer {
    public:
        auto getRenderPass() const -> const vkCore::RenderPass & { return _renderPass; }

        auto getPipeline() const -> const vk::Pipeline { return _pipeline.get(); }

        auto getPipelineLayout() const -> const vk::PipelineLayout { return _pipelineLayout.get(); }

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

        vkCore::RenderPass _renderPass;

        vk::UniquePipeline _pipeline;
        vk::UniquePipelineLayout _pipelineLayout;

        vkCore::Descriptors _descriptors;
        std::vector<vk::DescriptorSet> _descriptorSets;
    };
}
