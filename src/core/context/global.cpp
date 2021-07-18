//
// Created by jet on 4/9/21.
//

#include "core/context/global.hpp"

namespace kuafu::global {
    int frameCount = -1;
    std::string assetsPath;
    uint32_t geometryIndex = 0;
    uint32_t materialIndex = 0;
    uint32_t textureIndex = 0;
    std::vector<Material> materials;
}