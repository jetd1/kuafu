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
#define GLM_ENABLE_EXPERIMENTAL

namespace kuafu {

struct Vertex {
    glm::vec3 pos;      ///< The vertex's position in 3D space.
    glm::vec3 normal;   ///< The vertex's normal vector.
    glm::vec3 color;    ///< The vertex's color.
    glm::vec2 texCoord; ///< The vertex's texture coordinate.
    float padding0;     ///< Vertex padding 0.

    /// @return Returns the hard-coded vertex format (eR32G32B32Sfloat).
    static auto getVertexPositionFormat() -> vk::Format { return vk::Format::eR32G32B32Sfloat; }

    /// @return Returns the vertex's Vulkan binding description.
    static auto getBindingDescriptions() -> std::array<vk::VertexInputBindingDescription, 1> {
        std::array<vk::VertexInputBindingDescription, 1> bindingDescriptions{};

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

        return bindingDescriptions;
    }

    /// @return Returns the vertex's Vulkan attribute descriptions for all four attributes.
    static auto getAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 4> {
        std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = getVertexPositionFormat();
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, color);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    /// @cond INTERNAL
    auto operator==(const Vertex &other) const -> bool {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
    /// @endcond
};
}

namespace std {
/// @cond INTERNAL
template<>
struct hash<kuafu::Vertex> {
    auto operator( )(const kuafu::Vertex &vertex) const -> size_t {
        size_t hashPos = hash<glm::vec3>()(vertex.pos);
        size_t hashColor = hash<glm::vec3>()(vertex.color);
        size_t hashTexCoord = hash<glm::vec2>()(vertex.texCoord);
        size_t hashNormal = hash<glm::vec3>()(vertex.normal);

        return ((((hashPos ^ (hashColor << 1)) >> 1) ^ hashTexCoord) << 1) ^ hashNormal;
    }
};
/// @endcond
} // namespace std

