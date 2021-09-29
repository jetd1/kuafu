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

#include "core/image.hpp"
#include "core/rt/as.hpp"
#include "core/geometry.hpp"
#include "core/config.hpp"

namespace kuafu {
struct RtPushConstants {
    glm::vec4 clearColor = glm::vec4(1.0F);
    int frameCount = 0;
    uint32_t sampleRatePerPixel = 4;
    uint32_t recursionDepth = 4;
    uint32_t useEnvironmentMap = 0;

    uint32_t russianRoulette = 0;
    uint32_t russianRouletteMinBounces = 0;
    uint32_t nextEventEstimation = 0;
    uint32_t nextEventEstimationMinBounces = 0;
};

struct PathTracingCapabilities {
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties; ///< The physical device's path tracing capabilities.
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;
    /*
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR pipelineFeatures;
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
    */
};

/// Manages the building process of the acceleration structures.
/// @ingroup API
class RayTracer {
public:
    RayTracer() = default;
    ~RayTracer();
    RayTracer(const RayTracer &) = delete;
    RayTracer(const RayTracer &&) = delete;
    auto operator=(const RayTracer &) -> RayTracer & = delete;
    auto operator=(const RayTracer &&) -> RayTracer & = delete;

    /// Retrieves the physical device's path tracing capabilities.
    void init();

    /// Destroys all bottom and top level acceleration structures.
    void destroy();

    /// @return Returns the top level acceleration structure.
    [[nodiscard]] const auto& getTlas() const { return mTlas; }

    [[nodiscard]] const auto& getCapabilities() { return mCapabilities; }

    inline void setRenderTargets(std::shared_ptr<RenderTargets> renderTargets) { mRenderTargets = renderTargets; }

    inline void createRenderTarget(const std::string& target, const vk::ImageCreateInfo& createInfo) {
        StorageImage storageImage;
        storageImage.init(createInfo);
        (*mRenderTargets)[target] = std::move(storageImage);
    }

    [[nodiscard]] auto getStorageImage(const std::string& target) const { return mRenderTargets->at(target).get(); }

    [[nodiscard]] auto getStorageImageView(const std::string& target) const { return mRenderTargets->at(target).getView(); }

    [[nodiscard]] const auto& getStorageImageInfo(const std::string& target) const { return mRenderTargets->at(target).getInfo(); }

    [[nodiscard]] auto getPipeline() const { return _pipeline.get(); }

    [[nodiscard]] auto getPipelineLayout() const { return _layout.get(); };

    [[nodiscard]] auto getDescriptorSetLayout() const { return mDescriptors.layout.get(); }

    [[nodiscard]] auto getDescriptorSet(size_t index) const { return mDescriptorSets[index]; }

    /// Used to create a empty blas.
    /// @return Returns a dummy bottom level acceleration structure.
    [[nodiscard]] auto createDummyBlas() const;

    /// Used to convert wavefront models to a bottom level acceleration structure.
    /// @param vertexBuffer A vertex buffer of some geometry.
    /// @param indexBuffer An index buffer of some geometry.
    /// @return Returns the bottom level acceleration structure.
    [[nodiscard]] auto modelToBlas(const vkCore::StorageBuffer<Vertex> &vertexBuffer,
                     const vkCore::StorageBuffer<uint32_t> &indexBuffer, bool opaque) const;

    /// Used to convert a bottom level acceleration structure instance to a Vulkan geometry instance.
    /// @param instance A bottom level acceleration structure instance.
    /// @return Returns the Vulkan geometry instance.
    auto geometryInstanceToAccelerationStructureInstance(
            std::shared_ptr<GeometryInstance> &geometryInstance);

    /// Used to prepare building the bottom level acceleration structures.
    /// @param vertexBuffers Vertex buffers of all geometry in the scene.
    /// @param indexBuffers Index buffers of all geometry in the scene.
    void createBottomLevelAS(std::vector<vkCore::StorageBuffer<Vertex>> &vertexBuffers,
                             const std::vector<vkCore::StorageBuffer<uint32_t>> &indexBuffers,
                             const std::vector<std::shared_ptr<Geometry>> &geometries);

    /// Builds all bottom level acceleration structures.
    /// @param blas_ A vector of kuafu::Blas objects containing all bottom level acceleration structures prepared in createBottomLevelAS().
    /// @param flags The build flags.
    void buildBlas(
            vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    /// Build the top level acceleration structure.
    /// @param instances A vector of bottom level acceleration structure instances.
    /// @param flags The build flags.
    void buildTlas(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances,
                   vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
                   bool reuse = false);

    void updateTlas(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances,
                    vk::BuildAccelerationStructureFlagsKHR flags =
                    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                    vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);

    /// Creates the storage image which the path tracing shaders will write to.
    /// @param swapchainExtent The swapchain images' extent.
    void createStorageImage(vk::Extent2D swapchainExtent);

    /// Creates the shader binding tables.
    void createShaderBindingTable();

    /// Used to create a path tracing pipeline.
    /// @param descriptorSetLayouts The descriptor set layouts for the shaders.
    /// @param settings The renderer settings.
    void createPipeline(const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts);

    /// Used to record the actual path tracing commands to a given command buffer.
    /// @param swapchainCommandBuffer The command buffer to record to.
    /// @param swapchainImage The current image in the swapchain.
    /// @param extent The swapchain images' extent.
    void trace(vk::CommandBuffer swapchainCommandBuffer, vk::Image swapchainImage, vk::Extent2D extent);

    void initDescriptorSet();

    void updateDescriptors();

    void initVarianceBuffer(float width, float height);

    float getPixelVariance(uint32_t index);

private:
    vk::UniquePipeline _pipeline;
    vk::UniquePipelineLayout _layout;
    uint32_t _shaderGroups;
    PathTracingCapabilities mCapabilities;
    std::vector<Blas> mBlas;
    Tlas mTlas; ///< The top level acceleration structure.
    vkCore::Buffer _instanceBuffer;
    vkCore::Buffer _sbtBuffer; ///< The shader binding table buffer.

    std::shared_ptr<RenderTargets> mRenderTargets;

    vkCore::Descriptors mDescriptors;
    std::vector<vk::DescriptorSet> mDescriptorSets;

    vkCore::Buffer _varianceBuffer;
};
}
