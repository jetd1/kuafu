//#include "CustomCamera.hpp"
#include "Example.hpp"
#include "core/context/global.hpp"
#include <spdlog/spdlog.h>

int main() {
    const int width = 800;
    const int height = 600;
    kuafu::global::logger->set_level(spdlog::level::debug);

    auto config = std::make_shared<kuafu::Config>();
    config->setInitialWidth(width);
    config->setInitialHeight(height);

    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(1);
    config->setPathDepth(8);
    config->setRussianRoulette(false);
    config->setPresent(true);
    config->setAccumulatingFrames(false);
    config->setUseDenoiser(true);
    config->setGeometryLimit(20000);
    config->setGeometryInstanceLimit(20000);

    kuafu::Kuafu renderer(config);

    auto scene = renderer.getScene();
    loadScene(scene, Level::eTexture);     // eSpheres, eActive, eCornell, eGLTF

    auto scene1 = renderer.createScene();
    scene1->setCamera(scene1->createCamera(width, height));
    loadScene(scene1, Level::eGround);

    size_t counter = 0;
    while (renderer.isRunning()) {

        if (counter % 400 == 0)
            renderer.setScene(scene);

        if (counter % 400 == 200)
            renderer.setScene(scene1);


//        updateScene(scene);
        renderer.run();

//        auto ret = renderer.downloadLatestFrame();
//        KF_INFO("Downloaded!");

//        if (counter == 1000) {
//            auto camera1 = std::make_shared<CustomCamera>(
//                    width * 2, height * 2, glm::vec3(0.f, 0.f, 3.f));
//            renderer.getScene().setCamera(camera1);
//        }

//        if (counter == 200) {
//            renderer.getScene().setCamera(camera1);
//        }

        counter++;
    }

    return 0;
}
