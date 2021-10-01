//#include "CustomCamera.hpp"
#include "Example.hpp"
#include "core/context/global.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

int main() {
    const int width = 800;
    const int height = 600;
    kuafu::global::logger->set_level(spdlog::level::debug);

    auto config = std::make_shared<kuafu::Config>();
    config->setInitialWidth(width);
    config->setInitialHeight(height);

    config->setAssetsPath("../resources");
    config->setPerPixelSampleRate(32);
    config->setPathDepth(8);
    config->setRussianRoulette(false);
    config->setPresent(true);
    config->setAccumulatingFrames(true);
    config->setUseDenoiser(false);
    config->setGeometryLimit(20000);
    config->setGeometryInstanceLimit(20000);

    kuafu::Kuafu renderer(config);

    auto scene = renderer.getScene();
    loadScene(scene, Level::eSpheres);     // eSpheres, eActive, eCornell, eGLTF

//    auto scene1 = renderer.createScene();
//    scene1->setCamera(scene1->createCamera(width, height));
//    loadScene(scene1, Level::eGround);

    size_t counter = 0;
    while (renderer.isRunning()) {

//        updateScene(scene);
        renderer.run();

//        auto ret = renderer.downloadLatestFrame(renderer.getScene()->getCamera());
//        KF_INFO("Downloaded!");
//        stbi_write_jpg("1.png", 800, 600, 4, ret.data(), 95);

//        if (counter == 100) {
////            renderer.getWindow()->resize(width * 2, height * 2);
//            auto camera1 = renderer.getScene()->createCamera(width * 2, height * 2);
//            renderer.getScene()->setCamera(camera1);
//        }

        counter++;
    }

    return 0;
}
