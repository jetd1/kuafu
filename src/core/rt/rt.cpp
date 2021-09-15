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
#include "core/rt/rt.hpp"
#include "core/context/global.hpp"
#include "core/config.hpp"

namespace kuafu {
RayTracer::~RayTracer() {
    destroy();
}

void RayTracer::init() {
    auto pipelineProperties = vkCore::global::physicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    mCapabilities.pipelineProperties = pipelineProperties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    mCapabilities.accelerationStructureProperties = pipelineProperties.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
}

void RayTracer::destroy() {
    vkCore::global::device.waitIdle();

    for (Blas &blas : mBlas)
        blas.as.destroy();
    mTlas.as.destroy();
    mBlas.clear();
}


auto RayTracer::createDummyBlas() const -> Blas {

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
            Vertex::getVertexPositionFormat(),
            nullptr,
            sizeof(Vertex),
            0,
            vk::IndexType::eUint32,
            nullptr,
            {});

    vk::AccelerationStructureGeometryKHR asGeom(
            vk::GeometryTypeKHR::eTriangles,
            trianglesData,
            vk::GeometryFlagBitsKHR::eOpaque);

    vk::AccelerationStructureBuildRangeInfoKHR offset(0,
                                                      0,
                                                      0,
                                                      0);

    Blas blas;
    blas.asGeometry.push_back(asGeom);
    blas.asBuildRangeInfo.push_back(offset);

    return blas;
}


auto RayTracer::modelToBlas(const vkCore::StorageBuffer<Vertex> &vertexBuffer,
                            const vkCore::StorageBuffer<uint32_t> &indexBuffer, bool opaque) const -> Blas {
    // Using index 0, because there are no copies of these buffers.
    vk::BufferDeviceAddressInfo vertexAddressInfo(vertexBuffer.get(0));
    vk::BufferDeviceAddressInfo indexAddressInfo(indexBuffer.get(0));

    vk::DeviceAddress vertexAddress = vkCore::global::device.getBufferAddress(vertexAddressInfo);
    vk::DeviceAddress indexAddress = vkCore::global::device.getBufferAddress(indexAddressInfo);

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData(
            Vertex::getVertexPositionFormat(),
            vertexAddress,
            sizeof(Vertex),
            vertexBuffer.getCount(),
            vk::IndexType::eUint32,
            indexAddress,
            {});

    vk::AccelerationStructureGeometryKHR asGeom(
            vk::GeometryTypeKHR::eTriangles,
            trianglesData,
            opaque ? vk::GeometryFlagBitsKHR::eOpaque
                   : vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation);

    vk::AccelerationStructureBuildRangeInfoKHR offset(indexBuffer.getCount() / 3,
                                                      0,
                                                      0,
                                                      0);

    Blas blas;
    blas.asGeometry.push_back(asGeom);
    blas.asBuildRangeInfo.push_back(offset);

    return blas;
}

auto RayTracer::geometryInstanceToAccelerationStructureInstance(
        std::shared_ptr<GeometryInstance> &geometryInstance) -> vk::AccelerationStructureInstanceKHR {
    KF_ASSERT(mBlas.size() > geometryInstance->geometryIndex,
              "Failed to transform geometry instance to a VkGeometryInstanceKHR because index is out of bounds.");
    Blas &blas{mBlas[geometryInstance->geometryIndex]};

    vk::AccelerationStructureDeviceAddressInfoKHR addressInfo(blas.as.as);
    vk::DeviceAddress blasAddress = vkCore::global::device.getAccelerationStructureAddressKHR(addressInfo);

    glm::mat4 transpose = glm::transpose(geometryInstance->transform);

    vk::AccelerationStructureInstanceKHR gInst(
            {},                                                         // transform
            geometryInstance->geometryIndex,                             // instanceCustomIndex
            0xFF,                                                        // mask
            0,                                                           // instanceShaderBindingTableRecordOffset
            vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable, // flags
            blasAddress);                                               // accelerationStructureReference

    memcpy(reinterpret_cast<glm::mat4 *>(&gInst.transform), &transpose, sizeof(gInst.transform));

    return gInst;
}

