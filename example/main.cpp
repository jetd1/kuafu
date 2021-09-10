#include "CustomCamera.hpp"
#include "Example.hpp"
#include "CustomWindow.hpp"
#include "core/context/global.hpp"
#include <spdlog/spdlog.h>

int main() {
    const int width = 800;
    const int height = 600;
    kuafu::global::logger->set_level(spdlog::level::debug);

    auto config = std::make_shared<kuafu::Config>();
    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(4);
    config->setPathDepth(3);
    config->setRussianRoulette(false);

    auto camera = std::make_shared<CustomCamera>(
            width, height, glm::vec3(0.f, 0.f, 3.f));

    auto window = std::make_shared<CustomWindow>(
            width, height, "Test", SDL_WINDOW_RESIZABLE, camera);

    kuafu::Kuafu renderer(config, window, camera);

    loadScene(&renderer, Level::eSpheres);     // eSpheres, eActive, eCornell, eGLTF
//
//    window->resize(1920, 1080);
//    camera->setFullPerspective(
//            1920, 1080,
//            1.387511840820312500e+03, 1.386223266601562500e+03,
//            9.825937500000000000e+02, 5.653156127929687500e+02, 0);

    size_t counter = 0;
    while (renderer.isRunning()) {
        updateScene(renderer);
        renderer.run();

//        if (counter % 100 == 0)
//            auto ret = renderer.downloadLatestFrame();

        counter++;
    }

    return 0;
}
