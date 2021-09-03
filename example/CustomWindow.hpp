#pragma once
#include "Keys.hpp"
#include "kuafu.hpp"

class CustomWindow : public kuafu::Window {
public:
    CustomWindow(int width, int height, const char *title, uint32_t flags
                 , std::shared_ptr<kuafu::Camera> camera):
            kuafu::Window(width, height, title, flags), pCamera(camera)
            { }

    auto init() -> bool override {
        if (!kuafu::Window::init()) {
            return false;
        }

        SDL_SetRelativeMouseMode(SDL_FALSE);
//        SDL_SetRelativeMouseMode(SDL_TRUE);
        return true;
    }

    auto update() -> bool override {
        if (!kuafu::Window::update()) {
            return false;
        }

//        _scene->getCamera()->setSize(mWidth, mHeight);

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
                            Key::eW = true;
                            break;

                        case SDLK_a:
                            Key::eA = true;
                            break;

                        case SDLK_s:
                            Key::eS = true;
                            break;

                        case SDLK_d:
                            Key::eD = true;
                            break;

                        case SDLK_LSHIFT:
                            Key::eLeftShift = true;
                            break;

                        case SDLK_ESCAPE:
                            return false;

                        case SDLK_c:
                            Key::eC = true;
                            break;

                        case SDLK_b:
                            Key::eB = true;
                            break;

                        case SDLK_l:
                            Key::eL = true;
                            break;

                        case SDLK_LCTRL:
                            Key::eLeftCtrl = true;
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
                            Key::eW = false;
                            break;

                        case SDLK_a:
                            Key::eA = false;
                            break;

                        case SDLK_s:
                            Key::eS = false;
                            break;

                        case SDLK_d:
                            Key::eD = false;
                            break;

                        case SDLK_LSHIFT:
                            Key::eLeftShift = false;
                            break;

                        case SDLK_LCTRL:
                            Key::eLeftCtrl = false;
                            break;

                        case SDLK_c:
                            Key::eC = false;
                            break;

                        case SDLK_b:
                            Key::eB = false;
                            break;

                        case SDLK_l:
                            Key::eL = false;
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

private:
    std::shared_ptr<kuafu::Camera> pCamera;
    bool _mouseVisible = true;
};