void RayTracer::createBottomLevelAS(std::vector<vkCore::StorageBuffer<Vertex>> &vertexBuffers,
                                    const std::vector<vkCore::StorageBuffer<uint32_t>> &indexBuffers,
                                    const std::vector<std::shared_ptr<Geometry>> &geometries) {
//        KF_ASSERT(!vertexBuffers.empty(), "Failed to build bottom level acceleration structures because no geometry was provided.");

    // Clean up previous acceleration structures and free all memory.
    destroy();

    mBlas.reserve(vertexBuffers.size());

    for (size_t i = 0; i < vertexBuffers.size(); ++i)
        if (geometries.size() > i)
            if (geometries[i])
                mBlas.push_back(geometries[i]->hideRender ? createDummyBlas()
                                : modelToBlas(vertexBuffers[i], indexBuffers[i], geometries[i]->isOpaque));

    buildBlas(vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction |
              vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
}

void RayTracer::buildBlas(vk::BuildAccelerationStructureFlagsKHR flags) {
    KF_DEBUG("Rebuilding BLAS... This is very heavy!");

    uint32_t blasCount = static_cast<uint32_t>(mBlas.size());

    bool doCompaction = (flags & vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction) ==
                        vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;

    vk::DeviceSize maxScratch = 0; // Largest scratch buffer for our BLAS

    std::vector<vk::DeviceSize> originalSizes;
    originalSizes.resize(mBlas.size());

    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildInfos;
    buildInfos.reserve(mBlas.size());

    // Iterate over the groups of geometries, creating one BLAS for each group
    int index = 0;
    for (Blas &blas : mBlas) {
        if (!blas.as.as) {
            vkCore::global::device.destroyAccelerationStructureKHR(blas.as.as);
        }

        if (!blas.as.memory) {
            vkCore::global::device.freeMemory(blas.as.memory);
        }

        if (!blas.as.buffer) {
            vkCore::global::device.destroyBuffer(blas.as.buffer);
        }

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
                vk::AccelerationStructureTypeKHR::eBottomLevel,   // type
                flags,                                            // flags
                vk::BuildAccelerationStructureModeKHR::eBuild,    // mode
                nullptr,                                          // srcAccelerationStructure
                {},                                              // dstAccelerationStructure
                static_cast<uint32_t>(blas.asGeometry.size()), // geometryCount
                blas.asGeometry.data(),                          // pGeometries
                {},                                              // ppGeometries
                {});                                            // scratchData

        std::vector<uint32_t> maxPrimitiveCount(blas.asBuildRangeInfo.size());

        for (size_t i = 0; i < blas.asBuildRangeInfo.size(); ++i) {
            maxPrimitiveCount[i] = blas.asBuildRangeInfo[i].primitiveCount;
        }

        vk::AccelerationStructureBuildSizesInfoKHR sizeInfo;
        vkCore::global::device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                                     &buildInfo, maxPrimitiveCount.data(),
                                                                     &sizeInfo);

        // Create acceleration structure
        // @todo Potentially, pass size and type to initAccelerationStructure.
        vk::AccelerationStructureCreateInfoKHR createInfo(
                {},                                            // createFlags
                {},                                            // buffer
                {},                                            // offset
                sizeInfo.accelerationStructureSize,             // size
                vk::AccelerationStructureTypeKHR::eBottomLevel, // type
                {});                                          // deviceAddress

        blas.as = initAccelerationStructure(createInfo);
        //blas.flags = flags;

        buildInfo.dstAccelerationStructure = blas.as.as;

        maxScratch = std::max(maxScratch, sizeInfo.buildScratchSize);
        originalSizes[index] = sizeInfo.accelerationStructureSize;

        buildInfos.push_back(buildInfo);

        ++index;
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder.
    vk::MemoryAllocateFlagsInfo allocateFlags(vk::MemoryAllocateFlagBitsKHR::eDeviceAddress);

    vkCore::Buffer scratchBuffer(
            maxScratch,                                                                              // size
            vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, // usage
            {vkCore::global::graphicsFamilyIndex},                                                 // queueFamilyIndices
            vk::MemoryPropertyFlagBits::eDeviceLocal,                                                // memoryPropertyFlags
            &allocateFlags);

    vk::BufferDeviceAddressInfo bufferInfo(scratchBuffer.get());
    vk::DeviceAddress scratchAddress = vkCore::global::device.getBufferAddress(&bufferInfo);

    // Query size of compact BLAS.
    vk::UniqueQueryPool queryPool = vkCore::initQueryPoolUnique(blasCount,
                                                                vk::QueryType::eAccelerationStructureCompactedSizeKHR);

    // Create a command buffer containing all the BLAS builds.
    vk::UniqueCommandPool commandPool = vkCore::initCommandPoolUnique({vkCore::global::graphicsFamilyIndex});
    int ctr = 0;

    vkCore::CommandBuffer cmdBuf(commandPool.get(), blasCount);

    index = 0;
    for (Blas &blas : mBlas) {
        buildInfos[index].scratchData.deviceAddress = scratchAddress;

        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR *> pBuildRangeInfos(
                blas.asBuildRangeInfo.size());

        size_t infoIndex = 0;
        for (auto &pbuildRangeInfo : pBuildRangeInfos) {
            pbuildRangeInfo = &blas.asBuildRangeInfo[infoIndex];
            ++infoIndex;
        }

        cmdBuf.begin(index);

        // Building the acceleration structure
        cmdBuf.get(index).buildAccelerationStructuresKHR(1, &buildInfos[index], pBuildRangeInfos.data());

        // Make sure the BLAS were successfully built first before reusing the scratch buffer.
        vk::MemoryBarrier barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,  // srcAccessMask
                                  vk::AccessFlagBits::eAccelerationStructureReadKHR); // dstAccessMask

        cmdBuf.get(index).pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // srcStageMask
                                          vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // dstStageMask
                                          {},                                                       // dependencyFlags
                                          1,                                                         // memoryBarrierCount
                                          &barrier,                                                  // pMemoryBarriers
                                          0,                                                         // bufferMemoryBarrierCount
                                          nullptr,                                                   // pBufferMemoryBarriers
                                          0,                                                         // imageMemoryBarrierCount
                                          nullptr);                                                 // pImageMemoryBarriers

        if (doCompaction) {
            // After query pool creation, each query must be reset before it is used. Queries must also be reset between uses.
            // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdResetQueryPool.html
            cmdBuf.get(index).resetQueryPool(queryPool.get(), index, 1);

            cmdBuf.get(index).writeAccelerationStructuresPropertiesKHR(
                    1,                                                     // accelerationStructureCount
                    &blas.as.as,                                           // pAccelerationStructures
                    vk::QueryType::eAccelerationStructureCompactedSizeKHR, // queryType
                    queryPool.get(),                                      // queryPool
                    ctr++);                                               // firstQuery
        }

        cmdBuf.end(index);

        ++index;
    }

    cmdBuf.submitToQueue(vkCore::global::graphicsQueue);

    if (doCompaction) {
        vkCore::CommandBuffer compactionCmdBuf(vkCore::global::graphicsCmdPool);

        std::vector<vk::DeviceSize> compactSizes(mBlas.size());

        auto result = vkCore::global::device.getQueryPoolResults(
                queryPool.get(),                                // queryPool
                0,                                               // firstQuery
                static_cast<uint32_t>(compactSizes.size()),   // queryCount
                compactSizes.size() * sizeof(vk::DeviceSize), // dataSize
                compactSizes.data(),                            // pData
                sizeof(vk::DeviceSize),                        // stride
                vk::QueryResultFlagBits::eWait);                // flags

        KF_ASSERT(result == vk::Result::eSuccess, "Failed to get query pool results.");

        std::vector<AccelerationStructure> cleanupAS(mBlas.size());

        uint32_t totalOriginalSize = 0;
        uint32_t totalCompactSize = 0;

        compactionCmdBuf.begin(0);

        for (size_t i = 0; i < mBlas.size(); ++i) {
            totalOriginalSize += static_cast<uint32_t>(originalSizes[i]);
            totalCompactSize += static_cast<uint32_t>(compactSizes[i]);

            // Creating a compact version of the AS.
            vk::AccelerationStructureCreateInfoKHR asCreateInfo(
                    {},                                            // createFlags
                    {},                                            // buffer
                    {},                                            // offset
                    compactSizes[i],                                // size
                    vk::AccelerationStructureTypeKHR::eBottomLevel, // type
                    {});                                          // deviceAddress

            auto as = initAccelerationStructure(asCreateInfo);

            // Copy the original BLAS to a compact version
            vk::CopyAccelerationStructureInfoKHR copyInfo(mBlas[i].as.as,                                  // src
                                                          as.as,                                            // dst
                                                          vk::CopyAccelerationStructureModeKHR::eCompact); // mode

            compactionCmdBuf.get(0).copyAccelerationStructureKHR(&copyInfo);

            cleanupAS[i] = mBlas[i].as;
            mBlas[i].as = as;
        }

        compactionCmdBuf.end(0);
        compactionCmdBuf.submitToQueue(vkCore::global::graphicsQueue);

        for (auto &as : cleanupAS) {
            as.destroy();
        }

        KF_DEBUG("BLAS: Compaction Results: {} -> {} | Total: {}",
                 totalOriginalSize, totalCompactSize, totalOriginalSize - totalCompactSize);
    }
}

