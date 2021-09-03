#include "CustomCamera.hpp"
#include "Example.hpp"
#include "CustomWindow.hpp"
#include "core/context/global.hpp"

int main() {
    const int width = 800;
    const int height = 600;

    auto config = std::make_shared<kuafu::Config>();
    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(512);
    config->setPathDepth(12);
    config->setRussianRoulette(false);

    auto camera = std::make_shared<CustomCamera>(
            width, height, glm::vec3(0.f, 0.f, 3.f));

    auto window = std::make_shared<CustomWindow>(
            width, height, "Test", SDL_WINDOW_RESIZABLE, camera);

    kuafu::Kuafu renderer(config, window, camera);

//    auto &scene = renderer.getScene();

    loadScene(&renderer, Level::eActive);

//    glm::mat4 m = glm::mat4(1.0);
//    m[3][0] = 17;
//    camera->setPose(m);


    while (renderer.isRunning()) {
//        updateScene(&renderer);
        renderer.run();
    }

    return 0;
}
