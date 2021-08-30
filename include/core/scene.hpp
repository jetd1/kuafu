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

#include "core/camera.hpp"
#include "core/geometry.hpp"
#include "core/config.hpp"

namespace kuafu {
class Context;

class Kuafu;

/// Stores all geoemtry, geometry instances and light sources.
/// Provides functions to change said data.
/// @todo removeGeometry()
/// @ingroup BASE
/// @ingroup API
class Scene {
public:
    friend Context;
    friend Kuafu;

    auto getGeometries() const -> const std::vector<std::shared_ptr<Geometry>> &;

    auto getGeometryByGlobalIndex(size_t index) const -> std::shared_ptr<Geometry>;

    /// @return Returns all geometry instances in the scene.
    auto getGeometryInstances() const -> const std::vector<std::shared_ptr<GeometryInstance>> &;

    auto getGeometryInstance(size_t index) const -> std::shared_ptr<GeometryInstance>;

    /// Used to submit a geometry instance for rendering.
    /// @param geometryInstance The instance to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void submitGeometryInstance(std::shared_ptr<GeometryInstance> geometryInstance);

    /// Used to submit multiple geometry instances for rendering, replacing all existing instances.
    /// @param geometryInstances The instances to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void setGeometryInstances(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances);

    /// Used to remove a geometry instance.
    ///
    /// Once a geometry instance was removed, it will no longer be rendered.
    /// @param geometryInstance The instance to remove.
    void removeGeometryInstance(std::shared_ptr<GeometryInstance> geometryInstance);

    /// Used to remove all geometry instances.
    ///
    /// However, geometries remain loaded and must be deleted explicitely.
    void clearGeometryInstances();

    /// Used to remove the last geometry instance.
    void popGeometryInstance();

    /// Used to submit a geometry and set up its buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometry The geometry to submit.
    void submitGeometry(std::shared_ptr<Geometry> geometry);

    /// Used to submit multiple geometries and set up their buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometries The geometries to submit.
    void setGeometries(const std::vector<std::shared_ptr<Geometry>> &geometries);

    /// Used to remove a geometry.
    /// @param geometry The geometry to remove.
    void removeGeometry(std::shared_ptr<Geometry> geometry);

    /// Used to remove a geometry.
    /// @param geometryIndex The geometry's index.
    void removeGeometry(uint32_t geometryIndex);

    /// Used to remove the last geometry and all its instances.
    void popGeometry();

    /// Used to remove all geometries
    void clearGeometries();

    /// Used to retrieve a geoemtry based on its path.
    /// @param path The geometry's model's path, relative to the path to assets.
    auto findGeometry(std::string_view path) const -> std::shared_ptr<Geometry>;

    void setEnvironmentMap(std::string_view path);

    void removeEnvironmentMap();

    /// Used to set a custom camera.
    /// @param camera A pointer to a kuafu::Camera object.
    void setCamera(std::shared_ptr<Camera> camera);

    void setCamera(int width, int height, const glm::vec3 &position = {0.0F, 0.0F, 3.0F});

    /// @return Returns a pointer to the renderer's camera.
    auto getCamera() const -> std::shared_ptr<Camera> { return mCurrentCamera; }

    inline auto getGeometryInstanceCount() { return mGeometries.size(); }

    inline void markGeometriesChanged() { mUploadGeometries = true; }

    inline void markGeometryInstancesChanged() { mUploadGeometryInstancesToBuffer = true; }

private:
    void initSceneDescriptorSets();

    void initGeoemtryDescriptorSets();

    void prepareBuffers();

    void initCameraBuffer();

    void uploadCameraBuffer(uint32_t imageIndex);

    void uploadEnvironmentMap();

    void uploadGeometries();

    void uploadGeometryInstances();

    void addDummy();

    void removeDummy();

    void translateDummy();

    void updateSceneDescriptors();

    void updateGeoemtryDescriptors();

    void upload(vk::Fence fence, uint32_t imageIndex);

    vkCore::Descriptors mSceneDescriptors;
    vkCore::Descriptors mGeometryDescriptors;

    std::vector<vk::DescriptorSet> mSceneDescriptorSets;
    std::vector<vk::DescriptorSet> mGeometryDescriptorSets;
    std::vector<vk::DescriptorSet> mTextureDescriptorSets;

    vkCore::Cubemap mEnvironmentMap;
    vk::UniqueSampler mImmutableSampler;

    std::vector<vkCore::StorageBuffer<uint32_t>> mIndexBuffers;
    std::vector<vkCore::StorageBuffer<uint32_t>> mMaterialIndexBuffers;
    std::vector<vkCore::StorageBuffer<Vertex>> mVertexBuffers;
    vkCore::StorageBuffer<MaterialSSBO> mMaterialBuffers;
    vkCore::StorageBuffer<GeometryInstanceSSBO> mGeometryInstancesBuffer;
    std::vector<std::shared_ptr<vkCore::Texture>> mTextures;

    vkCore::UniformBuffer<CameraUBO> mCameraUniformBuffer;

    std::vector<std::shared_ptr<Geometry>> mGeometries;
    std::vector<std::shared_ptr<GeometryInstance>> mGeometryInstances;

    std::string mEnvironmentMapTexturePath;
    bool mUseEnvironmentMap = false;

    bool mUploadGeometryInstancesToBuffer = false;
    bool mUploadEnvironmentMap = false;
    bool mUploadGeometries = false;
    bool mDummy = false;

    std::shared_ptr<Camera> mCurrentCamera;               ///< The camera that is currently being used for rendering.

    Config *mConfig = nullptr;
};
}