void RayTracer::updateTlas(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances,
                           vk::BuildAccelerationStructureFlagsKHR flags) {
    buildTlas(geometryInstances, flags, true);
}

void RayTracer::buildTlas(const std::vector<std::shared_ptr<GeometryInstance>> &geometryInstances,
                          vk::BuildAccelerationStructureFlagsKHR flags, bool reuse) {
    //mTlas.flags = flags;

    std::vector<vk::AccelerationStructureInstanceKHR> tlasInstances;
    tlasInstances.reserve(geometryInstances.size());

    for (auto instance : geometryInstances) {
        tlasInstances.push_back(geometryInstanceToAccelerationStructureInstance(instance));
    }

    if (reuse) {
        // destroy geometry instances buffer (probably not necessary in this case because I am using a unique handle)
    }

    vk::MemoryAllocateFlagsInfo allocateFlags(vk::MemoryAllocateFlagBitsKHR::eDeviceAddress);

    _instanceBuffer.init(sizeof(vk::AccelerationStructureInstanceKHR) * geometryInstances.size(),
                         vk::BufferUsageFlagBits::eShaderDeviceAddress |
                         vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                         {vkCore::global::graphicsFamilyIndex},
                         vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,
                         &allocateFlags);

    _instanceBuffer.fill<vk::AccelerationStructureInstanceKHR>(tlasInstances);

    vk::BufferDeviceAddressInfo bufferInfo(_instanceBuffer.get());
    vk::DeviceAddress instanceAddress = vkCore::global::device.getBufferAddress(&bufferInfo);

    vk::UniqueCommandPool commandPool = vkCore::initCommandPoolUnique(vkCore::global::graphicsFamilyIndex);
    vkCore::CommandBuffer cmdBuf(commandPool.get());

    cmdBuf.begin(0);

    vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite,                   // srcAccessMask
                              vk::AccessFlagBits::eAccelerationStructureWriteKHR); // dstAccessMask

    cmdBuf.get(0).pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,                      // srcStageMask
                                  vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, // dstStageMask
                                  {},                                                       // dependencyFlags
                                  1,                                                         // memoryBarrierCount
                                  &barrier,                                                  // pMemoryBarriers
                                  0,                                                         // bufferMemoryBarrierCount
                                  nullptr,                                                   // pBufferMemoryBarriers
                                  0,                                                         // imageMemoryBarrierCount
                                  nullptr);                                                 // pImageMemoryBarriers

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData(VK_FALSE,          // arrayOfPointers
                                                                    instanceAddress); // data

    vk::AccelerationStructureGeometryKHR tlasGeometry(vk::GeometryTypeKHR::eInstances, // geometryType
                                                      instancesData,                   // geoemtry
                                                      {});                           // flags

    vk::BuildAccelerationStructureModeKHR mode = reuse ? vk::BuildAccelerationStructureModeKHR::eUpdate
                                                       : vk::BuildAccelerationStructureModeKHR::eBuild;

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(vk::AccelerationStructureTypeKHR::eTopLevel, // type
                                                            flags,                                       // flags
                                                            mode,                                        // mode
                                                            nullptr,                                     // srcAccelerationStructure
                                                            {},                                         // dstAccelerationStructure
                                                            1,                                           // geometryCount
                                                            &tlasGeometry,                               // pGeometries
                                                            {},                                         // ppGeometries
                                                            {});                                       // scratchData

    auto instancesCount = static_cast<uint32_t>(geometryInstances.size());

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo({},   // accelerationStructureSize
                                                              {},   // updateScratchSize
                                                              {}); // buildScratchSize

    vkCore::global::device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                                 &buildInfo, &instancesCount, &buildSizesInfo);

    if (!reuse) {
        vk::AccelerationStructureCreateInfoKHR asCreateInfo(
                {},                                         // createFlags
                {},                                         // buffer
                {},                                         // offset
                buildSizesInfo.accelerationStructureSize,    // size
                vk::AccelerationStructureTypeKHR::eTopLevel, // type
                {});                                       // deviceAddress

        mTlas.as = initAccelerationStructure(asCreateInfo);
    }

    vk::MemoryAllocateFlagsInfo allocateInfo(vk::MemoryAllocateFlagBitsKHR::eDeviceAddress);

    vkCore::Buffer scratchBuffer(
            buildSizesInfo.buildScratchSize,                                                                           // size
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::eStorageBuffer, // usage
            {vkCore::global::graphicsFamilyIndex},                                                                   // queueFamilyIndices
            vk::MemoryPropertyFlagBits::eDeviceLocal |
            vk::MemoryPropertyFlagBits::eHostCoherent,                      // memoryPropertyFlags
            &allocateInfo);

    vk::BufferDeviceAddressInfo scratchBufferInfo(scratchBuffer.get());
    vk::DeviceAddress scratchAddress = vkCore::global::device.getBufferAddress(&scratchBufferInfo);

    buildInfo.srcAccelerationStructure = reuse ? mTlas.as.as : nullptr;
    buildInfo.dstAccelerationStructure = mTlas.as.as;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo(instancesCount, // primitiveCount
                                                              0,              // primitiveOffset
                                                              0,              // firstVertex
                                                              0);            // transformOffset

    const vk::AccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &buildRangeInfo;

    cmdBuf.get(0).buildAccelerationStructuresKHR(1, &buildInfo, &pBuildRangeInfo);

    cmdBuf.end(0);
    cmdBuf.submitToQueue(vkCore::global::graphicsQueue);
}

