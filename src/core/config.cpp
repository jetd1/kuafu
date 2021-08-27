//
// Created by jet on 4/9/21.
//

#include "core/config.hpp"
#include "core/context/global.hpp"

namespace kuafu {
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

        _clearColor = clearColor;

        global::frameCount = -1;
    }

    void Config::setNextEventEstimation(bool flag) {
        _nextEventEstimation = flag;
    }

    void Config::setNextEventEstimationMinBounces(uint32_t minBounces) {
        _nextEventEstimationMinBounces = minBounces;
    }

    void Config::setRussianRoulette(bool flag) {
        _russianRoulette = flag;
    }

    void Config::setRussianRouletteMinBounces(uint32_t minBounces) {
        _russianRouletteMinBounces = minBounces;
    }

    void Config::setAssetsPath(int argc, char *argv[]) {
        _assetsPath = "";

        for (int i = 0; i < argc; ++i) {
            _assetsPath += argv[i];
        }

        std::replace(_assetsPath.begin(), _assetsPath.end(), '\\', '/');

        _assetsPath = _assetsPath.substr(0, _assetsPath.find_last_of('/') + 1);

        global::assetsPath = _assetsPath;
    }

    void Config::setAssetsPath(std::string_view path) {
        _assetsPath = path;

        std::replace(_assetsPath.begin(), _assetsPath.end(), '\\', '/');

        if (path[path.size() - 1] != '/') {
            _assetsPath += '/';
        }

        global::assetsPath = _assetsPath;
    }

    void Config::setAutomaticPipelineRefresh(bool flag) {
        _automaticPipelineRefresh = flag;
    }

    void Config::setGeometryInstanceLimit(uint32_t amount) {
        if (amount == 0) {
            ++amount;
//            KF_WARN( "Can not use value 0 for the maximum amount of geometry instances. Using 1 instead." );
        }

        // Increment by one to accommodate the triangle dummy for emtpy scenes.
        _maxGeometryInstances = amount;

        _maxGeometryInstancesChanged = true;
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

        _maxGeometry = ++amount;

        _maxGeometryChanged = true;
    }

    void Config::setTextureLimit(size_t amount) {
        if (amount == 0) {
//            amount;
            KF_WARN( "Can not use value 0 for the maximum amount of textures. Using 1 instead." );
        }

        _maxTextures = ++amount;

        _maxTexturesChanged = true;
    }

    void Config::setDefaultAssetsPath() {
        _assetsPath = std::filesystem::current_path().string() += "/";

        std::replace(_assetsPath.begin(), _assetsPath.end(), '\\', '/');

        global::assetsPath = _assetsPath;
//        KF_WARN( "No path to assets specified. Using default path as path to resources: ", _assetsPath );
    }

    void Config::setUseDenoiser(bool useDenoiser) {
        mUseDenoiser = useDenoiser;
        // TODO: implement this
    }

    void Config::setPerPixelSampleRate(uint32_t sampleRate) {
        mPerPixelSampleRate = sampleRate;
    }

    void Config::setAccumulatingFrames(bool flag) {
        _accumulateFrames = flag;
    }

    void Config::updateVariance(bool flag) {
        _updateVariance = flag;
    }
}
