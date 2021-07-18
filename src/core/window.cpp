//
// Created by jet on 4/9/21.
//

#include "core/window.hpp"
#include "core/context/global.hpp"
#include "sdl2/SDL_vulkan.h"

namespace kuafu {

    Window::Window(int width, int height, std::string title, uint32_t flags) :
            mWidth(width),
            mHeight(height),
            mTitle(title),
            mFlags(flags) {
        mFlags |= SDL_WINDOW_VULKAN;
    }

    Window::~Window() {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
        SDL_Quit();
    }


    auto Window::init() -> bool {
        //SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );
        //SDL_SetHint( SDL_HINT_RENDER_VSYNC, "1" );

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("SDL Error, Closing application.");
            return false;
        }

        mWindow = SDL_CreateWindow(mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight,
                                   mFlags);

        if (mWindow == nullptr) {
            throw std::runtime_error("Failed to create window. Closing application.");
            return false;
        }

        //if ( SDL_GL_SetSwapInterval( 1 ) == -1 )
        //{
        //  throw std::runtime_error( "Swap interval not supported." );
        //  return false;
        //}

        return true;
    }

    auto Window::update() -> bool {
        // Updates local timer bound to this window.
        Time::update();

        // Fetch the latest window dimensions.
        int width;
        int height;
        SDL_GetWindowSize(mWindow, &width, &height);
        resize(width, height);

        return true;
    }

//void Window::clean( )
//{
//    SDL_DestroyWindow( mWindow );
//    mWindow = nullptr;
//
//    SDL_Quit( );
//}


    void Window::resize(int width, int height) {
        mWidth = width;
        mHeight = height;
    }

    vk::Extent2D Window::getSize() const {
        int width;
        int height;
        SDL_GetWindowSize(mWindow, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    bool Window::changed() {
        static int prevWidth = mWidth;
        static int prevHeight = mHeight;

        if (mWidth != prevWidth || mHeight != prevHeight) {
            global::frameCount = -1;

            prevWidth = mWidth;
            prevHeight = mHeight;
            return true;
        }

        return false;
    }

    bool Window::minimized() {
        return (SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED) != 0U;
    }

    auto Window::getExtensions() const -> gsl::span<const char *> {
        // Retrieve all extensions needed by SDL2.
        uint32_t sdlExtensionsCount;
        SDL_bool result = SDL_Vulkan_GetInstanceExtensions(mWindow, &sdlExtensionsCount, nullptr);

        if (result != SDL_TRUE)
            throw std::runtime_error("Failed to get extensions required by SDL.");

        gsl::owner<const char **> sdlExtensionsNames = new const char *[sdlExtensionsCount];
        result = SDL_Vulkan_GetInstanceExtensions(mWindow, &sdlExtensionsCount, sdlExtensionsNames);

        if (result != SDL_TRUE)
            throw std::runtime_error("Failed to get extensions required by SDL.");

        return gsl::span<const char *>(sdlExtensionsNames, sdlExtensionsCount);
    }

}