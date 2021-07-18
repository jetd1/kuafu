//
// Created by jet on 4/9/21.
//

#pragma once

#include <string>
#include <vector>
#include "core/geometry.hpp"

namespace kuafu::global {
    extern int frameCount;

    extern std::string assetsPath;
    extern uint32_t geometryIndex;
    extern uint32_t materialIndex;
    extern uint32_t textureIndex;
    const size_t maxResources = 2;
    extern std::vector<Material> materials;
}
