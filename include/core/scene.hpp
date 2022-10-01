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
#include "core/light.hpp"

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

    Scene() = delete;

    auto getGeometries() const -> const std::vector<std::shared_ptr<Geometry>> &;

    /// @return Returns all geometry instances in the scene.
    auto getGeometryInstances() const -> const std::vector<std::shared_ptr<GeometryInstance>> &;

    auto getGeometryInstance(size_t index) const -> std::shared_ptr<GeometryInstance>;

    /// Used to submit a geometry instance for rendering.
    /// @param geometryInstance The instance to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void submitGeometryInstance(std::shared_ptr<GeometryInstance> geometryInstance);
    void submitGeometryInstance(const GeometryInstance& geometryInstance);

    /// Used to submit multiple geometry instances for rendering, replacing all existing instances.
    /// @param geometryInstances The instances to queue for rendering.
    /// @note This function does not invoke any draw calls.
    void setGeometryInstances(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances);

    /// Used to remove a geometry instance.
    ///
    /// Once a geometry instance was removed, it will no longer be rendered.
    /// @param geometryInstance The instance to remove.
    void removeGeometryInstance(const std::shared_ptr<GeometryInstance> &geometryInstance);
    void removeGeometryInstances(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances);

    /// Used to remove all geometry instances.
    ///
    /// However, geometries remain loaded and must be deleted explicitely.
    void clearGeometryInstances();

    /// Used to submit a geometry and set up its buffers.
    ///
    /// Once a geometry was submitted, geometry instances referencing this particular geometry can be drawn.
    /// @param geometry The geometry to submit.
    void submitGeometry(std::shared_ptr<Geometry> geometry);
    void submitGeometry(const Geometry& geometry);

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

    /// Used to remove all geometries
    void clearGeometries();

    inline void setClearColor(const glm::vec4 &clearColor) {
        mClearColor = clearColor;
        pConfig->triggerSwapchainRefresh();
        global::frameCount = -1;
    };

    inline auto getClearColor() const { return mClearColor; };

    /// Used to retrieve a geoemtry based on its path.
    /// @param path The geometry's model's path, relative to the path to assets.
    auto findGeometry(std::string_view path) const -> std::shared_ptr<Geometry>;

    void setEnvironmentMap(std::string_view path);

    void removeEnvironmentMap();

    inline Camera* createCamera(int width, int height) {
        mRegisteredCameras.emplace_back(
                new Camera(width, height, glm::vec3(0.f, 0.f, 0.f)));
        return mRegisteredCameras.back().get();
    };

    inline void removeCamera(Camera* camera) {
        vkCore::global::device.waitIdle();

        KF_ASSERT(camera, "Trying to remove an invalid camera!");
        auto ret = std::find_if(mRegisteredCameras.begin(), mRegisteredCameras.end(),
                                [camera](auto& c) { return camera == c.get(); });
        KF_ASSERT(ret != mRegisteredCameras.end(),
                  "Trying to remove an camera which does not belong to the scene!");

        if (mCurrentCamera == camera) {
            KF_INFO("Removing the active camera. This may cause problems.");
            mCurrentCamera = nullptr;
        }

        mRegisteredCameras.erase(
                std::remove_if(mRegisteredCameras.begin(), mRegisteredCameras.end(),
                               [camera](auto &c) { return camera == c.get(); }),
                mRegisteredCameras.end());
    }

    /// Used to set a custom camera.
    /// @param camera A pointer to a kuafu::Camera object.
    void setCamera(Camera* camera);

    /// @return Returns a pointer to the renderer's camera.
    Camera* getCamera() const { return mCurrentCamera; }

    inline auto getGeometryInstanceCount() { return mGeometries.size(); }

    inline void markGeometriesChanged() { mUploadGeometries = true; }

    inline void markGeometryInstancesChanged() { mUploadGeometryInstancesToBuffer = true; }

    inline void setDirectionalLight(std::shared_ptr<DirectionalLight> light) { pDirectionalLight = light; };
    inline void removeDirectionalLight() { pDirectionalLight = nullptr; }

    inline void addPointLight(const std::shared_ptr<PointLight>& light) {
        if (pPointLights.size() >= global::maxPointLights)
            KF_WARN("Reached max point light number. The light will not be added!");
        else
            pPointLights.push_back(light);
    };
    inline void removePointLight(const std::shared_ptr<PointLight>& light) {
        KF_ASSERT(light, "Deleting an invalid light!");
        pPointLights.erase(
                std::remove_if(pPointLights.begin(), pPointLights.end(),
                               [light](auto &l) { return l == light; }),
                pPointLights.end());
    };

    inline void addActiveLight(const std::shared_ptr<ActiveLight>& light) {
        if (pActiveLights.size() >= global::maxActiveLights)
            KF_WARN("Reached max active light number. The light will not be added!");
        else {
            pActiveLights.push_back(light);
            markGeometriesChanged();          // uploads light texture inside uploadGeometries()
        }
    };
    inline void removeActiveLight(const std::shared_ptr<ActiveLight>& light) {
        KF_ASSERT(light, "Deleting an invalid light!");
        pActiveLights.erase(
                std::remove_if(pActiveLights.begin(), pActiveLights.end(),
                               [light](auto &l) { return l == light; }),
                pActiveLights.end());
    };

    inline void init() {               // TODO: This can be optimized
        static bool first = true;

        prepareBuffers();
        initSceneDescriptorSets();
        initGeometryDescriptorSets();

        if (first) {
            setEnvironmentMap("");
            uploadEnvironmentMap();
            removeEnvironmentMap();
            first = false;
        } else {
            uploadEnvironmentMap();
        }

        updateSceneDescriptors();

        markGeometriesChanged();
        markGeometryInstancesChanged();

        initialized = true;
    }


