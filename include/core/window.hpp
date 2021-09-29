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
#include "camera.hpp"

namespace kuafu {

class Window {
protected:
    SDL_Window *pWindow = nullptr;
    uint32_t mFlags = {};

    int mWidth = 800;
    int mHeight = 600;
    std::string mTitle = "Kuafu App";
    bool mCalledResize = false;

    Camera* pCamera = nullptr;
    bool _mouseVisible = true;

public:
    explicit Window(int width = 800, int height = 600, const std::string &title = "Kuafu App", uint32_t flags = {}, Camera* camera = nullptr);

    virtual ~Window();

    Window(const Window &) = delete;

    Window(const Window &&) = delete;

    Window &operator=(const Window &) = delete;

    Window &operator=(const Window &&) = delete;

    bool init();

    bool update();

    virtual void resize(int, int);

    SDL_Window *get() { return pWindow; };

    vk::Extent2D getSize() const;

    int getWidth() const { return mWidth; }

    int getHeight() const { return mHeight; }

    bool changed();

    bool minimized();

    std::vector<const char*> getExtensions() const;
};

}
