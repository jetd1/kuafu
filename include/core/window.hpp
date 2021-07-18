//
// Created by jet on 4/9/21.
//

#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "sdl2/SDL.h"
#include "gsl/gsl"

namespace kuafu {

    class Window {
    protected:
        SDL_Window *mWindow = nullptr;
        uint32_t mFlags = 0;

        int mWidth = 800;
        int mHeight = 600;
        std::string mTitle = "App";

    public:
        Window(int width = 800, int height = 600, std::string title = "App", uint32_t flags = 0);

        virtual ~Window();

        Window(const Window &) = delete;

        Window(const Window &&) = delete;

        Window &operator=(const Window &) = delete;

        Window &operator=(const Window &&) = delete;

        virtual bool init();

        virtual bool update();

        virtual void resize(int, int);

        SDL_Window *get() { return mWindow; };

        vk::Extent2D getSize() const;

        int getWidth() const { return mWidth; }

        int getHeight() const { return mHeight; }

        bool changed();

        bool minimized();

        gsl::span<const char *> getExtensions() const;
    };

}
