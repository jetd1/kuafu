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
#include "core/rt/as.hpp"
#include "core/context/global.hpp"

namespace kuafu {
void AccelerationStructure::destroy() {
    if (as) {
        vkCore::global::device.destroyAccelerationStructureKHR(as);
    }

    if (buffer) {
        vkCore::global::device.destroyBuffer(buffer);
    }

    if (memory) {
        vkCore::global::device.freeMemory(memory);
    }
}

auto initAccelerationStructure(vk::AccelerationStructureCreateInfoKHR &asCreateInfo) -> AccelerationStructure {
    kuafu::AccelerationStructure resultAs;

    vk::MemoryAllocateFlagsInfo allocateFlags(vk::MemoryAllocateFlagBitsKHR::eDeviceAddress);

    vk::BufferCreateInfo createInfo(
            {},                                                                                                       // flags
            asCreateInfo.size,                                                                                         // size
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress, // usage
            vk::SharingMode::eExclusive,                                                                               // sharingMode
            1,                                                                                                         // queueFamilyIndexCount
            &vkCore::global::graphicsFamilyIndex);                                                                    // pQueueFamilyIndices

    resultAs.buffer = vkCore::global::device.createBuffer(createInfo);
//        KF_ASSERT( resultAs.buffer, "Failed to create buffer." );

    resultAs.memory = vkCore::allocateMemory(resultAs.buffer,
                                             vk::MemoryPropertyFlagBits::eDeviceLocal |
                                             vk::MemoryPropertyFlagBits::eHostCoherent,
                                             &allocateFlags);

    vkCore::global::device.bindBufferMemory(resultAs.buffer, resultAs.memory, 0);

    asCreateInfo.buffer = resultAs.buffer;

    resultAs.as = vkCore::global::device.createAccelerationStructureKHR(asCreateInfo, nullptr);

    return resultAs;
}
}
