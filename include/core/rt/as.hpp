//
// Created by jet on 4/9/21.
//

#pragma once

#include "stdafx.hpp"

namespace kuafu {
    /// A wrapper for a Vulkan acceleration Structure.
    /// @ingroup API
    struct AccelerationStructure {
        vk::AccelerationStructureKHR as; ///< The Vulkan acceleration structure.
        vk::DeviceMemory memory;         ///< The acceleration structure's memory.
        vk::Buffer buffer;

        /// Used to destroy the acceleration structure and free its memory.
        void destroy();
    };

    /// A wrapper for a top level acceleration structure.
    /// @ingroup API
    struct Tlas {
        AccelerationStructure as; ///< The kuafu::AccelerationStructure object containing the Vulkan acceleration structure.
        //vk::BuildAccelerationStructureFlagsKHR flags; ///< The top level acceleration structure's build flags.
    };

    /// A wrapper for a bottom level acceleration structure.
    /// @ingroup API
    struct Blas {
        AccelerationStructure as; ///< The kuafu::AccelerationStructure object containing the Vulkan acceleration structure.
        //vk::BuildAccelerationStructureFlagsKHR flags; ///< The top level acceleration structure's build flags.

        std::vector<vk::AccelerationStructureGeometryKHR> asGeometry;             ///< Data used to build acceleration structure geometry.
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> asBuildRangeInfo; ///< The offset between acceleration structures when building.
    };

    /// Creates the acceleration structure and allocates and binds memory for it.
    /// @param asCreateInfo The Vulkan init info for the acceleration structure.
    /// @return Returns an kuafu::AccelerationStructure object that contains the AS itself as well as the memory for it.
    auto initAccelerationStructure(vk::AccelerationStructureCreateInfoKHR &asCreateInfo) -> AccelerationStructure;

    /*
    /// An instance of a bottom level acceleration structure.
    /// @ingroup API
    struct BlasInstance
    {
      uint32_t blasId     = 0; ///< The index of the bottom level acceleration structure in blas_.
      uint32_t instanceId = 0; ///< The instance index (gl_InstanceID).
      uint32_t hitGroupId = 0; ///< The hit group index in the shader binding table.
      uint32_t mask       = 0xFF; ///< The visibility mask, will be AND-ed with the ray mask.
      vk::GeometryInstanceFlagsKHR flags = { vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable }; ///< The geometry display options.
      glm::mat4 transform = glm::fmat4( 1.0f ); ///< The world transform matrix of the bottom level acceleration structure instance.
    };
    */
}
