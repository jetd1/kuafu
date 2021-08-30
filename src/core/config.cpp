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
#include "core/config.hpp"
#include "core/context/global.hpp"

namespace kuafu {
std::string Config::sDefaultAssetsPath;

void Config::setDefaultAssetsPath(std::string p) { sDefaultAssetsPath = std::move(p); }

void Config::setPathDepth(uint32_t recursionDepth) {
    if (recursionDepth <= mMaxPathDepth) {
        mPathDepth = recursionDepth;
    } else {
        mPathDepth = mMaxPathDepth;
//            KF_WARN( "Exceeded maximum path depth of ", mMaxPathDepth, ". Using highest possible value instead." );
    }

    mPathDepth = recursionDepth;
    //triggerPipelineRefresh( );
}

void Config::setClearColor(const glm::vec4 &clearColor) {
    static bool firstRun = true;

    if (firstRun) {
        firstRun = false;
    } else {
        triggerSwapchainRefresh();
    }

    mClearColor = clearColor;

    global::frameCount = -1;
}

void Config::setNextEventEstimation(bool flag) { mNextEventEstimation = flag;
}

void Config::setNextEventEstimationMinBounces(uint32_t minBounces) {
  mNextEventEstimationMinBounces = minBounces;
}

void Config::setRussianRoulette(bool flag) { mRussianRoulette = flag;
}

void Config::setRussianRouletteMinBounces(uint32_t minBounces) {
  mRussianRouletteMinBounces = minBounces;
}

void Config::setAssetsPath(int argc, char *argv[]) {
  mAssetsPath = "";

    for (int i = 0; i < argc; ++i) {
      mAssetsPath += argv[i];
    }

    std::replace(mAssetsPath.begin(), mAssetsPath.end(), '\\', '/');

    mAssetsPath = mAssetsPath.substr(0, mAssetsPath.find_last_of('/') + 1);

    global::assetsPath = mAssetsPath;
}

void Config::setAssetsPath(std::string_view path) {
  mAssetsPath = path;

    std::replace(mAssetsPath.begin(), mAssetsPath.end(), '\\', '/');

    if (path[path.size() - 1] != '/') {
      mAssetsPath += '/';
    }

    global::assetsPath = mAssetsPath;
}

void Config::setAutomaticPipelineRefresh(bool flag) { mAutomaticPipelineRefresh = flag;
}

void Config::setGeometryInstanceLimit(uint32_t amount) {
    if (amount == 0) {
        ++amount;
        KF_WARN("Can not use value 0 for the maximum amount of geometry instances. Using 1 instead.");
    }

    // Increment by one to accommodate the triangle dummy for emtpy scenes.
    mMaxGeometryInstances = amount;

    mMaxGeometryInstancesChanged = true;
}

void Config::setGeometryLimit(size_t amount) {
    if (amount == 0) {
//            KF_WARN( "Can not use value 0 for the maximum number of geometries. Using 16 instead." );
        amount = 16;
    }

    if (amount < 16) {
//            KF_WARN( "Can not use value ", amount, " for the maximum number of geometries. Using 16 instead." );
        amount = 16;
    }

    if (amount % 4 != 0) {
//            KF_WARN( "Minimum storage buffer for geometries alignment must be a multiple of 4. Using 16 instead." );
        amount = 16;
    }

    mMaxGeometry = ++amount;

    mMaxGeometryChanged = true;
}

void Config::setTextureLimit(size_t amount) {
    if (amount == 0) {
//            amount;
        KF_WARN("Can not use value 0 for the maximum amount of textures. Using 1 instead.");
    }

    mMaxTextures = ++amount;

    mMaxTexturesChanged = true;
}

void Config::setUseDenoiser(bool useDenoiser) {
    mUseDenoiser = useDenoiser;
    // TODO: implement this
}

void Config::setPerPixelSampleRate(uint32_t sampleRate) {
    mPerPixelSampleRate = sampleRate;
}

void Config::setAccumulatingFrames(bool flag) { mAccumulateFrames = flag;
}

void Config::updateVariance(bool flag) { mUpdateVariance = flag;
}
}
