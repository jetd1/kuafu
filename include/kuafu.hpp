//
// Created by jet on 4/9/21.
//

#pragma once

#include <memory>
#include "core/window.hpp"
#include "core/context/context.hpp"

namespace kuafu {
    class Kuafu {
        std::shared_ptr<Window> mWindow = nullptr;
        std::shared_ptr<Gui> mGUI = nullptr;
        kuafu::Context mContext;

        bool mInitialized = false;
        bool mRunning = true;

    public:
        void init();

        void run();

        bool isRunning() const;

        const std::vector<uint8_t>& downloadLatestFrame() const;

        void setWindow(std::shared_ptr<Window> other);

        void setWindow(int width, int height, const char *title = "App", uint32_t flags = 0);

        auto getWindow() const { return mWindow; }

        void setGui(std::shared_ptr<Gui> gui);

        inline kuafu::Config &getConfig() { return mContext.mConfig; }

        inline kuafu::Scene &getScene() { return mContext.mScene; }

        void reset();
    };

}
