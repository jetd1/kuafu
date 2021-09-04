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
#include "core/scene.hpp"
#include "core/context/global.hpp"
#include <kuafu_utils.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

//#include <gltf/tiny_gltf.h>

namespace kuafu {
CameraUBO cameraUBO;
DirectionalLightUBO directionalLightUBO;
PointLightsUBO pointLightsUBO;
ActiveLightsUBO activeLightsUBO;

std::shared_ptr<Geometry> triangle = nullptr; ///< A dummy triangle that will be placed in the scene if it empty. This assures the AS creation.
std::shared_ptr<GeometryInstance> triangleInstance = nullptr;

std::vector<GeometryInstanceSSBO> memAlignedGeometryInstances;
std::vector<NiceMaterialSSBO> memAlignedMaterials;

auto Scene::getGeometries() const -> const std::vector<std::shared_ptr<Geometry>> & {
    return mGeometries;
}

auto Scene::getGeometryByGlobalIndex(size_t index) const -> std::shared_ptr<Geometry> {
    for (auto p: mGeometries)
        if (index == p->geometryIndex)
            return p;

    KF_WARN("Geometry not found");
    return nullptr;
}

auto Scene::getGeometryInstances() const -> const std::vector<std::shared_ptr<GeometryInstance>> & {
    return mGeometryInstances;
}

auto Scene::getGeometryInstance(size_t index) const -> std::shared_ptr<GeometryInstance> {
    if (index < mGeometryInstances.size())
        return mGeometryInstances[index];
    else
        throw std::runtime_error("Geometry Instances out of bound.");
    return nullptr;
}

void Scene::submitGeometryInstance(std::shared_ptr<GeometryInstance> geometryInstance) {
    if (!mDummy) {
        if (mGeometryInstances.size() > pConfig->mMaxGeometryInstances) {
            throw std::runtime_error(
                    "Failed to submit geometry instance because instance buffer size has been exceeded.");
            return;
        }
    }

    mGeometryInstances.push_back(geometryInstance);
    markGeometryInstancesChanged();
}

void Scene::setGeometryInstances(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances) {
    mGeometryInstances.clear();
    mGeometryInstances.reserve(geometryInstances.size());

    for (auto geometryInstance : geometryInstances) {
        submitGeometryInstance(geometryInstance);
    }

    markGeometryInstancesChanged();
}

void Scene::removeGeometryInstance(std::shared_ptr<GeometryInstance> geometryInstance) {
    if (geometryInstance == nullptr) {
        throw std::runtime_error("An invalid geometry instance can not be removed.");
        return;
    }

    // Create a copy of all geometry instances
    std::vector<std::shared_ptr<GeometryInstance>> temp(mGeometryInstances);

    // Clear the original container
    mGeometryInstances.clear();
    mGeometryInstances.reserve(temp.size());

    // Iterate over the container with the copies
    for (auto it : temp) {
        // Delete the given geometry instance if it matches
        if (it != geometryInstance) {
            mGeometryInstances.push_back(it);
        }
    }

    markGeometryInstancesChanged();
}

void Scene::clearGeometryInstances() {
    // Only allow clearing the scene if there is no dummy element.
    if (!mDummy) {
        mGeometryInstances.clear();
        markGeometryInstancesChanged();
    }
}

void Scene::popGeometryInstance() {
    // Only allow clearing the scene if the scene is not empty and does not contain a dummy element.
    if (!mGeometryInstances.empty() && !mDummy) {
        mGeometryInstances.erase(mGeometryInstances.end() - 1);
        markGeometryInstancesChanged();
    }
}

void Scene::submitGeometry(std::shared_ptr<Geometry> geometry) {
    if (!mDummy) {
        if (mGeometries.size() >= pConfig->mMaxGeometry) {
            throw std::runtime_error(
                    "Failed to submit geometry because geometries buffer size has been exceeded.");
            return;
        }
    }

    mGeometries.push_back(geometry);
    markGeometriesChanged();
}

void Scene::setGeometries(const std::vector<std::shared_ptr<Geometry>> &geometries) {
    mGeometries.clear();
    mGeometries.reserve(geometries.size());

    for (auto geometry : geometries) {
        submitGeometry(geometry);
    }

    markGeometriesChanged();
}

void Scene::removeGeometry(std::shared_ptr<Geometry> geometry) {
    if (geometry == nullptr) {
        throw std::runtime_error("An invalid geometry can not be removed.");
        return;
    }

    // Removing a geometry also means removing all its instances.
    std::vector<std::shared_ptr<GeometryInstance>> instancesToDelete;
    for (auto it : mGeometryInstances) {
        if (it->geometryIndex == geometry->geometryIndex) {
            instancesToDelete.push_back(it);
        }
    }

    // Remove all instances of that geometry index
    // Create a copy of all geometry instances
    std::vector<std::shared_ptr<GeometryInstance>> temp3(mGeometryInstances);

    // Clear the original container
    mGeometryInstances.clear();
    mGeometryInstances.reserve(temp3.size());

    // Iterate over the container with the copies
    for (auto it : temp3) {
        // Delete the given geometry instance if it matches
        if (it->geometryIndex != geometry->geometryIndex) {
            mGeometryInstances.push_back(it);
        }
    }

    markGeometryInstancesChanged();

    std::vector<std::shared_ptr<Geometry>> temp(mGeometries);
    mGeometries.clear();
    mGeometries.reserve(temp.size());

    uint32_t geometryIndex = 0;
    for (auto it : temp) {
        if (it != geometry) {
            it->geometryIndex = geometryIndex++;
            mGeometries.push_back(it);
        }
    }

    --global::geometryIndex;
    markGeometriesChanged(); // @todo Might not be necessary.

    // Update geometry indices for geometry instances.
    std::vector<std::shared_ptr<GeometryInstance>> temp2(mGeometryInstances);
    mGeometryInstances.clear();
    mGeometries.reserve(temp2.size());

    geometryIndex = 0;
    for (auto it : temp2) {
        if (it->geometryIndex > geometry->geometryIndex) {
            --it->geometryIndex;
            mGeometryInstances.push_back(it);
        } else {
            mGeometryInstances.push_back(it);
        }
    }

    markGeometryInstancesChanged();
}

void Scene::removeGeometry(uint32_t geometryIndex) {
    for (auto it : mGeometries) {
        if (it->geometryIndex == geometryIndex) {
            removeGeometry(it);
            break;
        }
    }
}

void Scene::clearGeometries() {
//        KF_INFO( "Clearing geometry." );

    mGeometries.clear();
    mGeometryInstances.clear();

    // Reset index counter.
    global::geometryIndex = 0;

    // Reset texture counter.
    global::textureIndex = 0;

    markGeometriesChanged();
    markGeometryInstancesChanged();
}

void Scene::popGeometry() {
    if (!mGeometries.empty()) {
        removeGeometry(*(mGeometries.end() - 1));
    }
}

auto Scene::findGeometry(std::string_view path) const -> std::shared_ptr<Geometry> {
    for (std::shared_ptr<Geometry> geometry : mGeometries) {
        if (geometry->path == path) {
            return geometry;
        }
    }
    return nullptr;
}

void Scene::setEnvironmentMap(std::string_view path) {
  mEnvironmentMapTexturePath = path;
  mUseEnvironmentMap = true;
  mUploadEnvironmentMap = true;
}

void Scene::removeEnvironmentMap() { mUseEnvironmentMap = false;
}

void Scene::setCamera(std::shared_ptr<Camera> camera) {
//        mCameras.insert(camera);
    mCurrentCamera = camera;
}

void Scene::setCamera(int width, int height, const glm::vec3 &position) {
    auto cam = std::make_shared<Camera>(width, height, position);
//        mCameras.insert(cam);
    mCurrentCamera = cam;
}

void Scene::prepareBuffers() {
    // Resize and initialize buffers with "dummy data".
    // The advantage of doing this is that the buffers are all initialized right away (even though it is invalid data) and
    // this makes it possible to call fill instead of initialize again, when changing any of the data below.
    std::vector<GeometryInstanceSSBO> geometryInstances(pConfig->mMaxGeometryInstances);
    mGeometryInstancesBuffer.init(geometryInstances, global::maxResources);

    // @todo is this even necessary?
    std::vector<NiceMaterialSSBO> materials(pConfig->mMaxMaterials);
    mMaterialBuffers.init(materials, global::maxResources);

    mVertexBuffers.resize(pConfig->mMaxGeometry);
    mIndexBuffers.resize(pConfig->mMaxGeometry);
    mMaterialIndexBuffers.resize(pConfig->mMaxGeometry);
    mTextures.resize(pConfig->mMaxTextures);

    mCameraUniformBuffer.init();
    mDirectionalLightUniformBuffer.init();
    mPointLightsUniformBuffer.init();
    mActiveLightsUniformBuffer.init();
}


void Scene::uploadUniformBuffers(uint32_t imageIndex) {
    uploadCameraBuffer(imageIndex);
    uploadLightBuffers(imageIndex);
}

void Scene::uploadCameraBuffer(uint32_t imageIndex) {
    // Upload camera.
    if (mCurrentCamera != nullptr) {
        if (mCurrentCamera->mViewNeedsUpdate) {
            cameraUBO.view = mCurrentCamera->getViewMatrix();
            cameraUBO.viewInverse = mCurrentCamera->getViewInverseMatrix();

            mCurrentCamera->mViewNeedsUpdate = false;
        }

        if (mCurrentCamera->mProjNeedsUpdate) {
            cameraUBO.projection = mCurrentCamera->getProjectionMatrix();
            cameraUBO.projectionInverse = mCurrentCamera->getProjectionInverseMatrix();

            mCurrentCamera->mProjNeedsUpdate = false;
        }

        cameraUBO.position = glm::vec4(mCurrentCamera->getPosition(), mCurrentCamera->getAperture());
        cameraUBO.front = glm::vec4(mCurrentCamera->getFront(), mCurrentCamera->getFocalLength());
    }

    mCameraUniformBuffer.upload(imageIndex, cameraUBO);
}

void Scene::uploadLightBuffers(uint32_t imageIndex) {
    if (pDirectionalLight) {
        directionalLightUBO.direction = {glm::normalize(pDirectionalLight->direction), pDirectionalLight->softness};
        directionalLightUBO.rgbs = {pDirectionalLight->color, pDirectionalLight->strength};
    }
    mDirectionalLightUniformBuffer.upload(imageIndex, directionalLightUBO);

    for (size_t i = 0; i < global::maxPointLights; i++) {
        if (i < pPointLights.size()) {
            KF_ASSERT(pPointLights[i] != nullptr, "Invalid point light!");
            pointLightsUBO.posr[i] = {pPointLights[i]->position, pPointLights[i]->radius};
            pointLightsUBO.rgbs[i] = {pPointLights[i]->color, pPointLights[i]->strength};
        } else
            pointLightsUBO.rgbs[i][3] = 0.0;
    }
    mPointLightsUniformBuffer.upload(imageIndex, pointLightsUBO);

    for (size_t i = 0; i < global::maxActiveLights; i++) {
        if (i < pActiveLights.size()) {
            KF_ASSERT(pActiveLights[i] != nullptr, "Invalid point light!");

            auto& viewMat = pActiveLights[i]->viewMat;
            auto viewMatInv = glm::inverse(viewMat);
            activeLightsUBO.viewMat[i] = viewMat;
            activeLightsUBO.projMat[i] = glm::perspective(
                    pActiveLights[i]->fov, 1.F, 0.01F, 1000.0F);
            activeLightsUBO.front[i] = {-viewMatInv[2][0], -viewMatInv[2][1], -viewMatInv[2][2], 1};
            activeLightsUBO.rgbs[i] = {pActiveLights[i]->color, pActiveLights[i]->strength};
            activeLightsUBO.position[i] = {viewMatInv[3][0], viewMatInv[3][1], viewMatInv[3][2], 0};

            activeLightsUBO.sftp[i] = {pActiveLights[i]->softness, pActiveLights[i]->fov, pActiveLights[i]->texID, 0};
        } else
            activeLightsUBO.front[i][3] = 0;         // if front[i].w = 0, no light
    }
    mActiveLightsUniformBuffer.upload(imageIndex, activeLightsUBO);
}

void Scene::uploadEnvironmentMap() {
  mUploadEnvironmentMap = false;
    if (!mUseEnvironmentMap || mEnvironmentMapTexturePath.empty()) {
      mEnvironmentMap.init("");
        return;
    }

    auto ext = std::filesystem::path(mEnvironmentMapTexturePath).extension();
    if (kuafu::utils::iequals(ext, ".ktx")) {
      mEnvironmentMap.init(mEnvironmentMapTexturePath);
        return;
    }

    // TODO: build cube map from 6 images
    throw std::runtime_error(
            "cubemap format not supported: " + mEnvironmentMapTexturePath);
}

void Scene::uploadGeometries() {
  mUploadGeometries = false;

    memAlignedMaterials.clear();
    memAlignedMaterials.reserve(global::materials.size());

    // Create all textures of a material
    for (size_t i = 0; i < global::materials.size(); ++i) {
        // Convert to memory aligned struct
        NiceMaterialSSBO mat2 {
                .diffuse = {global::materials[i].diffuseColor, 0},
                .emission = {global::materials[i].emission, global::materials[i].emissionStrength},
                .alpha = global::materials[i].alpha,
                .metallic = global::materials[i].metallic,
                .specular = global::materials[i].specular,
                .roughness = global::materials[i].roughness,
                .ior = global::materials[i].ior,
                .transmission = global::materials[i].transmission
        };

        // Set up texture
        if (!global::materials[i].diffuseTexPath.empty()) {
            mat2.texIdx = global::textureIndex;
            mat2.diffuse.w = 1;

            auto texture = std::make_shared<vkCore::Texture>();
            texture->init(global::materials[i].diffuseTexPath);
            mTextures[global::textureIndex++] = texture;
        }

        memAlignedMaterials.push_back(mat2);
    }

    // upload active light textures
    for (auto& light: pActiveLights) {
        if (!light->texPath.empty()) {
            light->texID = global::textureIndex;

            auto texture = std::make_shared<vkCore::Texture>();
            texture->init(light->texPath);
            mTextures[global::textureIndex++] = texture;
        } else {
            light->texID = -1;
        }
    }

    // upload materials
    mMaterialBuffers.upload(memAlignedMaterials);

    for (size_t i = 0; i < mGeometries.size(); ++i) {
        if (i < mGeometries.size()) {
            if (mGeometries[i] != nullptr) {
                if (!mGeometries[i]->initialized) {
                    // Only keep one copy of both index and vertex buffers each.
                    mVertexBuffers[i].init(
                            mGeometries[i]->vertices, 2, true,
                            {vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR}
                    );
                    mIndexBuffers[i].init(
                            mGeometries[i]->indices, 2, true,
                            {vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR}

                    );
                    mMaterialIndexBuffers[i].init(mGeometries[i]->matIndex, 2, true);

                    mGeometries[i]->initialized = true;
//                        KF_SUCCESS( "Initialized Geometries." );
                }
            }
        }
    }

//        KF_SUCCESS( "Uploaded Geometries." );
}

void Scene::uploadGeometryInstances() {
    if (pConfig->mMaxGeometryInstancesChanged) {
        pConfig->mMaxGeometryInstancesChanged = false;

        std::vector<GeometryInstanceSSBO> geometryInstances(pConfig->mMaxGeometryInstances);
        mGeometryInstancesBuffer.init(geometryInstances, global::maxResources);

        updateSceneDescriptors();
    }

    mUploadGeometryInstancesToBuffer = false;

    memAlignedGeometryInstances.resize(mGeometryInstances.size());
    std::transform(mGeometryInstances.begin(), mGeometryInstances.end(), memAlignedGeometryInstances.begin(),
                   [](std::shared_ptr<GeometryInstance> instance) {
                       return GeometryInstanceSSBO{instance->transform,
                                                   instance->geometryIndex};
                   });

    mGeometryInstancesBuffer.upload(memAlignedGeometryInstances);

//        KF_SUCCESS( "Uploaded geometry instances." );
}

void Scene::translateDummy() {
    auto dummyInstance = getGeometryInstance(triangle->geometryIndex);
    auto camPos = mCurrentCamera->getPosition();
    dummyInstance->setTransform(glm::translate(glm::mat4(1.0F), glm::vec3(camPos.x, camPos.y, camPos.z + 2.0F)));
}

void Scene::addDummy() {
    mDummy = true;

    Vertex v1;
    v1.normal = glm::vec3(0.0F, 1.0F, 0.0F);
    v1.pos = glm::vec3(-0.00001F, 0.0F, 0.00001F);

    Vertex v2;
    v2.normal = glm::vec3(0.0F, 1.0F, 0.0F);
    v2.pos = glm::vec3(0.00001F, 0.0F, 0.00001F);

    Vertex v3;
    v3.normal = glm::vec3(0.0F, 1.0F, 0.0F);
    v3.pos = glm::vec3(0.00001F, 0.0F, -0.00001F);

    triangle = std::make_shared<Geometry>();
    triangle->vertices = {v1, v2, v3};
    triangle->indices = {0, 1, 2};
    triangle->geometryIndex = global::geometryIndex++;
    triangle->matIndex = {0}; // a single triangle for the material buffer
    triangle->path = "Custom Dummy Triangle";
    triangle->dynamic = true;

    NiceMaterial mat;
    triangle->setMaterial(mat);

    auto camPos = mCurrentCamera->getPosition();
    auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(camPos.x + 2.0F, camPos.y, camPos.z));
    triangleInstance = instance(triangle, transform);