private:
    explicit Scene(std::shared_ptr<Config> pConfig): pConfig(pConfig) {
        setCamera(createCamera(1, 1));
    };

    void initSceneDescriptorSets();

    void initGeometryDescriptorSets();

    void prepareBuffers();

    void uploadUniformBuffers(uint32_t imageIndex);

    void uploadCameraBuffer(uint32_t imageIndex);

    void uploadLightBuffers(uint32_t imageIndex);

    void uploadEnvironmentMap();

    void uploadGeometries();

    void uploadGeometryInstances();

    void addDummy();

    void removeDummy();

    void translateDummy();

    void updateSceneDescriptors();

    void updateGeometryDescriptors();

    void upload(vk::Fence fence, uint32_t imageIndex);

    bool initialized = false;

    glm::vec4 mClearColor = glm::vec4(0.F, 0.F, 0.F, 1.F);

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
    vkCore::StorageBuffer<NiceMaterialSSBO> mMaterialBuffers;
    vkCore::StorageBuffer<GeometryInstanceSSBO> mGeometryInstancesBuffer;
    std::vector<std::shared_ptr<vkCore::Texture>> mTextures;

    vkCore::UniformBuffer<CameraUBO> mCameraUniformBuffer;

    std::vector<std::shared_ptr<Geometry>> mGeometries;
    std::vector<std::shared_ptr<GeometryInstance>> mGeometryInstances;

    std::shared_ptr<DirectionalLight> pDirectionalLight;
    vkCore::UniformBuffer<DirectionalLightUBO> mDirectionalLightUniformBuffer;

    std::vector<std::shared_ptr<PointLight>> pPointLights;
    vkCore::UniformBuffer<PointLightsUBO> mPointLightsUniformBuffer;

    std::vector<std::shared_ptr<ActiveLight>> pActiveLights;
    vkCore::UniformBuffer<ActiveLightsUBO> mActiveLightsUniformBuffer;

    std::string mEnvironmentMapTexturePath;
    bool mUseEnvironmentMap = false;

    bool mUploadGeometryInstancesToBuffer = false;
    bool mUploadEnvironmentMap = false;
    bool mUploadGeometries = false;
    bool mDummy = false;

    std::vector<std::unique_ptr<Camera>> mRegisteredCameras;
    Camera* mCurrentCamera = nullptr;      ///< The camera that is currently being used for rendering.
    std::shared_ptr<Config> pConfig = nullptr;
};
}
