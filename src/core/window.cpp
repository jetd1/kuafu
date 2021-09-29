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

Window::Window(int width, int height, const std::string &title, uint32_t flags, Camera* camera) :
        mFlags(flags),
        mWidth(width),
        mHeight(height),
        mTitle(title),
        pCamera(camera) {
    mFlags |= SDL_WINDOW_VULKAN;
}

Window::~Window() {
    SDL_DestroyWindow(pWindow);
    pWindow = nullptr;
    SDL_Quit();
}


auto Window::init() -> bool {
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

    SDL_SetRelativeMouseMode(SDL_FALSE);

    return true;
}

auto Window::update() -> bool {
    // Updates local timer bound to this window.

    if (mCalledResize) {       // If called manually
      mCalledResize = false;
    } else {                   // Fetch the latest window dimensions.
        int width, height;
        SDL_GetWindowSize(pWindow, &width, &height);
        mWidth = width;
        mHeight = height;
    }

    // Add your custom event polling and integrate your event system.
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
//            ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT: {
                return false;
            }

            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        return false;

                    case SDL_WINDOWEVENT_RESIZED:
                        resize(static_cast<int>( event.window.data1 ), static_cast<int>( event.window.data2 ));
                        break;

                    case SDL_WINDOWEVENT_MINIMIZED:
                        resize(0, 0);
                        break;
                }
                break;
            }

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        global::keys::eW = true;
                        break;

                    case SDLK_a:
                        global::keys::eA = true;
                        break;

                    case SDLK_s:
                        global::keys::eS = true;
                        break;

                    case SDLK_d:
                        global::keys::eD = true;
                        break;

                    case SDLK_LSHIFT:
                        global::keys::eLeftShift = true;
                        break;

                    case SDLK_ESCAPE:
                        return false;

                    case SDLK_c:
                        global::keys::eC = true;
                        break;

                    case SDLK_b:
                        global::keys::eB = true;
                        break;

                    case SDLK_l:
                        global::keys::eL = true;
                        break;

                    case SDLK_LCTRL:
                        global::keys::eLeftCtrl = true;
                        break;

                    case SDLK_SPACE: {
                        if (_mouseVisible) {
                            _mouseVisible = false;
                            SDL_SetRelativeMouseMode(SDL_TRUE);
                            SDL_GetRelativeMouseState(nullptr, nullptr); // Magic fix!
                        } else {
                            SDL_SetRelativeMouseMode(SDL_FALSE);
                            _mouseVisible = true;
                        }

                        break;
                    }
                }
                break;
            }

            case SDL_KEYUP: {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        global::keys::eW = false;
                        break;

                    case SDLK_a:
                        global::keys::eA = false;
                        break;

                    case SDLK_s:
                        global::keys::eS = false;
                        break;

                    case SDLK_d:
                        global::keys::eD = false;
                        break;

                    case SDLK_LSHIFT:
                        global::keys::eLeftShift = false;
                        break;

                    case SDLK_LCTRL:
                        global::keys::eLeftCtrl = false;
                        break;

                    case SDLK_c:
                        global::keys::eC = false;
                        break;

                    case SDLK_b:
                        global::keys::eB = false;
                        break;

                    case SDLK_l:
                        global::keys::eL = false;
                        break;
                }
                break;
            }

            case SDL_MOUSEMOTION: {
                if (!_mouseVisible) {
                    int x;
                    int y;
                    SDL_GetRelativeMouseState(&x, &y);
                    pCamera->processMouse(x, -y);
                    break;
                }
            }
        }
    }

    return true;
}

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