    submitGeometry(triangle);
    submitGeometryInstance(triangleInstance);

//        KF_VERBOSE( "Scene is empty. Added dummy element." );
}

void Scene::removeDummy() {
    if (triangle != nullptr && mGeometryInstances.size() > 1) {
        mDummy = false;

//            KF_VERBOSE( "Removing dummy element." );
        removeGeometry(triangle);
        triangle = nullptr;
    }
}

void Scene::initSceneDescriptorSets() {
  mSceneDescriptors.bindings.reset();

    // Camera uniform buffer
  mSceneDescriptors.bindings.add(0, vk::DescriptorType::eUniformBuffer,
                                   vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
    // Scene description buffer
  mSceneDescriptors.bindings.add(1, vk::DescriptorType::eStorageBuffer,
                                   vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    // Environment map
  mSceneDescriptors.bindings.add(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eMissKHR);

    // Directional light uniform buffer
    mSceneDescriptors.bindings.add(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);

    // Point lights uniform buffer
    mSceneDescriptors.bindings.add(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);

    // Active lights uniform buffer
    mSceneDescriptors.bindings.add(5, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eClosestHitKHR);

    mSceneDescriptors.layout = mSceneDescriptors.bindings.initLayoutUnique();
  mSceneDescriptors.pool = mSceneDescriptors.bindings.initPoolUnique(global::maxResources);
  mSceneDescriptorSets = vkCore::allocateDescriptorSets(mSceneDescriptors.pool.get(), mSceneDescriptors.layout.get());
}

void Scene::initGeometryDescriptorSets() {
  mGeometryDescriptors.bindings.reset();

    // Vertex buffers
  mGeometryDescriptors.bindings.add(0,
                                      vk::DescriptorType::eStorageBuffer,
                                      vk::ShaderStageFlagBits::eClosestHitKHR,
                                      pConfig->mMaxGeometry,
                                      vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    // Index buffers
  mGeometryDescriptors.bindings.add(1,
                                      vk::DescriptorType::eStorageBuffer,
                                      vk::ShaderStageFlagBits::eClosestHitKHR,
                                      pConfig->mMaxGeometry,
                                      vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    // MatIndex buffers
  mGeometryDescriptors.bindings.add(2,
                                      vk::DescriptorType::eStorageBuffer,
                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                      pConfig->mMaxGeometry,
                                      vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    // Textures
    if (!mImmutableSampler) {
      mImmutableSampler = vkCore::initSamplerUnique(vkCore::getSamplerCreateInfo());
    }

    std::vector<vk::Sampler> immutableSamplers(pConfig->mMaxTextures);
    for (auto &immutableSampler : immutableSamplers) {
        immutableSampler = mImmutableSampler.get();
    }

    mGeometryDescriptors.bindings.add(3,
                                      vk::DescriptorType::eCombinedImageSampler,
                                      vk::ShaderStageFlagBits::eClosestHitKHR,
                                      pConfig->mMaxTextures,
                                      vk::DescriptorBindingFlagBits::eUpdateAfterBind,
                                      immutableSamplers.data());

    mGeometryDescriptors.bindings.add(4,
                                      vk::DescriptorType::eStorageBuffer,
                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                      1,
                                      vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    mGeometryDescriptors.layout = mGeometryDescriptors.bindings.initLayoutUnique(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
    mGeometryDescriptors.pool = mGeometryDescriptors.bindings.initPoolUnique(vkCore::global::swapchainImageCount,
                                                                             vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
    mGeometryDescriptorSets = vkCore::allocateDescriptorSets(mGeometryDescriptors.pool.get(),
                                                             mGeometryDescriptors.layout.get());
}

void Scene::updateSceneDescriptors() {
    // Environment map
    vk::DescriptorImageInfo environmentMapTextureInfo;
    if (mEnvironmentMap.getImageView() && mEnvironmentMap.getSampler()) {
        environmentMapTextureInfo.imageLayout = mEnvironmentMap.getLayout();
        environmentMapTextureInfo.imageView = mEnvironmentMap.getImageView();
        environmentMapTextureInfo.sampler = mEnvironmentMap.getSampler();
    } else {
        throw std::runtime_error("No default environment map provided.");
    }

    mSceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 0,
                                          mCameraUniformBuffer._bufferInfos.data());
    mSceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 1,
                                          mGeometryInstancesBuffer.getDescriptorInfos().data());
    mSceneDescriptors.bindings.write(mSceneDescriptorSets, 2, &environmentMapTextureInfo);
    mSceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 3,
                                          mDirectionalLightUniformBuffer._bufferInfos.data());
    mSceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 4,
                                          mPointLightsUniformBuffer._bufferInfos.data());
    mSceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 5,
                                          mActiveLightsUniformBuffer._bufferInfos.data());
    mSceneDescriptors.bindings.update();
}

void Scene::updateGeoemtryDescriptors() {
//        KF_ASSERT( mGeometries.size( ) <= pConfig->mMaxGeometry, "Can not bind more than ", pConfig->mMaxGeometry, " geometries." );
//        KF_ASSERT( mVertexBuffers.size( ) == pConfig->mMaxGeometry, "Vertex buffers container size and geometry limit must be identical." );
//        KF_ASSERT( mIndexBuffers.size( ) == pConfig->mMaxGeometry, "Index buffers container size and geometry limit must be identical." );
//        KF_ASSERT( mTextures.size( ) == pConfig->mMaxTextures, "Texture container size and texture limit must be identical." );

    // Vertex buffers infos
    std::vector<vk::DescriptorBufferInfo> vertexBufferInfos;
    vertexBufferInfos.reserve(mVertexBuffers.size());
    for (const auto &vertexBuffer : mVertexBuffers) {
        vk::DescriptorBufferInfo vertexDataBufferInfo(vertexBuffer.get().empty() ? nullptr : vertexBuffer.get(0),
                                                      0,
                                                      VK_WHOLE_SIZE);

        vertexBufferInfos.push_back(vertexDataBufferInfo);
    }

    // Index buffers infos
    std::vector<vk::DescriptorBufferInfo> indexBufferInfos;
    indexBufferInfos.reserve(mIndexBuffers.size());
    for (const auto &indexBuffer : mIndexBuffers) {
        vk::DescriptorBufferInfo indexDataBufferInfo(indexBuffer.get().empty() ? nullptr : indexBuffer.get(0),
                                                     0,
                                                     VK_WHOLE_SIZE);

        indexBufferInfos.push_back(indexDataBufferInfo);
    }

    // MatIndices infos
    std::vector<vk::DescriptorBufferInfo> matIndexBufferInfos;
    matIndexBufferInfos.reserve(mMaterialIndexBuffers.size());
    for (const auto &materialIndexBuffer : mMaterialIndexBuffers) {
        vk::DescriptorBufferInfo materialIndexDataBufferInfo(
                materialIndexBuffer.get().empty() ? nullptr : materialIndexBuffer.get(0),
                0,
                VK_WHOLE_SIZE);

        matIndexBufferInfos.push_back(materialIndexDataBufferInfo);
    }

    // Texture samplers
    std::vector<vk::DescriptorImageInfo> textureInfos;
    textureInfos.reserve(mTextures.size());
    for (size_t i = 0; i < pConfig->mMaxTextures; ++i) {
        vk::DescriptorImageInfo textureInfo = {};

        if (mTextures[i] != nullptr) {
            textureInfo.imageLayout = mTextures[i]->getLayout();
            textureInfo.imageView = mTextures[i]->getImageView();
            textureInfo.sampler = mImmutableSampler.get();
        } else {
            textureInfo.imageLayout = {};
            textureInfo.sampler = mImmutableSampler.get();
        }

        textureInfos.push_back(textureInfo);
    }

    // Write to and update descriptor bindings
    mGeometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 0, vertexBufferInfos.data());
    mGeometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 1, indexBufferInfos.data());
    mGeometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 2, matIndexBufferInfos.data()); // matIndices
    mGeometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 3, textureInfos.data());
    mGeometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 4,
        mMaterialBuffers.getDescriptorInfos().data()); // materials

    mGeometryDescriptors.bindings.update();
}

void Scene::upload(vk::Fence fence, uint32_t imageIndex) {
}
}
