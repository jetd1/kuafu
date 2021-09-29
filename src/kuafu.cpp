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

namespace kuafu {
Kuafu::Kuafu(std::shared_ptr<Config> config) {
    mContext.pConfig = std::make_shared<Config>(*config);           // TODO: make config a value member instead
    mContext.mCurrentScene = createScene();

    std::shared_ptr<Window> window;

    if (mContext.pConfig->mPresent) {

        KF_INFO("Present mode enabled.");

        auto cam = mContext.mCurrentScene->createCamera(
                mContext.pConfig->mInitialWidth, mContext.pConfig->mInitialHeight);
        mContext.mCurrentScene->setCamera(cam);

        window = std::make_shared<Window>(
                mContext.pConfig->mInitialWidth, mContext.pConfig->mInitialHeight,
                "Viewer", SDL_WINDOW_RESIZABLE, cam);

    } else
        KF_INFO("Offscreen mode enabled.");

    pWindow = window;
    mContext.pWindow = window;

    if (mContext.pConfig->getAssetsPath().empty())
        mContext.pConfig->setAssetsPath(mContext.pConfig->sDefaultAssetsPath);

    if (pWindow)
        pWindow->init();
    else {
        assert(SDL_Init(SDL_INIT_VIDEO) == 0);
        assert(SDL_Vulkan_LoadLibrary(nullptr) == 0);
    }

    mContext.init();
}

auto Kuafu::isRunning() const -> bool {
    if (!mRunning)
        KF_INFO("Shutting down Kuafu.");

    return mRunning;
}

void Kuafu::run() {
    if (!mRunning)
        return;

    Time::update();

    if (mContext.pConfig->mPresent) {
        mRunning = pWindow->update();
        mContext.mSurface.setExtent(pWindow->getSize());
    }

    mContext.mCurrentScene->mCurrentCamera->update();
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
    mContext.setGui(gui, true);
}

void Kuafu::reset() {
    // Reset indices
//    kuafu::global::textureIndex = 0;
//    kuafu::global::materialIndex = 0;

    // Reset frame counter
    kuafu::global::frameCount = -1;

//    mContext.mCurrentScene->getCamera()->resetView();

//    // Delete all textures
//    global::materials.clear();
//    global::materials.reserve(mContext.pConfig->mMaxMaterials);

//    mContext.mCurrentScene->mTextures.clear();
//    mContext.mCurrentScene->mTextures.resize(static_cast<size_t>(mContext.pConfig->mMaxTextures));
//    mContext.mCurrentScene->updateGeometryDescriptors();
}
}
