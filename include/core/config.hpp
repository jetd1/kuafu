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

#include "stdafx.hpp"

namespace kuafu {
/// Exposes all graphic settings supported by the renderer.
///
/// Any necessary pipeline recreations and swapchain recreations will not be performed at the point of calling any setter but instead the next time the renderer
/// will be updated.
/// @warning Any function that sets the maximum of a given entity needs to be called before kuafu::Kuafu::init().
/// @ingroup BASE
/// @todo Add a setUseTotalPathsOnly( bool flag )
class Config {
public:
    friend class Context;

    friend class Kuafu;

    friend class Scene;

    /// @return Returns the path depth.
    auto getPathDepth() const -> uint32_t { return mPathDepth; }

    /// Used to set the path depth.
    ///
    /// The function will trigger a pipeline recreation as soon as possible unless it was explicitely disabled using setAutomaticPipelineRefresh(bool).
    /// If a value higher than the device's maximum supported value is set, it will use the maximum value instead.
    /// @param recursionDepth The new value for the recursion depth.
    void setPathDepth(uint32_t recursionDepth);

    bool getRussianRoulette() { return mRussianRoulette; }

    void setRussianRoulette(bool flag);

    bool getNextEventEstimation() { return mNextEventEstimation; }

    void setNextEventEstimation(bool flag);

    uint32_t getNextEventEstimationMinBounces() { return mNextEventEstimationMinBounces; }

    void setNextEventEstimationMinBounces(uint32_t minBounces);

    uint32_t getRussianRouletteMinBounces() { return mRussianRouletteMinBounces; }

    void setRussianRouletteMinBounces(uint32_t minBounces);

    /// @return Returns the maximum path depth on the GPU.
    auto getMaxPathDepth() const -> uint32_t { return mMaxPathDepth; }

    /// This function will be called by Kuafu::init() in case the path was not set manually.
    /// @warning This function might file in setting the correct path. That is why it is recommended to set it automatically using setAssetsPath(std::string).
    static std::string sDefaultAssetsPath;

    static void setDefaultAssetsPath(std::string path);

    /// @return Returns the path to assets.
    auto getAssetsPath() const -> std::string_view { return mAssetsPath; }

    /// Used to set a path to the directory containing all assets.
    ///
    /// This path should contain all models, textures and shaders.
    /// @param argc The argc parameter that can be retrieved from the main-function's parameters.
    /// @param argv The argv parameter that can be retrieved from the main-function's parameters.
    void setAssetsPath(int argc, char *argv[]);

    /// Used to set a path to the directory containing all assets.
    ///
    /// This path should contain all models, textures and shaders.
    /// @param path The path to assets.
    void setAssetsPath(std::string_view path);

    /// Used to toggle the automatic pipeline recreation.
    /// @param flag If false, the pipelines will not be recreated automatically until this function is called with true.
    void setAutomaticPipelineRefresh(bool flag);

    /// Used to set the maximum amount of geometry (3D models) that can be loaded.
    void setGeometryLimit(size_t amount);

    /// Used to set the maximum amount of geometry instances (instances of 3D models) that can be loaded.
    void setGeometryInstanceLimit(uint32_t amount);

    /// Used to set the maximum amount of textures that can be loaded.
    void setTextureLimit(size_t amount);

    void setMaterialLimit(size_t amount);


    void setPerPixelSampleRate(uint32_t sampleRate);

    auto getPerPixelSampleRate() const -> uint32_t { return mPerPixelSampleRate; }

    void setUseDenoiser(bool useDenoiser = true);

    auto isUsingDenoiser() const -> bool { return mUseDenoiser; }

    void setAccumulatingFrames(bool flag);

    auto isAccumulatingFrames() const -> bool { return mAccumulateFrames; }

    void triggerPipelineRefresh() { mPipelineNeedsRefresh = true; }

    void triggerSwapchainRefresh() { mSwapchainNeedsRefresh = true; }

    float getVariance() { return mVariance; }

    void updateVariance(bool flag);

    inline void setPresent(bool present) { mPresent = present; }

    inline bool getPresent() { return mPresent; }

    inline void setInitialWidth(int w) { mInitialWidth = w; }
    inline void setInitialHeight(int h) { mInitialHeight = h; }

private:
    // TODO: separate into fixed and changeable parts

    int mInitialWidth = 800;
    int mInitialHeight = 600;

    bool mPresent = true;                                  /// not changeable: whether or not initialize the surface
    vk::Format mFormat = vk::Format::eB8G8R8A8Srgb;
    vk::ColorSpaceKHR mColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    bool mUseDenoiser = false;  // todo

    bool mPipelineNeedsRefresh = false; ///< Keeps track of whether or not the graphics pipeline needs to be recreated.
    bool mSwapchainNeedsRefresh = false; ///< Keeps track of whether or not the swapchain needs to be recreated.

    size_t mMaxGeometryInstances = 256; ///< Can be set to avoid pipeline recreation everytime a geometry instance is added.
    bool mMaxGeometryInstancesChanged = false;
    size_t mMaxGeometry = 128; ///< The maximum amount of geometry (Must be a multiple of minimum storage buffer alignment).
    bool mMaxGeometryChanged = false;
    size_t mMaxTextures = 128; ///< The maximum amount of textures.
    bool mMaxTexturesChanged = false;
    size_t mMaxMaterials = 256;

    std::string mAssetsPath; ///< Where all assets like ~~~models, textures and~~~ shaders are stored.

    uint32_t mMaxPathDepth = 12;                                     ///< The maximum path depth.
    uint32_t mPathDepth = 8;                                         ///< The current path depth.
    uint32_t mPerPixelSampleRate = 32;                                      ///< Stores the total amount of samples that will be taken and averaged per pixel.
    uint32_t mRussianRouletteMinBounces = 4;

    bool mNextEventEstimation = true;            // TODO: not used!
    uint32_t mNextEventEstimationMinBounces = 0; // TODO: not used!

    float mVariance = 0.0F;
    bool mUpdateVariance = false;

    bool mAccumulateFrames = true;
    bool mRussianRoulette = true;
    bool mAutomaticPipelineRefresh = false;  ///< Keeps track of whether or not the graphics pipelines should be recreated automatically as soon as possible.
    bool mAutomaticSwapchainRefresh = false; ///< Keeps track of whether or not the swapchain should be recreated automatically as soon as possible.
};
}