void RayTracer::createStorageImage(vk::Extent2D extent) {
    auto storageImageInfo = vkCore::getImageCreateInfo(
            vk::Extent3D(extent.width, extent.height, 1));
    storageImageInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
                             vk::ImageUsageFlagBits::eColorAttachment;
    storageImageInfo.format = vk::Format::eB8G8R8A8Unorm; // TODO: make this the surface format, and not hard-coded

    mStorageImage.init(storageImageInfo);
    mStorageImage.transitionToLayout(vk::ImageLayout::eGeneral);

    mStorageImageView = vkCore::initImageViewUnique(
            vkCore::getImageViewCreateInfo(mStorageImage.get(), mStorageImage.getFormat()));

    auto samplerCreateInfo = vkCore::getSamplerCreateInfo();
    mStorageImageSampler = vkCore::initSamplerUnique(samplerCreateInfo);

    mStorageImageInfo.sampler = mStorageImageSampler.get();
    mStorageImageInfo.imageView = mStorageImageView.get();
    mStorageImageInfo.imageLayout = mStorageImage.getLayout();
}

void RayTracer::createShaderBindingTable() {
    uint32_t groupHandleSize = mCapabilities.pipelineProperties.shaderGroupHandleSize;
    uint32_t baseAlignment = mCapabilities.pipelineProperties.shaderGroupBaseAlignment;

    uint32_t sbtSize = _shaderGroups * baseAlignment;

    vk::MemoryAllocateFlagsInfo allocateFlags(vk::MemoryAllocateFlagBitsKHR::eDeviceAddress);

    _sbtBuffer.init(sbtSize,
                    vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                    vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                    {vkCore::global::graphicsFamilyIndex},
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    &allocateFlags);

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    auto result = vkCore::global::device.getRayTracingShaderGroupHandlesKHR(_pipeline.get(),
                                                                            0,
                                                                            _shaderGroups,
                                                                            sbtSize,
                                                                            shaderHandleStorage.data());

    KF_ASSERT(result == vk::Result::eSuccess, "Failed to get ray tracing shader group handles.");

    void *mapped = NULL;
    result = vkCore::global::device.mapMemory(_sbtBuffer.getMemory(), 0, _sbtBuffer.getSize(), {}, &mapped);

//        KF_ASSERT(result == vk::Result::eSuccess, "Failed to map memory for shader binding table.");

    auto *pData = reinterpret_cast<uint8_t *>(mapped);
    for (uint32_t i = 0; i < _shaderGroups; ++i) {
        memcpy(pData, shaderHandleStorage.data() + i * groupHandleSize, groupHandleSize); // raygen
        pData += baseAlignment;
    }

    vkCore::global::device.unmapMemory(_sbtBuffer.getMemory());
}

