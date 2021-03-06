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
#include "core/postprocess.hpp"
#include "core/context/global.hpp"

namespace kuafu {
/*
void PostProcessingRenderer::initDepthImage( vk::Extent2D extent )
{
  vk::Format format = vkCore::getSupportedDepthFormat( vkCore::global::physicalDevice );

  auto imageCreateInfo   = vkCore::getImageCreateInfo( vk::Extent3D( extent, 1 ) );
  imageCreateInfo.format = format;
  imageCreateInfo.usage  = vk::ImageUsageFlagBits::eDepthStencilAttachment;
  _depthImage.init( imageCreateInfo );

  auto imageViewCreateInfo = vkCore::getImageViewCreateInfo( _depthImage.get( ), format, vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eDepth );
  _depthImageView          = vkCore::initImageViewUnique( imageViewCreateInfo );

  vk::ImageSubresourceRange subresourceRange( vk::ImageAspectFlagBits::eDepth, // aspectMask
                                              0,                               // baseMipLevel
                                              VK_REMAINING_MIP_LEVELS,         // levelCount
                                              0,                               // baseArrayLayer
                                              VK_REMAINING_ARRAY_LAYERS );     // layerCount

  _depthImage.transitionToLayout( vk::ImageLayout::eDepthStencilAttachmentOptimal, &subresourceRange );
}
*/

void PostProcessingRenderer::initRenderPass(vk::Format format) {
    // Color attachment
    vk::AttachmentDescription colorAttachmentDescription({},                               // flags
                                                         format,                            // format
                                                         vk::SampleCountFlagBits::e1,       // samples
                                                         vk::AttachmentLoadOp::eClear,      // loadOp
                                                         {},                               // storeOp
                                                         {},                               // stencilLoadOp
                                                         {},                               // stencilStoreOp
                                                         {},                               // initialLayout
                                                         vk::ImageLayout::ePresentSrcKHR); // finalLayout

    vk::AttachmentReference colorAttachmentReference(0,                                          // attachment
                                                     vk::ImageLayout::eColorAttachmentOptimal); // layout

    vk::SubpassDescription subpassDescription({},                              // flags
                                              vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
                                              0,                                // inputAttachmentsCount
                                              nullptr,                          // pInputAttachments
                                              1,                                // colorAttachmentsCount
                                              &colorAttachmentReference,        // pColorAttachments
                                              nullptr,                          // pResolveAttachments
                                              nullptr,                          // pDepthStencilAttachment
                                              0,                                // preserveAttachemntCount
                                              nullptr);                        // pPreserveAttachments

    vk::SubpassDependency subpassDependencies(
            VK_SUBPASS_EXTERNAL,                                                                  // srcSubpass
            0,                                                                                    // dstSubpass
            vk::PipelineStageFlagBits::eBottomOfPipe,                                             // srcStageMask
            vk::PipelineStageFlagBits::eColorAttachmentOutput,                                    // dstStageMask
            vk::AccessFlagBits::eMemoryRead,                                                      // srcAccessMask
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, // dstAccessMask
            vk::DependencyFlagBits::eByRegion);                                                  // dependencyFlags

    mRenderPass.init({colorAttachmentDescription}, {subpassDescription}, {subpassDependencies});
}

void PostProcessingRenderer::initDescriptorSet() {
    // Color image binding
    mDescriptors.bindings.add(0,
                              vk::DescriptorType::eCombinedImageSampler,
                              vk::ShaderStageFlagBits::eFragment);

    mDescriptors.layout = mDescriptors.bindings.initLayoutUnique();
    mDescriptors.pool = mDescriptors.bindings.initPoolUnique(vkCore::global::swapchainImageCount);
    mDescriptorSets = vkCore::allocateDescriptorSets(mDescriptors.pool.get(), mDescriptors.layout.get());
}

void PostProcessingRenderer::updateDescriptors(const vk::DescriptorImageInfo &imageInfo) {
//        KF_ASSERT( imageInfo.imageView && imageInfo.sampler, "Failed to update post processing renderer descriptor sets because storage image info contains invalid elements." );

mDescriptors.bindings.write(mDescriptorSets, 0, &imageInfo);
mDescriptors.bindings.update();
}

void PostProcessingRenderer::initPipeline() {
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({},       // flags
                                                           0,         // vertexBindingDescriptionCount
                                                           nullptr,   // pVertexBindingDescriptions
                                                           0,         // vertexAttributeDescriptionCount
                                                           nullptr); // pVertexAttributeDescriptions

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({},                                  // flags
                                                           vk::PrimitiveTopology::eTriangleList, // topology
                                                           VK_FALSE);                           // primitiveRestartEnable

    vk::PipelineViewportStateCreateInfo viewportState({},       // flags
                                                      1,         // viewportCount
                                                      nullptr,   // pViewports
                                                      1,         // scissorCount
                                                      nullptr); // pScissors

    vk::PipelineRasterizationStateCreateInfo rasterizer({},                              // flags
                                                        VK_FALSE,                         // depthClampEnable
                                                        VK_FALSE,                         // rasterizerDiscardEnable
                                                        vk::PolygonMode::eFill,           // polygonMode
                                                        vk::CullModeFlagBits::eNone,      // cullMode
                                                        vk::FrontFace::eCounterClockwise, // frontFace
                                                        VK_FALSE,                         // depthBiasEnable
                                                        0.0F,                             // depthBiasConstantFactor
                                                        0.0F,                             // depthBiasClamp
                                                        0.0F,                             // depthBiasSlopeFactor
                                                        1.0F);                           // lineWidth

    vk::PipelineMultisampleStateCreateInfo multisampling({},                         // flags
                                                         vk::SampleCountFlagBits::e1, // rasterizationSamples
                                                         VK_FALSE,                    // sampleShadingEnable
                                                         0.0F,                        // minSampleShading
                                                         nullptr,                     // pSampleMask
                                                         VK_FALSE,                    // alphaToCoverageEnable
                                                         VK_FALSE);                  // alphaToOneEnable

    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
            VK_FALSE,                                                                                                                            // blendEnable
            {},                                                                                                                                 // srcColorBlendFactor
            {},                                                                                                                                 // dstColorBlendFactor
            {},                                                                                                                                 // colorBlendOp
            {},                                                                                                                                 // srcAlphaBlendFactor
            {},                                                                                                                                 // dstAlphaBlendFactor
            {},                                                                                                                                 // alphaBlendOp
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA); // colorWriteMask

