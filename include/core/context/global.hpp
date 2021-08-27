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

    namespace keys {
        extern bool eW;
        extern bool eA;
        extern bool eS;
        extern bool eD;

        extern bool eQ;
        extern bool eE;

        extern bool eX;
        extern bool eY;
        extern bool eZ;

        extern bool eC;
        extern bool eSpace;
        extern bool eLeftShift;
        extern bool eLeftCtrl;
        extern bool eB;
        extern bool eL;
    }
}