void RayTracer::createPipeline(const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts) {
    //uint32_t anticipatedDirectionalLights = settings->maxDirectionalLights.has_value() ? settings->maxDirectionalLights.value() : global::maxDirectionalLights;
    //uint32_t anticipatedPointLights       = settings->maxPointLights.has_value() ? settings->maxPointLights.value() : global::maxPointLights;
    //Util::processShaderMacros("shaders/PathTrace.rchit", anticipatedDirectionalLights, anticipatedPointLights, global::modelCount);

    auto rgen = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PathTrace.rgen", KF_GLSLC_PATH);
    auto miss = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PathTrace.rmiss", KF_GLSLC_PATH);
    auto chit = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PathTrace.rchit", KF_GLSLC_PATH);
    auto ahit = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PathTrace.rahit", KF_GLSLC_PATH);
    //auto ahit1 = vk::Initializer::initShaderModuleUnique("shaders/PathTrace1.rahit");
    auto missShadow = vkCore::initShaderModuleUnique(global::assetsPath + "shaders/PathTraceShadow.rmiss",
                                                     KF_GLSLC_PATH);

    vk::PushConstantRange ptPushConstant(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
                                         vk::ShaderStageFlagBits::eClosestHitKHR, // stageFlags
                                         0,                                                                                                                 // offset
                                         sizeof(RtPushConstants));                                                                                       // size

    std::array<vk::PushConstantRange, 1> pushConstantRanges = {ptPushConstant};

    vk::PipelineLayoutCreateInfo layoutInfo({},                                                   // flags
                                            static_cast<uint32_t>(descriptorSetLayouts.size()), // setLayoutCount
                                            descriptorSetLayouts.data(),                          // pSetLayouts
                                            static_cast<uint32_t>(pushConstantRanges.size()),   // pushConstantRangeCount
                                            pushConstantRanges.data());                          // pPushConstantRanges

    _layout = vkCore::global::device.createPipelineLayoutUnique(layoutInfo);
    KF_ASSERT(_layout.get(), "Failed to create pipeline layout for path tracing pipeline.");

    std::array<vk::PipelineShaderStageCreateInfo, 5> shaderStages;
    shaderStages[0] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eRaygenKHR, rgen.get());
    shaderStages[1] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eMissKHR, miss.get());
    shaderStages[2] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eMissKHR, missShadow.get());
    shaderStages[3] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eClosestHitKHR, chit.get());
    shaderStages[4] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eAnyHitKHR, ahit.get());
    //shaderStages[4] = vkCore::getPipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eAnyHitKHR, ahit1.get());

    // Set up path tracing shader groups.
    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 4> groups;

    for (auto &group : groups) {
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;
    }

    groups[0].generalShader = 0;
    groups[0].type = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[1].generalShader = 1;
    groups[1].type = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[2].generalShader = 2;
    groups[2].type = vk::RayTracingShaderGroupTypeKHR::eGeneral;

    groups[3].closestHitShader = 3;
    groups[3].anyHitShader = 4;
    groups[3].type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;

    //groups[3].closestHitShader = 4;
    //groups[3].anyHitShader = 3;
    //groups[3].type         = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;

    _shaderGroups = static_cast<uint32_t>(groups.size());

    // @todo change hard-coded recursion depth
    vk::RayTracingPipelineCreateInfoKHR createInfo({},                                           // flags
                                                   static_cast<uint32_t>(shaderStages.size()), // stageCount
                                                   shaderStages.data(),                          // pStages
                                                   static_cast<uint32_t>(groups.size()),       // groupCount
                                                   groups.data(),                                // pGroups
                                                   30,                                            // maxPipelineRayRecursionDepth
                                                   {},                                           // pLibraryInfo
                                                   nullptr,                                       // pLibraryInterface
                                                   {},                                           // pDynamicState
                                                   _layout.get(),                                // layout
                                                   {},                                           // basePipelineHandle
                                                   0);                                           // basePipelineIndex

    _pipeline = static_cast<vk::UniquePipeline>(
            vkCore::global::device.createRayTracingPipelineKHRUnique({}, nullptr, createInfo).value);
