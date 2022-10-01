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
#include "core/context/context.hpp"
#include "core/window.hpp"
#include "core/gui.hpp"

namespace kuafu {
class Kuafu {
    std::shared_ptr<Window> pWindow = nullptr;
    std::shared_ptr<Gui> pGUI = nullptr;
    kuafu::Context mContext;

    bool mRunning = true;

    void reset();

public:
    explicit Kuafu(std::shared_ptr<Config> config = nullptr);

    void run();

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] std::vector<uint8_t> downloadLatestFrame(Camera* cam);

    void setWindow(std::shared_ptr<Window> other);

    void setWindow(int width, int height, const char *title = "App", uint32_t flags = 0);

    [[nodiscard]] auto getWindow() const { return pWindow; }

    void setGui(std::shared_ptr<Gui> gui);

    inline auto& getConfig() { return *mContext.pConfig; }

    inline Scene* getScene() { return mContext.mCurrentScene; }

    inline void setScene(Scene* scene) {
        if (mContext.mCurrentScene == scene and scene->initialized)
            return;

        KF_INFO("Switching scene, this is heavy...");

        KF_ASSERT(scene, "Trying to set an invalid scene!");
        auto ret = std::find_if(mContext.mScenes.begin(), mContext.mScenes.end(),
                                [scene](auto& s) { return scene == s.get(); });
        KF_ASSERT(ret != mContext.mScenes.end(), "???");

        scene->init();
        mContext.mCurrentScene = scene;
        mContext.pConfig->triggerSwapchainRefresh();
        mContext.pConfig->triggerPipelineRefresh();
        global::frameCount = -1;
    }

    inline Scene* createScene() {
        mContext.mScenes.emplace_back(new Scene(mContext.pConfig));
        return mContext.mScenes.back().get();
    };

    inline void removeScene(Scene* scene) {
        KF_ASSERT(scene, "Trying to remove an invalid scene!");
        auto ret = std::find_if(mContext.mScenes.begin(), mContext.mScenes.end(),
                                [scene](auto& s) { return scene == s.get(); });
        KF_ASSERT(ret != mContext.mScenes.end(), "???");

        mContext.mScenes.erase(std::remove_if(mContext.mScenes.begin(), mContext.mScenes.end(),
                                     [scene](auto &s) { return scene == s.get(); }), mContext.mScenes.end());
    }
};

}
