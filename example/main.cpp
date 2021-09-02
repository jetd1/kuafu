#include "CustomCamera.hpp"
#include "Example.hpp"
#include "CustomWindow.hpp"
#include "core/context/global.hpp"

int main() {
    const int width = 640;
    const int height = 640;

    auto config = std::make_shared<kuafu::Config>();
    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(128);
    config->setPathDepth(12);
    config->setRussianRoulette(false);

    auto camera = std::make_shared<CustomCamera>(
            width, height, glm::vec3(0.f, 0.f, 3.f));

    auto window = std::make_shared<CustomWindow>(
            width, height, "Test", SDL_WINDOW_RESIZABLE, camera);

    kuafu::Kuafu renderer(config, window, camera);

//    auto &scene = renderer.getScene();

    loadScene(&renderer, Level::eSpheres);

    while (renderer.isRunning()) {
//        updateScene(&renderer);
        renderer.run();
    }

    return 0;
}