//        KF_ASSERT(mPipeline.get(), "Failed to create path tracing pipeline.");
}

void RayTracer::trace(vk::CommandBuffer swapchainCommandBuffer, vk::Image swapchainImage, vk::Extent2D extent) {
    vk::DeviceSize progSize = mCapabilities.pipelineProperties.shaderGroupBaseAlignment;
//        vk::DeviceSize sbtSize = progSize * static_cast<vk::DeviceSize>(_shaderGroups);

    vk::DeviceAddress sbtAddress = vkCore::global::device.getBufferAddress(_sbtBuffer.get());

    vk::StridedDeviceAddressRegionKHR bufferRegionRayGen(sbtAddress,     // deviceAddress
                                                         progSize,       // stride
                                                         progSize * 1); // size

    vk::StridedDeviceAddressRegionKHR bufferRegionMiss(sbtAddress + (1U * progSize), // deviceAddress
                                                       progSize,                       // stride
                                                       progSize * 2);                 // size

    vk::StridedDeviceAddressRegionKHR bufferRegionChit(sbtAddress + (3U * progSize), // deviceAddress
                                                       progSize,                       // stride
                                                       progSize * 1);                 // size

    vk::StridedDeviceAddressRegionKHR callableShaderBindingTable(0,   // deviceAddress
                                                                 0,   // stride
                                                                 0); // size

    swapchainCommandBuffer.traceRaysKHR(&bufferRegionRayGen,         // pRaygenShaderBindingTable
                                        &bufferRegionMiss,           // pMissShaderBindingTable
                                        &bufferRegionChit,           // pHitShaderBindingTable
                                        &callableShaderBindingTable, // pCallableShaderBindingTable
                                        extent.width,                // width
                                        extent.height,               // height
                                        1);                         // depth
}

