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
#include "kuafu.hpp"
#include "core/context/global.hpp"

#include <utility>
#include <memory>

namespace kuafu {
Kuafu::Kuafu(std::shared_ptr<Config> config,
             std::shared_ptr<Window> window,
             std::shared_ptr<Camera> camera,
             std::shared_ptr<Gui> gui) {
    pWindow = window ? window
            : std::make_shared<Window>();
    mContext.pWindow = pWindow;

    mContext.mScene.mCurrentCamera = camera ? camera
            : std::make_shared<Camera>(pWindow->getWidth(), pWindow->getHeight());

    mContext.pConfig = config ? config
            : std::make_shared<Config>();
    mContext.mScene.pConfig = mContext.pConfig;

    if (mContext.pConfig->getAssetsPath() == "")
        mContext.pConfig->setAssetsPath(mContext.pConfig->sDefaultAssetsPath);

    if (mInitialized)
        throw std::runtime_error("Renderer was already initialized.");

    mInitialized = pWindow->init();
    mContext.init();
}

auto Kuafu::isRunning() const -> bool {
    if (!mRunning)
        KF_INFO("Shutting down Kuafu.");

    return mRunning;
}

void Kuafu::run() {
    if (!mRunning || !mInitialized) {
        KF_WARN("!mRunning || !mInitialized");
        return;
    }

    mRunning = pWindow->update();
    mContext.mScene.mCurrentCamera->update();
    mContext.render();
}

std::vector<uint8_t> Kuafu::downloadLatestFrame() {
    return mContext.downloadLatestFrame();
}

void Kuafu::setWindow(std::shared_ptr<Window> window) {
  pWindow = window;
    mContext.pWindow = pWindow;
}

void Kuafu::setWindow(int width, int height, const char *title, uint32_t flags) {
  pWindow = std::make_shared<Window>(width, height, title, flags);
    mContext.pWindow = pWindow;
}

void Kuafu::setGui(std::shared_ptr<Gui> gui) {
    mInitialized ? mContext.setGui(gui, true) : mContext.setGui(gui);
}

void Kuafu::reset() {
    // Reset indices
    kuafu::global::geometryIndex = 0;
    kuafu::global::textureIndex = 0;
    kuafu::global::materialIndex = 0;

    // Reset frame counter
    kuafu::global::frameCount = -1;

    mContext.mScene.getCamera()->reset();

    // Delete all textures
    global::materials.clear();
    global::materials.reserve(mContext.mScene.pConfig->mMaxMaterials);

    mContext.mScene.mTextures.clear();
    mContext.mScene.mTextures.resize(static_cast<size_t>( mContext.mScene.pConfig->mMaxTextures));
    mContext.mScene.updateGeoemtryDescriptors();
}
}
