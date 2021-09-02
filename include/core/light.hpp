//
// By Jet <i@jetd.me>, 2021.
//
#pragma once
#include "context/global.hpp"

namespace kuafu {

struct DirectionalLight {
    glm::vec3 direction = {0, 0, -1};
    glm::vec3 color = {1, 1, 1};
    float     strength = 10.0;     // Don't know the physical meaning just a factor :P
    float     softness = 0.0;      // Don't know the physical meaning just a factor :P
};

struct PointLight {
    glm::vec3 position = {0, 0, 0};
    glm::vec3 color = {1, 1, 1};
    float     radius = 1;          // Some blender black magic for shadows
    float     strength = 0.0;      // Don't know the physical meaning just a factor :P
};

struct Spotlight {
    // TODO: (don't want to add this at all)
};

struct DirectionalLightUBO {
    glm::vec4 direction {};    // direction + softness
    glm::vec4 rgbs {};         // rgb + strength
};

struct PointLightsUBO {
    glm::vec4   posr[global::maxPointLights] {};        // position + radius
    glm::vec4   rgbs[global::maxPointLights] {};        // rgb + strength
};

};