    vk::PipelineColorBlendStateCreateInfo colorBlending({},                          // flags
                                                        VK_FALSE,                     // logicOpEnable
                                                        vk::LogicOp::eClear,          // logicOp)
                                                        1,                            // attachmentCount
                                                        &colorBlendAttachment,        // pAttachments
                                                        {0.0F, 0.0F, 0.0F, 0.0F}); // blendConstants

    std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({},                                            // flags
                                                        static_cast<uint32_t>( dynamicStates.size()), // dynamicStateCount
                                                        dynamicStates.data());                        // pDynamicStates

    vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eFragment, // stageFlags
                                            0,                                  // offset
                                            sizeof(float));                  // size

    vk::PipelineLayoutCreateInfo layoutCreateInfo({},                         // flags
                                                  1,                           // setLayoutCount
                                                  &mDescriptors.layout.get(), // pSetLayouts
                                                  1,                           // pushConstantRangeCount
                                                  &pushConstantRange);        // pPushConstantRanges

    mPipelineLayout = vkCore::global::device.createPipelineLayoutUnique(layoutCreateInfo);
//        KF_ASSERT( mPipelineLayout.get( ), "Failed to create pipeline layout for post processing renderer." );

    auto vert = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PostProcessing.vert", KF_GLSLC_PATH);
    auto frag = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PostProcessing.frag", KF_GLSLC_PATH);

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
    shaderStages[0] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, vert.get());
    shaderStages[1] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, frag.get());

    vk::GraphicsPipelineCreateInfo createInfo({},                                           // flags
                                              static_cast<uint32_t>( shaderStages.size()), // stageCount
                                              shaderStages.data(),                          // pStages
                                              &vertexInputInfo,                              // pVertexInputState
                                              &inputAssembly,                                // pInputAssemblyState
                                              nullptr,                                       // pTessellationState
                                              &viewportState,                                // pViewportStage
                                              &rasterizer,                                   // pRasterizationState
                                              &multisampling,                                // pMultisampleState
                                              nullptr,                                       // pDepthStencilState
                                              &colorBlending,                                // pColorBlendState
                                              &dynamicStateInfo,                             // pDynamicState
        mPipelineLayout.get(),                        // layout
        mRenderPass.get(),                            // renderPass
                                              0,                                             // subpass
                                              nullptr,                                       // basePipelineHandle
                                              0);                                           // basePipelineIndex

    mPipeline = static_cast<vk::UniquePipeline>(
            vkCore::global::device.createGraphicsPipelineUnique(nullptr, createInfo, nullptr).value);
    assert(mPipeline.get()); // "Failed to create rasterization pipeline."
}

void PostProcessingRenderer::beginRenderPass(vk::CommandBuffer commandBuffer, vk::Framebuffer framebuffer,
                                             vk::Extent2D size) {
    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0F, 0};

    mRenderPass.begin(framebuffer, commandBuffer, {0, size}, {clearValues[0], clearValues[1]});
}

void PostProcessingRenderer::endRenderPass(vk::CommandBuffer commandBuffer) {
  mRenderPass.end(commandBuffer);
}

void PostProcessingRenderer::render(vk::CommandBuffer commandBuffer, vk::Extent2D size, size_t imageIndex) {
    auto width = static_cast<float>( size.width );
    auto height = static_cast<float>( size.height );

    commandBuffer.setViewport(0, {vk::Viewport(0.0F, 0.0F, width, height, 0.0F, 1.0F)});
    commandBuffer.setScissor(0, {{{0, 0}, size}});

    auto aspectRatio = width / height;
    commandBuffer.pushConstants(mPipelineLayout.get(),             // layout
                                vk::ShaderStageFlagBits::eFragment, // stageFlags
                                0,                                  // offset
                                sizeof(float),                    // size
                                &aspectRatio);                     // pValues

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline.get());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
                                     mPipelineLayout.get(),           // layout
                                     0,                                // first set
                                     1,                                // descriptor set count
                                     &mDescriptorSets[imageIndex],     // descriptor sets
                                     0,                                // dynamic offset count
                                     nullptr);                        // dynamic offsets

    commandBuffer.draw(3, 1, 0, 0);
}
}
