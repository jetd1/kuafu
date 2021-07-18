//
// Created by jet on 4/9/21.
//

#include "kuafu.hpp"
#include "core/context/global.hpp"

#include <utility>
#include <memory>

namespace kuafu {
    void Kuafu::init() {
//        KF_INFO( "Starting Kuafu." );

        if (mWindow == nullptr) {
//            KF_VERBOSE( "No custom window implementation was provided. Using default implementation instead." );
            mWindow = std::make_shared<Window>();
        }

        if (mContext.mScene.mCurrentCamera == nullptr) {
//            KF_VERBOSE( "No custom camera implementation was provided. Using default implementation instead." );
            mContext.mScene.mCurrentCamera = std::make_shared<Camera>(mWindow->getWidth(), mWindow->getHeight());
            mContext.mScene.mCameras.insert(mContext.mScene.mCurrentCamera);
        }

        mContext.mWindow = mWindow;
        mContext.mCamera = mContext.mScene.mCurrentCamera;
        mContext.mScene._settings = &mContext.mConfig;

        if (mContext.mConfig.getAssetsPath().empty()) {
            //mContext.mConfig.setDefaultAssetsPath();
        }

        if (mInitialized) {
            throw std::runtime_error("Renderer was already initialized.");
            return;
        }

//#ifdef KF_COPY_ASSETS
//        KF_INFO( "Copying resources to binary output directory. " );
//
//        std::filesystem::copy( KF_ASSETS_PATH "shaders", KF_PATH_TO_LIBRARY "shaders", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
//        std::filesystem::copy( KF_ASSETS_PATH "models", KF_PATH_TO_LIBRARY "models", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
//        std::filesystem::copy( KF_ASSETS_PATH "DroidSans.ttf", KF_PATH_TO_LIBRARY "DroidSans.ttf", std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive );
//#endif

        mInitialized = mWindow->init();
        mContext.init();
    }

    auto Kuafu::isRunning() const -> bool {
        if (!mRunning) {
//            KF_INFO( "Shutting down Kuafu." );
        }

        return mRunning;
    }

    void Kuafu::run() {
        if (!mRunning || !mInitialized)
            return;

        mRunning = mWindow->update();
        mContext.mScene.mCurrentCamera->update();

        mContext.render();
    }

    void Kuafu::setWindow(std::shared_ptr<Window> window) {
        mWindow = window;
        mContext.mWindow = mWindow;
    }

    void Kuafu::setWindow(int width, int height, const char *title, uint32_t flags) {
        mWindow = std::make_shared<Window>(width, height, title, flags);
        mContext.mWindow = mWindow;
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
        global::materials.reserve(mContext.mScene._settings->_maxMaterials);

        mContext.mScene._textures.clear();
        mContext.mScene._textures.resize(static_cast<size_t>( mContext.mScene._settings->_maxTextures ));
        mContext.mScene.updateGeoemtryDescriptors();
    }
}
