#include "CustomCamera.hpp"
#include "Example.hpp"
#include "CustomWindow.hpp"
#include "core/context/global.hpp"

int main() {
    const int width = 800;
    const int height = 600;

    auto config = std::make_shared<kuafu::Config>();
    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(8);
    config->setPathDepth(3);
    config->setRussianRoulette(false);

    auto camera = std::make_shared<CustomCamera>(
            width, height, glm::vec3(0.f, 0.f, 3.f));

    auto window = std::make_shared<CustomWindow>(
            width, height, "Test", SDL_WINDOW_RESIZABLE, camera);

    kuafu::Kuafu renderer(config, window, camera);

    loadScene(&renderer, Level::eSpheres);     // eActive, eCornell

    size_t counter = 0;
    while (renderer.isRunning()) {
//        updateScene(&renderer);
        renderer.run();

//        if (counter % 100 == 0)
//            auto ret = renderer.downloadLatestFrame();

        counter++;
    }

    return 0;
}
