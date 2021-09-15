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
    mContext.pConfig = std::make_shared<Config>(*config);           // TODO: make config a value member instead
    mContext.mScene.pConfig = mContext.pConfig;

    if (mContext.pConfig->mPresent) {
        KF_INFO("Present mode enabled.");

        if (!window) {
            KF_WARN("No window specified, creating a default window.");
            if (camera)
                window = std::make_shared<Window>(camera->getWidth(), camera->getHeight());
            else {
                KF_WARN("Adding a default camera for the viewer.");
                window = std::make_shared<Window>();
                camera = std::make_shared<Camera>(window->getWidth(), window->getHeight());
            }
        } else {
            if (!camera) {
                KF_WARN("Adding a default camera for the viewer.");
                camera = std::make_shared<Camera>(window->getWidth(), window->getHeight());
            }
        }

    } else {
        KF_INFO("Offscreen mode enabled.");

        if (window) {
            KF_WARN("Specified window will be ignored.");
            window = nullptr;
        }

    }

    pWindow = window;
    mContext.pWindow = window;

    mContext.mScene.mCurrentCamera = camera;

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
    mContext.setGui(gui, true);
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
