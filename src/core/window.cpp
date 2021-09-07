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
#include "core/window.hpp"
#include "core/context/global.hpp"
#include "SDL_vulkan.h"

namespace kuafu {

Window::Window(int width, int height, const std::string &title, uint32_t flags) :
        mFlags(flags),
        mWidth(width),
        mHeight(height),
        mTitle(title) {
    mFlags |= SDL_WINDOW_VULKAN;
}

Window::~Window() {
    SDL_DestroyWindow(pWindow);
    pWindow = nullptr;
    SDL_Quit();
}


auto Window::init() -> bool {
    //SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );
    //SDL_SetHint( SDL_HINT_RENDER_VSYNC, "1" );

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error("SDL Error, Closing application.");
        return false;
    }

    pWindow = SDL_CreateWindow(mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight,
                               mFlags);

    if (pWindow == nullptr) {
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

    if (mCalledResize) {       // If called manually
      mCalledResize = false;
    } else {                   // Fetch the latest window dimensions.
        int width, height;
        SDL_GetWindowSize(pWindow, &width, &height);
        mWidth = width;
        mHeight = height;
    }

    return true;
}

//void Window::clean( )
//{
//    SDL_DestroyWindow( pWindow );
//    pWindow = nullptr;
//
//    SDL_Quit( );
//}


void Window::resize(int width, int height) {
    mWidth = width;
    mHeight = height;

    SDL_SetWindowSize(pWindow, width, height);
    mCalledResize = true;
}

vk::Extent2D Window::getSize() const {
    int width;
    int height;
    SDL_GetWindowSize(pWindow, &width, &height);
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
    return (SDL_GetWindowFlags(pWindow) & SDL_WINDOW_MINIMIZED) != 0U;
}

std::vector<const char*> Window::getExtensions() const {
    // Retrieve all extensions needed by SDL2.
    uint32_t sdlExtensionsCount;
    SDL_bool result = SDL_Vulkan_GetInstanceExtensions(pWindow, &sdlExtensionsCount, nullptr);

    if (result != SDL_TRUE)
        throw std::runtime_error("Failed to get extensions required by SDL.");

    const char** sdlExtensionsNames = new const char *[sdlExtensionsCount];
    for (size_t i = 0; i < sdlExtensionsCount; i++)
        sdlExtensionsNames[i] = new char[50];
    result = SDL_Vulkan_GetInstanceExtensions(pWindow, &sdlExtensionsCount, sdlExtensionsNames);

    if (result != SDL_TRUE)
        throw std::runtime_error("Failed to get extensions required by SDL.");

    std::vector<const char*> ret(sdlExtensionsCount);
    for (size_t i = 0; i < sdlExtensionsCount; i++) {
        ret[i] = sdlExtensionsNames[i];                    // FIXME: this is potentially dangerous
    }
    delete[] sdlExtensionsNames;

    return ret;
}

}