void RayTracer::initDescriptorSet() {
    // Tlas
    mDescriptors.bindings.add(0,
                              vk::DescriptorType::eAccelerationStructureKHR,
                              vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
    // Output image
    mDescriptors.bindings.add(1,
                              vk::DescriptorType::eStorageImage,
                              vk::ShaderStageFlagBits::eRaygenKHR);

    mDescriptors.layout = mDescriptors.bindings.initLayoutUnique();
    mDescriptors.pool = mDescriptors.bindings.initPoolUnique(vkCore::global::swapchainImageCount);
    _descriptorSets = vkCore::allocateDescriptorSets(mDescriptors.pool.get(), mDescriptors.layout.get());
}

void RayTracer::updateDescriptors() {
    vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo(1,
                                                            &mTlas.as.as);

    vk::DescriptorBufferInfo varianceBufferInfo(_varianceBuffer.get(),
                                                0,
                                                VK_WHOLE_SIZE);

    mDescriptors.bindings.write(_descriptorSets, 0, &tlasInfo);
    mDescriptors.bindings.write(_descriptorSets, 1, &mStorageImageInfo);

    mDescriptors.bindings.update();
}

void RayTracer::initVarianceBuffer(float width, float height) {
    // Create buffer to store variance value
    _varianceBuffer.init(sizeof(float) * 3 * width * height,
                         vk::BufferUsageFlagBits::eStorageBuffer,
                         {vkCore::global::graphicsFamilyIndex},
                         vk::MemoryPropertyFlagBits::eHostVisible);
}

// This is wrong! This calculates variance off all pixel colors of a frame.
// Instead, one should look at the individual pixel at increased sample rates, right?
float RayTracer::getPixelVariance(uint32_t index) {
    static bool alreadyMapped = false;
    static void *mapped = NULL;

    if (!alreadyMapped) {
        alreadyMapped = true;
        vk::Result result = vkCore::global::device.mapMemory(_varianceBuffer.getMemory(), 0,
                                                             _varianceBuffer.getSize(), {}, &mapped);
        KF_ASSERT(result == vk::Result::eSuccess, "Failed to map memory of variance buffer.");
    }

    auto *pData = reinterpret_cast<float *>(mapped);

    float res = (pData)[index];
    return res;
}
}
