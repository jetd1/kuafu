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
#include "core/context/global.hpp"

namespace kuafu::global {
std::shared_ptr<spdlog::logger> logger =
        spdlog::stderr_color_mt("kuafu");

int frameCount = -1;
std::string assetsPath;
uint32_t materialIndex = 0;
uint32_t textureIndex = 0;
std::vector<NiceMaterial> materials;

namespace keys {
bool eW;
bool eA;
bool eS;
bool eD;

bool eQ;
bool eE;

bool eX;
bool eY;
bool eZ;

bool eC;
bool eSpace;
bool eLeftShift;
bool eLeftCtrl;
bool eB;
bool eL;
}
}