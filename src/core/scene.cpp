//
// Created by jet on 4/9/21.
//

#include "core/scene.hpp"
#include "core/context/global.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

//#include <gltf/tiny_gltf.h>

namespace kuafu {
    CameraUBO cameraUBO;

    std::shared_ptr<Geometry> triangle = nullptr; ///< A dummy triangle that will be placed in the scene if it empty. This assures the AS creation.
    std::shared_ptr<GeometryInstance> triangleInstance = nullptr;

    std::vector<GeometryInstanceSSBO> memAlignedGeometryInstances;
    std::vector<MaterialSSBO> memAlignedMaterials;

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
            if (mGeometryInstances.size() > _settings->_maxGeometryInstances) {
                throw std::runtime_error(
                        "Failed to submit geometry instance because instance buffer size has been exceeded.");
                return;
            }
        }

        mGeometryInstances.push_back(geometryInstance);
        _uploadGeometryInstancesToBuffer = true;
    }

    void Scene::setGeometryInstances(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances) {
        mGeometryInstances.clear();
        mGeometryInstances.reserve(geometryInstances.size());

        for (auto geometryInstance : geometryInstances) {
            submitGeometryInstance(geometryInstance);
        }

        _uploadGeometryInstancesToBuffer = true;
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

        _uploadGeometryInstancesToBuffer = true;
    }

    void Scene::clearGeometryInstances() {
        // Only allow clearing the scene if there is no dummy element.
        if (!mDummy) {
            mGeometryInstances.clear();
            _uploadGeometryInstancesToBuffer = true;
        }
    }

    void Scene::popGeometryInstance() {
        // Only allow clearing the scene if the scene is not empty and does not contain a dummy element.
        if (!mGeometryInstances.empty() && !mDummy) {
            mGeometryInstances.erase(mGeometryInstances.end() - 1);
            _uploadGeometryInstancesToBuffer = true;
        }
    }

    void Scene::submitGeometry(std::shared_ptr<Geometry> geometry) {
        if (!mDummy) {
            if (mGeometries.size() >= _settings->_maxGeometry) {
                throw std::runtime_error(
                        "Failed to submit geometry because geometries buffer size has been exceeded.");
                return;
            }
        }

        mGeometries.push_back(geometry);
        _uploadGeometries = true;
    }

    void Scene::setGeometries(const std::vector<std::shared_ptr<Geometry>> &geometries) {
        mGeometries.clear();
        mGeometries.reserve(geometries.size());

        for (auto geometry : geometries) {
            submitGeometry(geometry);
        }

        _uploadGeometries = true;
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

        _uploadGeometryInstancesToBuffer = true;

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
        _uploadGeometries = true; // @todo Might not be necessary.

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

        _uploadGeometryInstancesToBuffer = true;
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

        _uploadGeometries = true;
        _uploadGeometryInstancesToBuffer = true;
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

//        KF_INFO( "Could not find geometry in scene." );
        return nullptr;
    }

    void Scene::setEnvironmentMap(std::string_view path) {
        _environmentMapTexturePath = path;
        _useEnvironmentMap = true;
        _uploadEnvironmentMap = true;
    }

    void Scene::removeEnvironmentMap() {
        _useEnvironmentMap = false;
    }

    void Scene::setCamera(std::shared_ptr<Camera> camera) {
        mCameras.insert(camera);
        mCurrentCamera = camera;
    }

    void Scene::setCamera(int width, int height, const glm::vec3 &position) {
        auto cam = std::make_shared<Camera>(width, height, position);
        mCameras.insert(cam);
        mCurrentCamera = cam;
    }

    void Scene::prepareBuffers() {
        // Resize and initialize buffers with "dummy data".
        // The advantage of doing this is that the buffers are all initialized right away (even though it is invalid data) and
        // this makes it possible to call fill instead of initialize again, when changing any of the data below.
        std::vector<GeometryInstanceSSBO> geometryInstances(_settings->_maxGeometryInstances);
        _geometryInstancesBuffer.init(geometryInstances, global::maxResources);

        // @todo is this even necessary?
        std::vector<MaterialSSBO> materials(_settings->_maxMaterials);
        _materialBuffers.init(materials, global::maxResources);

        mVertexBuffers.resize(_settings->_maxGeometry);
        mIndexBuffers.resize(_settings->_maxGeometry);
        _materialIndexBuffers.resize(_settings->_maxGeometry);
        _textures.resize(_settings->_maxTextures);

        initCameraBuffer();
    }

    void Scene::initCameraBuffer() {
        _cameraUniformBuffer.init();
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
            cameraUBO.front = glm::vec4(mCurrentCamera->getFront(), mCurrentCamera->getFocalDistance());
        }

        _cameraUniformBuffer.upload(imageIndex, cameraUBO);
    }

    void Scene::uploadEnvironmentMap() {
        _uploadEnvironmentMap = false;

        _environmentMap.init(_environmentMapTexturePath);

        if (_removeEnvironmentMap) {
            removeEnvironmentMap();
            _removeEnvironmentMap = false;
        }
    }

    void Scene::uploadGeometries() {
        _uploadGeometries = false;

        memAlignedMaterials.clear();
        memAlignedMaterials.reserve(global::materials.size());

        // Create all textures of a material
        for (size_t i = 0; i < global::materials.size(); ++i) {
            // Convert to memory aligned struct
            MaterialSSBO mat2;
            mat2.diffuse = glm::vec4(global::materials[i].kd, -1.0F);
            mat2.emission = glm::vec4(global::materials[i].emission, global::materials[i].ns);
            mat2.dissolve = global::materials[i].d;
            mat2.ior = global::materials[i].ni;
            mat2.illum = global::materials[i].illum;
            mat2.fuzziness = global::materials[i].fuzziness;

            // Set up texture
            if (!global::materials[i].diffuseTexPath.empty()) {
                mat2.diffuse.w = static_cast<float>( global::textureIndex );

                auto texture = std::make_shared<vkCore::Texture>();
                texture->init(global::assetsPath + global::materials[i].diffuseTexPath);
                _textures[global::textureIndex++] = texture;
            }

            memAlignedMaterials.push_back(mat2);
        }

        // upload materials
        _materialBuffers.upload(memAlignedMaterials);

        for (size_t i = 0; i < mGeometries.size(); ++i) {
            if (i < mGeometries.size()) {
                if (mGeometries[i] != nullptr) {
                    if (!mGeometries[i]->initialized) {
                        // Only keep one copy of both index and vertex buffers each.
                        mVertexBuffers[i].init(mGeometries[i]->vertices, 2, true);
                        mIndexBuffers[i].init(mGeometries[i]->indices, 2, true);
                        _materialIndexBuffers[i].init(mGeometries[i]->matIndex, 2, true);

                        mGeometries[i]->initialized = true;
//                        KF_SUCCESS( "Initialized Geometries." );
                    }
                }
            }
        }

//        KF_SUCCESS( "Uploaded Geometries." );
    }

    void Scene::uploadGeometryInstances() {
        if (_settings->_maxGeometryInstancesChanged) {
            _settings->_maxGeometryInstancesChanged = false;

            std::vector<GeometryInstanceSSBO> geometryInstances(_settings->_maxGeometryInstances);
            _geometryInstancesBuffer.init(geometryInstances, global::maxResources);

            updateSceneDescriptors();
        }

        _uploadGeometryInstancesToBuffer = false;

        memAlignedGeometryInstances.resize(mGeometryInstances.size());
        std::transform(mGeometryInstances.begin(), mGeometryInstances.end(), memAlignedGeometryInstances.begin(),
                       [](std::shared_ptr<GeometryInstance> instance) {
                           return GeometryInstanceSSBO{instance->transform,
                                                       instance->geometryIndex};
                       });

        _geometryInstancesBuffer.upload(memAlignedGeometryInstances);

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

        Material mat;
        triangle->setMaterial(mat);

        auto camPos = mCurrentCamera->getPosition();
        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(camPos.x, camPos.y, camPos.z + 2.0F));
        triangleInstance = instance(triangle);

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
        _sceneDescriptors.bindings.reset();

        // Camera uniform buffer
        _sceneDescriptors.bindings.add(0, vk::DescriptorType::eUniformBuffer,
                                       vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
        // Scene description buffer
        _sceneDescriptors.bindings.add(1, vk::DescriptorType::eStorageBuffer,
                                       vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
        // Environment map
        _sceneDescriptors.bindings.add(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eMissKHR);

        _sceneDescriptors.layout = _sceneDescriptors.bindings.initLayoutUnique();
        _sceneDescriptors.pool = _sceneDescriptors.bindings.initPoolUnique(global::maxResources);
        mSceneDescriptorSets = vkCore::allocateDescriptorSets(_sceneDescriptors.pool.get(),
                                                              _sceneDescriptors.layout.get());
    }

    void Scene::initGeoemtryDescriptorSets() {
        _geometryDescriptors.bindings.reset();

        // Vertex buffers
        _geometryDescriptors.bindings.add(0,
                                          vk::DescriptorType::eStorageBuffer,
                                          vk::ShaderStageFlagBits::eClosestHitKHR,
                                          _settings->_maxGeometry,
                                          vk::DescriptorBindingFlagBits::eUpdateAfterBind);

        // Index buffers
        _geometryDescriptors.bindings.add(1,
                                          vk::DescriptorType::eStorageBuffer,
                                          vk::ShaderStageFlagBits::eClosestHitKHR,
                                          _settings->_maxGeometry,
                                          vk::DescriptorBindingFlagBits::eUpdateAfterBind);

        // MatIndex buffers
        _geometryDescriptors.bindings.add(2,
                                          vk::DescriptorType::eStorageBuffer,
                                          vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                          _settings->_maxGeometry,
                                          vk::DescriptorBindingFlagBits::eUpdateAfterBind);

        // Textures
        if (!_immutableSampler) {
            _immutableSampler = vkCore::initSamplerUnique(vkCore::getSamplerCreateInfo());
        }

        std::vector<vk::Sampler> immutableSamplers(_settings->_maxTextures);
        for (auto &immutableSampler : immutableSamplers) {
            immutableSampler = _immutableSampler.get();
        }

        _geometryDescriptors.bindings.add(3,
                                          vk::DescriptorType::eCombinedImageSampler,
                                          vk::ShaderStageFlagBits::eClosestHitKHR,
                                          _settings->_maxTextures,
                                          vk::DescriptorBindingFlagBits::eUpdateAfterBind,
                                          immutableSamplers.data());

        _geometryDescriptors.bindings.add(4,
                                          vk::DescriptorType::eStorageBuffer,
                                          vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                          1,
                                          vk::DescriptorBindingFlagBits::eUpdateAfterBind);

        _geometryDescriptors.layout = _geometryDescriptors.bindings.initLayoutUnique(
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
        _geometryDescriptors.pool = _geometryDescriptors.bindings.initPoolUnique(vkCore::global::swapchainImageCount,
                                                                                 vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
        mGeometryDescriptorSets = vkCore::allocateDescriptorSets(_geometryDescriptors.pool.get(),
                                                                 _geometryDescriptors.layout.get());
    }

    void Scene::updateSceneDescriptors() {
        // Environment map
        vk::DescriptorImageInfo environmentMapTextureInfo;
        if (_environmentMap.getImageView() && _environmentMap.getSampler()) {
            environmentMapTextureInfo.imageLayout = _environmentMap.getLayout();
            environmentMapTextureInfo.imageView = _environmentMap.getImageView();
            environmentMapTextureInfo.sampler = _environmentMap.getSampler();
        } else {
            throw std::runtime_error("No default environment map provided.");
        }

        _sceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 0, _cameraUniformBuffer._bufferInfos.data());
        _sceneDescriptors.bindings.writeArray(mSceneDescriptorSets, 1,
                                              _geometryInstancesBuffer.getDescriptorInfos().data());
        _sceneDescriptors.bindings.write(mSceneDescriptorSets, 2, &environmentMapTextureInfo);
        _sceneDescriptors.bindings.update();
    }

    void Scene::updateGeoemtryDescriptors() {
//        KF_ASSERT( mGeometries.size( ) <= _settings->_maxGeometry, "Can not bind more than ", _settings->_maxGeometry, " geometries." );
//        KF_ASSERT( mVertexBuffers.size( ) == _settings->_maxGeometry, "Vertex buffers container size and geometry limit must be identical." );
//        KF_ASSERT( mIndexBuffers.size( ) == _settings->_maxGeometry, "Index buffers container size and geometry limit must be identical." );
//        KF_ASSERT( _textures.size( ) == _settings->_maxTextures, "Texture container size and texture limit must be identical." );

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
        matIndexBufferInfos.reserve(_materialIndexBuffers.size());
        for (const auto &materialIndexBuffer : _materialIndexBuffers) {
            vk::DescriptorBufferInfo materialIndexDataBufferInfo(
                    materialIndexBuffer.get().empty() ? nullptr : materialIndexBuffer.get(0),
                    0,
                    VK_WHOLE_SIZE);

            matIndexBufferInfos.push_back(materialIndexDataBufferInfo);
        }

        // Texture samplers
        std::vector<vk::DescriptorImageInfo> textureInfos;
        textureInfos.reserve(_textures.size());
        for (size_t i = 0; i < _settings->_maxTextures; ++i) {
            vk::DescriptorImageInfo textureInfo = {};

            if (_textures[i] != nullptr) {
                textureInfo.imageLayout = _textures[i]->getLayout();
                textureInfo.imageView = _textures[i]->getImageView();
                textureInfo.sampler = _immutableSampler.get();
            } else {
                textureInfo.imageLayout = {};
                textureInfo.sampler = _immutableSampler.get();
            }

            textureInfos.push_back(textureInfo);
        }

        // Write to and update descriptor bindings
        _geometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 0, vertexBufferInfos.data());
        _geometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 1, indexBufferInfos.data());
        _geometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 2, matIndexBufferInfos.data()); // matIndices
        _geometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 3, textureInfos.data());
        _geometryDescriptors.bindings.writeArray(mGeometryDescriptorSets, 4,
                                                 _materialBuffers.getDescriptorInfos().data()); // materials

        _geometryDescriptors.bindings.update();
    }

    void Scene::upload(vk::Fence fence, uint32_t imageIndex) {
    }
}
