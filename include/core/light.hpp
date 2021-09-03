//
// By Jet <i@jetd.me>, 2021.
//
#pragma once
#include "context/global.hpp"

namespace kuafu {

struct DirectionalLight {
    glm::vec3 direction = {0, 0, -1};
    glm::vec3 color = {1, 1, 1};
    float     strength = 0.0;      // Don't know the physical meaning just a factor :P
    float     softness = 0.0;      // Don't know the physical meaning just a factor :P
};

struct PointLight {
    glm::vec3 position = {0, 0, 0};
    glm::vec3 color = {1, 1, 1};
    float     radius = 1;          // Some blender black magic for shadows
    float     strength = 0.0;      // Don't know the physical meaning just a factor :P
};

//struct Spotlight {
//    glm::vec3 position = {0, 0, 0};
//    glm::vec3 color = {1, 1, 1};
//
//    float fovInner = 0.0;
//    float fovOuter = 0.0;
//};

struct ActiveLight {
    glm::mat4 viewMat {};
    glm::vec3 color = {1, 1, 1};
    float fov = 0.0;
    float strength = 0.0;
    float softness = 0.0;                      // TODO: softness -> radius
    std::string texPath;
    int texID = -1;
};

struct DirectionalLightUBO {
    glm::vec4 direction {};    // direction + softness
    glm::vec4 rgbs {};         // rgb + strength
};

struct PointLightsUBO {
    glm::vec4   posr[global::maxPointLights] {};        // position + radius
    glm::vec4   rgbs[global::maxPointLights] {};        // rgb + strength
};

struct ActiveLightsUBO {
    glm::mat4   viewMat[global::maxActiveLights] {};         // mat44 viewMat
    glm::mat4   projMat[global::maxActiveLights] {};         // mat33 projMat
    glm::vec4   front[global::maxActiveLights] {};           // vec3 front
    glm::vec4   rgbs[global::maxActiveLights] {};            // vec3 color + float strength
    glm::vec4   position[global::maxActiveLights] {};        // vec3 position
    glm::vec4   sftp[global::maxActiveLights] {};            // softness, fov, texID, padding
};
};

