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

#include "core/geometry.hpp"

namespace kuafu::global {
extern std::shared_ptr<spdlog::logger> logger;

extern int frameCount;

extern std::string assetsPath;
extern uint32_t materialIndex;
extern uint32_t textureIndex;
extern std::vector<NiceMaterial> materials;

const size_t maxResources = 2;
const size_t maxPointLights = 32;
const size_t maxActiveLights = 8;

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
